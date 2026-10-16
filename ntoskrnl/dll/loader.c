#include "loader.h"
#include "../mm/mm.h"
#include "../../drivers/fs/vfs/vfs.h"
#include <nt/pe.h>
#include <rt.h>
#include <dbg/print/kdprint.h>
#include "../ps/ps.h"

extern void  *PmmAllocatePage(void);
extern void   PmmFreePage(void *Page);

static int      StrEqI(const char *a, const char *b);
static int      ReadFile(const char *path, uint8_t *buf, uint32_t max);
static int      FlattenPE(const uint8_t *file, uint32_t fileSize,
                          uint8_t *out, uint32_t outMax,
                          IMAGE_OPTIONAL_HEADER64 **ohOut,
                          IMAGE_FILE_HEADER       **fhOut,
                          IMAGE_SECTION_HEADER    **shOut);
static void     ParseExports(IMAGE_EXPORTS *ex, uint8_t *img,
                             IMAGE_OPTIONAL_HEADER64 *oh);
static void    *FindExport(IMAGE_EXPORTS *ex, const char *name);
IMAGE_EXPORTS  *KipGetKernelExports(void);

static uint8_t        gFile[IMG_LOAD_MAX_SIZE];
static uint8_t        gImage[IMG_LOAD_MAX_SIZE];
static IMAGE_EXPORTS  gNtdllExportsStore;
static IMAGE_EXPORTS *gNtdllExports = 0;

static IMAGE_EXPORTS  gKernelExports;
static int            gKernelExportsInit = 0;

IMAGE_EXPORTS *KipGetNtdllExports(void) { return gNtdllExports; }
void KipResetNtdllExports(void) { gNtdllExports = 0; }


NTSTATUS KipLoadKernelImage(const char *Path,
                            uint64_t PreferredBase,   /* 0xFFFFF80000000000+ */
                            IMAGE_EXPORTS **OutExports,
                            uint64_t *OutEntry)
{
    if (!Path || !PreferredBase) return STATUS_INVALID_PARAMETER;

    int n = ReadFile(Path, gFile, IMG_LOAD_MAX_SIZE);
    if (n <= 0) {
        KdPrintf("[KipK] read fail: %s\n", Path);
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    IMAGE_OPTIONAL_HEADER64 *oh;
    IMAGE_FILE_HEADER       *fh;
    IMAGE_SECTION_HEADER    *sh;
    if (FlattenPE(gFile, (uint32_t)n, gImage, IMG_LOAD_MAX_SIZE,
                  &oh, &fh, &sh) != 0) {
        KdPrintf("[KipK] not a PE32+: %s\n", Path);
        return STATUS_INVALID_IMAGE_FORMAT;
    }

    const char *name = Path;
    for (const char *p = Path; *p; p++)
        if (*p == '\\' || *p == '/') name = p + 1;

    IMAGE_EXPORTS *kexp = KipGetKernelExports();
    if (oh->NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_IMPORT) {
        IMAGE_DATA_DIRECTORY d = oh->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
        if (d.VirtualAddress && d.Size) {
            IMAGE_IMPORT_DESCRIPTOR *imp =
                (IMAGE_IMPORT_DESCRIPTOR *)(gImage + d.VirtualAddress);
            for (; imp->Name; imp++) {
                const char *depname = (const char *)(gImage + imp->Name);
                IMAGE_EXPORTS *depEx = 0;
                if (!gKernelExportsInit) {
                     gKernelExportsInit = 1;
                     gKernelExports.Base = 0;
                     gKernelExports.Size = 0;
                     const char *nm = "ntoskrnl.exe";
                                          int j = 0;
                        while (nm[j] && j < 63) { gKernelExports.Name[j] = nm[j]; j++; }
                            gKernelExports.Name[j] = 0; }
                if (!depEx) {
                    KdPrintf("[KipK] unknown dep: %s\n", depname);
                    return STATUS_NOT_FOUND;
                }
                uint32_t oft = imp->OriginalFirstThunk ?
                               imp->OriginalFirstThunk : imp->FirstThunk;
                uint64_t *orig = (uint64_t *)(gImage + oft);
                uint64_t *iat  = (uint64_t *)(gImage + imp->FirstThunk);
                for (int k = 0; orig[k]; k++) {
                    IMAGE_IMPORT_BY_NAME *ibn =
                        (IMAGE_IMPORT_BY_NAME *)(gImage + (uint32_t)orig[k]);
                    void *addr = FindExport(depEx, ibn->Name);
                    if (!addr) {
                        KdPrintf("[KipK] unresolved: %s!%s\n",
                                 depname, ibn->Name);
                        return STATUS_NOT_FOUND;
                    }
                   iat[k] = (uint64_t)(uintptr_t)addr;
                }
            }
        }
    }

    IMAGE_SECTION_ENTRY *sec = 0;
    NTSTATUS st = ImgSecCreate(name, gImage, oh->SizeOfImage,
                               PreferredBase, &sec);
    if (!NT_SUCCESS(st)) {
        KdPrintf("[KipK] ImgSecCreate failed for %s\n", name);
        return st;
    }

    extern uint64_t MmGetKernelPml4(void);
    st = ImgSecMap(sec, MmGetKernelPml4());
    if (!NT_SUCCESS(st)) {
        KdPrintf("[KipK] ImgSecMap failed for %s\n", name);
        return st;
    }

    if (OutExports) {
        static IMAGE_EXPORTS sTmp;
        for (uint32_t i = 0; i < sizeof(sTmp); i++) ((uint8_t *)&sTmp)[i] = 0;
        uint32_t j = 0;
        while (name[j] && j < 63) { sTmp.Name[j] = name[j]; j++; }
        sTmp.Name[j] = 0;
        sTmp.Base = sec->Base;
        sTmp.Size = oh->SizeOfImage;
        ParseExports(&sTmp, gImage, oh);
        *OutExports = &sTmp;
    }

    if (OutEntry) *OutEntry = sec->Base + oh->AddressOfEntryPoint;

    KdPrintf("[KipK] loaded '%s' base=0x%llx entry=0x%llx\n",
             name, (unsigned long long)sec->Base,
             (unsigned long long)(sec->Base + oh->AddressOfEntryPoint));
    return STATUS_SUCCESS;
}

static int StrEqI(const char *a, const char *b)
{
    for (; *a && *b; a++, b++) {
        char ca = *a, cb = *b;
        if (ca >= 'a' && ca <= 'z') ca -= 32;
        if (cb >= 'a' && cb <= 'z') cb -= 32;
        if (ca != cb) return 0;
    }
    return *a == 0 && *b == 0;
}

static int ReadFile(const char *path, uint8_t *buf, uint32_t max)
{
    VFS_FILE f;
    if (VfsOpen(path, &f) != 0) return -1;
    int n = VfsRead(&f, buf, max);
    VfsClose(&f);
    return n;
}

static int FlattenPE(const uint8_t *file, uint32_t fileSize,
                     uint8_t *out, uint32_t outMax,
                     IMAGE_OPTIONAL_HEADER64 **ohOut,
                     IMAGE_FILE_HEADER **fhOut,
                     IMAGE_SECTION_HEADER **shOut)
{
    KdPrintf("[PE] fileSize=%u outMax=%u\n", fileSize, outMax);
    KdPrintf("[PE] first16: %02x %02x %02x %02x %02x %02x %02x %02x "
             "%02x %02x %02x %02x %02x %02x %02x %02x\n",
             file[0], file[1], file[2], file[3],
             file[4], file[5], file[6], file[7],
             file[8], file[9], file[10], file[11],
             file[12], file[13], file[14], file[15]);

    if (fileSize < sizeof(IMAGE_DOS_HEADER)) {
        KdPrintf("[PE] FAIL: fileSize %u < sizeof(DOS) %u\n",
                 fileSize, (unsigned)sizeof(IMAGE_DOS_HEADER));
        return -1;
    }
    IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)file;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
        KdPrintf("[PE] FAIL: e_magic=0x%04x expect 0x%04x\n",
                 dos->e_magic, IMAGE_DOS_SIGNATURE);
        return -1;
    }
    KdPrintf("[PE] e_magic OK, e_lfanew=0x%x\n", dos->e_lfanew);

    if ((uint64_t)dos->e_lfanew + 4 + sizeof(IMAGE_FILE_HEADER) +
        sizeof(IMAGE_OPTIONAL_HEADER64) > fileSize) {
        KdPrintf("[PE] FAIL: NT hdrs extend past file "
                 "(e_lfanew=0x%x + %u + %u + %u > %u)\n",
                 dos->e_lfanew,
                 (unsigned)(4 + sizeof(IMAGE_FILE_HEADER) +
                            sizeof(IMAGE_OPTIONAL_HEADER64)),
                 0u, 0u, fileSize);
        return -1;
    }

    uint8_t *nt = (uint8_t *)file + dos->e_lfanew;
    KdPrintf("[PE] NT sig bytes: %02x %02x %02x %02x\n",
             nt[0], nt[1], nt[2], nt[3]);
    if (*(uint32_t *)nt != IMAGE_NT_SIGNATURE) {
        KdPrintf("[PE] FAIL: NT sig=0x%08x expect 0x%08x\n",
                 *(uint32_t *)nt, IMAGE_NT_SIGNATURE);
        return -1;
    }

    IMAGE_FILE_HEADER *fh = (IMAGE_FILE_HEADER *)(nt + 4);
    KdPrintf("[PE] Machine=0x%04x (expect 0x%04x) NumSec=%u OptHdrSz=%u\n",
             fh->Machine, IMAGE_FILE_MACHINE_AMD64,
             fh->NumberOfSections, fh->SizeOfOptionalHeader);
    if (fh->Machine != IMAGE_FILE_MACHINE_AMD64) {
        KdPrintf("[PE] FAIL: machine mismatch (got 0x%04x, want 0x%04x)\n",
                 fh->Machine, IMAGE_FILE_MACHINE_AMD64);
        return -1;
    }

    IMAGE_OPTIONAL_HEADER64 *oh =
        (IMAGE_OPTIONAL_HEADER64 *)((uint8_t *)fh + sizeof(IMAGE_FILE_HEADER));
    KdPrintf("[PE] Magic=0x%04x (expect 0x%04x) "
             "SizeOfImage=0x%x ImageBase=0x%llx EntryRVA=0x%x\n",
             oh->Magic, IMAGE_NT_OPTIONAL_HDR64_MAGIC,
             oh->SizeOfImage,
             (unsigned long long)oh->ImageBase,
             oh->AddressOfEntryPoint);
    if (oh->Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
        KdPrintf("[PE] FAIL: magic mismatch (got 0x%04x, want 0x%04x)\n",
                 oh->Magic, IMAGE_NT_OPTIONAL_HDR64_MAGIC);
        return -1;
    }
    if (oh->SizeOfImage == 0) {
        KdPrintf("[PE] FAIL: SizeOfImage == 0\n");
        return -1;
    }
    if (oh->SizeOfImage > outMax) {
        KdPrintf("[PE] FAIL: SizeOfImage 0x%x > outMax 0x%x "
                 "(IMG_LOAD_MAX_SIZE too small!)\n",
                 oh->SizeOfImage, outMax);
        return -1;
    }

    IMAGE_SECTION_HEADER *sh =
        (IMAGE_SECTION_HEADER *)((uint8_t *)oh + fh->SizeOfOptionalHeader);

    for (uint64_t i = 0; i < oh->SizeOfImage; i++) out[i] = 0;

    uint32_t hdrsz = oh->SizeOfHeaders;
    if (hdrsz > fileSize)         hdrsz = fileSize;
    if (hdrsz > oh->SizeOfImage)  hdrsz = oh->SizeOfImage;
    for (uint32_t i = 0; i < hdrsz; i++) out[i] = file[i];

        for (uint16_t i = 0; i < fh->NumberOfSections; i++) {
            IMAGE_SECTION_HEADER *s = &sh[i];

            char sn[9];
        for (int k = 0; k < 8; k++) {
           char c = (char)s->Name[k];
            sn[k] = (c >= 32 && c < 127) ? c : '.';
    }
    sn[8] = 0;

    KdPrintf("[PE] sec[%u]: '%s' VA=0x%x VSz=0x%x RawOff=0x%x RawSz=0x%x\n",
             (unsigned)i, sn,
             (unsigned)s->VirtualAddress,
             (unsigned)s->Misc.VirtualSize,
             (unsigned)s->PointerToRawData,
             (unsigned)s->SizeOfRawData);


        if (!s->SizeOfRawData) continue;

        if ((uint64_t)s->PointerToRawData + s->SizeOfRawData > fileSize) {
            KdPrintf("[PE] FAIL: sec[%u] raw overflow file "
                     "(0x%x + 0x%x > 0x%x)\n",
                     i, s->PointerToRawData, s->SizeOfRawData, fileSize);
            return -1;
        }
        if ((uint64_t)s->VirtualAddress + s->SizeOfRawData > oh->SizeOfImage) {
            KdPrintf("[PE] FAIL: sec[%u] virt overflow image "
                     "(0x%x + 0x%x > 0x%x)\n",
                     i, s->VirtualAddress, s->SizeOfRawData,
                     oh->SizeOfImage);
            return -1;
        }

        for (uint32_t k = 0; k < s->SizeOfRawData; k++)
            out[s->VirtualAddress + k] = file[s->PointerToRawData + k];
    }

    KdPrintf("[PE] OK: flattened, SizeOfImage=0x%x NumSec=%u\n",
             oh->SizeOfImage, fh->NumberOfSections);

    *ohOut = oh;
    *fhOut = fh;
    *shOut = sh;
    return 0;
}
static void ParseExports(IMAGE_EXPORTS *ex, uint8_t *img,
                         IMAGE_OPTIONAL_HEADER64 *oh)
{
    ex->ExportCount = 0;
    if (oh->NumberOfRvaAndSizes <= IMAGE_DIRECTORY_ENTRY_EXPORT) return;
    IMAGE_DATA_DIRECTORY d = oh->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if (!d.VirtualAddress || !d.Size) return;

    IMAGE_EXPORT_DIRECTORY *ed =
        (IMAGE_EXPORT_DIRECTORY *)(img + d.VirtualAddress);
    uint32_t n = ed->NumberOfNames;
    if (n > 256) n = 256;

    uint32_t *names = (uint32_t *)(img + ed->AddressOfNames);
    uint16_t *ords  = (uint16_t *)(img + ed->AddressOfNameOrdinals);
    uint32_t *fns   = (uint32_t *)(img + ed->AddressOfFunctions);

    for (uint32_t i = 0; i < n; i++) {
        const char *nm = (const char *)(img + names[i]);
        uint32_t j = 0;
        while (nm[j] && j < 63) { ex->Exports[ex->ExportCount].Name[j] = nm[j]; j++; }
        ex->Exports[ex->ExportCount].Name[j] = 0;
        ex->Exports[ex->ExportCount].Rva = fns[ords[i]];
        ex->ExportCount++;
    }
}

static void *FindExport(IMAGE_EXPORTS *ex, const char *name)
{
    if (!ex) return 0;
    for (uint32_t i = 0; i < ex->ExportCount; i++)
        if (StrEqI(ex->Exports[i].Name, name))
            return (void *)(uintptr_t)(ex->Base + ex->Exports[i].Rva);
    return 0;
}

NTSTATUS KipLoadImage(EPROCESS *Proc, const char *Path,
                      IMAGE_EXPORTS *Fallback, int ResolveSelf,
                      IMAGE_SECTION_ENTRY **OutSec,
                      IMAGE_EXPORTS **OutExports,
                      uint64_t *OutEntry)
{
    if (!Proc || !Path || !Proc->Pml4Phys) return STATUS_INVALID_PARAMETER;

    int n = ReadFile(Path, gFile, IMG_LOAD_MAX_SIZE);
    if (n <= 0) {
        KdPrintf("[Kip] read fail: %s\n", Path);
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    IMAGE_OPTIONAL_HEADER64 *oh;
    IMAGE_FILE_HEADER       *fh;
    IMAGE_SECTION_HEADER    *sh;
    if (FlattenPE(gFile, (uint32_t)n, gImage, IMG_LOAD_MAX_SIZE,
                  &oh, &fh, &sh) != 0) {
        KdPrintf("[Kip] not a PE32+: %s\n", Path);
        return STATUS_INVALID_IMAGE_FORMAT;
    }

    const char *name = Path;
    for (const char *p = Path; *p; p++) if (*p == '\\' || *p == '/') name = p + 1;

    IMAGE_EXPORTS *exp = 0;
    if (OutExports) {
        static IMAGE_EXPORTS sTmp;
        for (uint32_t i = 0; i < sizeof(sTmp); i++) ((uint8_t *)&sTmp)[i] = 0;
        uint32_t j = 0;
        while (name[j] && j < 63) { sTmp.Name[j] = name[j]; j++; }
        sTmp.Name[j] = 0;
        sTmp.Base = oh->ImageBase;
        sTmp.Size = oh->SizeOfImage;
        ParseExports(&sTmp, gImage, oh);
        exp = &sTmp;
    }

    if (oh->NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_IMPORT) {
        IMAGE_DATA_DIRECTORY d = oh->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
        if (d.VirtualAddress && d.Size) {
            IMAGE_IMPORT_DESCRIPTOR *imp =
                (IMAGE_IMPORT_DESCRIPTOR *)(gImage + d.VirtualAddress);
            for (; imp->Name; imp++) {
                const char *depname = (const char *)(gImage + imp->Name);

                uint64_t depBase = 0;
                IMAGE_EXPORTS *depEx = 0;

                if (ResolveSelf && StrEqI(depname, name)) {
                    depBase = oh->ImageBase;
                    depEx   = exp;
                } else if (Fallback && StrEqI(depname, Fallback->Name)) {
                    depBase = Fallback->Base;
                    depEx   = Fallback;
                } else {
                    IMAGE_SECTION_ENTRY *sec = ImgSecFind(depname);
                    if (sec) depBase = sec->Base;
                    if (!depEx && Fallback) depEx = Fallback;
                }
                (void)depBase;

                uint32_t oft = imp->OriginalFirstThunk ? imp->OriginalFirstThunk
                                                       : imp->FirstThunk;
                uint64_t *orig = (uint64_t *)(gImage + oft);
                uint64_t *iat  = (uint64_t *)(gImage + imp->FirstThunk);
                for (int k = 0; orig[k]; k++) {
                    uint64_t v = orig[k];
                    if (v & IMAGE_ORDINAL_FLAG64) {
                        KdPrintf("[Kip] ordinal import unsupported: %s\n", depname);
                        return STATUS_NOT_IMPLEMENTED;
                    }
                    IMAGE_IMPORT_BY_NAME *ibn =
                        (IMAGE_IMPORT_BY_NAME *)(gImage + (uint32_t)v);
                    void *addr = FindExport(depEx, ibn->Name);
                    if (!addr) {
                        KdPrintf("[Kip] unresolved: %s!%s\n", depname, ibn->Name);
                        return STATUS_OBJECT_NAME_NOT_FOUND;
                    }
                    iat[k] = (uint64_t)(uintptr_t)addr;
                }
            }
        }
    }

    IMAGE_SECTION_ENTRY *sec = 0;
    NTSTATUS st = ImgSecCreate(name, gImage, oh->SizeOfImage,
                               oh->ImageBase, &sec);
    if (!NT_SUCCESS(st)) {
        KdPrintf("[Kip] ImgSecCreate failed for %s: st=0x%08x\n",
                 name, (unsigned)st);
        return st;
    }

    st = ImgSecMap(sec, Proc->Pml4Phys);
    if (!NT_SUCCESS(st)) {
        KdPrintf("[Kip] ImgSecMap failed for %s: st=0x%08x\n",
                 name, (unsigned)st);
        return st;
    }

    if (StrEqI(name, "ntdll.dll")) {
        gNtdllExportsStore = *exp;
        gNtdllExports = &gNtdllExportsStore;
    }

    if (OutSec)     *OutSec = sec;
    if (OutExports) *OutExports = exp;
    if (OutEntry)   *OutEntry = sec->Base + oh->AddressOfEntryPoint;

    KdPrintf("[Kip] loaded '%s' base=0x%llx entry=0x%llx\n",
             name, (unsigned long long)sec->Base,
             (unsigned long long)(sec->Base + oh->AddressOfEntryPoint));
    return STATUS_SUCCESS;
}

static void KipRegisterKernelExport(const char *name, void *addr, uint32_t rva)
{
    IMAGE_EXPORTS *ex = &gKernelExports;
    uint32_t i = ex->ExportCount++;
    int j = 0;
    while (name[j] && j < 63) { ex->Exports[i].Name[j] = name[j]; j++; }
    ex->Exports[i].Name[j] = 0;
    ex->Exports[i].Rva = rva;
}

IMAGE_EXPORTS *KipGetKernelExports(void)
{
    if (!gKernelExportsInit) {
        gKernelExportsInit = 1;
        gKernelExports.Base = 0;  
        gKernelExports.Size = 0;
        uint32_t i = 0;
        KipRegisterKernelExport("MmAllocatePages",
                                (void*)MmAllocatePages, i++);
        KipRegisterKernelExport("MmFreePages",
                                (void*)MmFreePages, i++);
        KipRegisterKernelExport("MmGetKernelPml4",
                                (void*)MmGetKernelPml4, i++);
        KipRegisterKernelExport("MiMapPage",
                                (void*)MiMapPage, i++);
        KipRegisterKernelExport("MiUnmapPage",
                                (void*)MiUnmapPage, i++);
        KipRegisterKernelExport("KdPrintf",
                                (void*)KdPrintf, i++);
        KipRegisterKernelExport("KeGetCurrentThread",
                                (void*)KeGetCurrentThread, i++);
    }
    return &gKernelExports;
}

static void *FindKernelExport(IMAGE_EXPORTS *ex, const char *name)
{
    if (!ex) return 0;
    for (uint32_t i = 0; i < ex->ExportCount; i++)
        if (StrEqI(ex->Exports[i].Name, name))
            return (void *)(uintptr_t)ex->Exports[i].Rva;
    return 0;
}