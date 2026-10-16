#include <dbg/print/kdprint.h>
#include <ds/fb/fb.h>
#include "dll.h"
#include "../mm/mm.h"
#include <rt.h>
#include "../../drivers/fs/vfs/vfs.h"
#include <nt/pe.h>

#define DLL_FILE_MAX (1 * 1024 * 1024)
#define DLL_IMG_MAX  (2 * 1024 * 1024)
#define DLL_SEARCH   "\\NocturnalOS\\system\\libs\\"

static uint8_t   gDllFile[DLL_FILE_MAX];
static uint8_t   gDllImage[DLL_IMG_MAX];
static DLL_ENTRY gDllTable[DLL_MAX_LOADED];

extern int MiIsMapped(uint64_t Pml4, uint64_t Virt);


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

static void CopyI(char *dst, const char *src, uint32_t cap)
{
    uint32_t i = 0;
    while (src[i] && i + 1 < cap) { dst[i] = src[i]; i++; }
    dst[i] = 0;
}

static void MemZero(void *d, uint64_t n)
{ uint8_t *p = d; while (n--) *p++ = 0; }
static void MemCopy(void *d, const void *s, uint64_t n)
{ uint8_t *dp = d; const uint8_t *sp = s; while (n--) *dp++ = *sp++; }


static int DllReadFile(const char *path, uint8_t *buf, uint32_t max)
{
    VFS_FILE f;
    if (VfsOpen(path, &f) != 0) return -1;
    int n = VfsRead(&f, buf, max);
    VfsClose(&f);
    return n;
}


int DllInitSystem(void)
{
    MemZero(gDllTable, sizeof(gDllTable));
    KdPrint("[DLL] subsystem ready\n");
    return 0;
}

DLL_ENTRY *DllFindByName(const char *name)
{
    for (int i = 0; i < DLL_MAX_LOADED; i++)
        if (gDllTable[i].Base && StrEqI(gDllTable[i].Name, name))
            return &gDllTable[i];
    return 0;
}

static DLL_ENTRY *DllFindByBase(uint64_t base)
{
    for (int i = 0; i < DLL_MAX_LOADED; i++)
        if (gDllTable[i].Base == base && base) return &gDllTable[i];
    return 0;
}

static DLL_ENTRY *DllAllocSlot(void)
{
    for (int i = 0; i < DLL_MAX_LOADED; i++)
        if (gDllTable[i].Base == 0) return &gDllTable[i];
    return 0;
}


static void ParseExports(DLL_ENTRY *e, uint8_t *img, IMAGE_OPTIONAL_HEADER64 *oh)
{
    e->ExportCount = 0;
    if (oh->NumberOfRvaAndSizes <= IMAGE_DIRECTORY_ENTRY_EXPORT) return;
    IMAGE_DATA_DIRECTORY d = oh->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if (!d.VirtualAddress || !d.Size) return;

    IMAGE_EXPORT_DIRECTORY *ex = (IMAGE_EXPORT_DIRECTORY *)(img + d.VirtualAddress);
    uint32_t n = ex->NumberOfNames;
    if (n > DLL_EXPORT_MAX) n = DLL_EXPORT_MAX;

    uint32_t *names = (uint32_t *)(img + ex->AddressOfNames);
    uint16_t *ords  = (uint16_t *)(img + ex->AddressOfNameOrdinals);
    uint32_t *fns   = (uint32_t *)(img + ex->AddressOfFunctions);

    for (uint32_t i = 0; i < n; i++) {
        const char *nm = (const char *)(img + names[i]);
        CopyI(e->Exports[e->ExportCount].Name, nm, 64);
        e->Exports[e->ExportCount].Rva = fns[ords[i]];
        e->ExportCount++;
    }
}

void *DllGetProcAddress(DLL_ENTRY *e, const char *func)
{
    if (!e || !func) return 0;
    for (uint32_t i = 0; i < e->ExportCount; i++)
        if (StrEqI(e->Exports[i].Name, func))
            return (void *)(uintptr_t)(e->Base + e->Exports[i].Rva);
    return 0;
}

static int BuildImportPath(const char *name, char *out, uint32_t cap)
{
    uint32_t p = 0;
    const char *pre = DLL_SEARCH;
    while (pre[p] && p + 1 < cap) { out[p] = pre[p]; p++; }
    uint32_t j = 0;
    while (name[j] && p + 1 < cap) out[p++] = name[j++];
    const char *ext = ".DLL";
    j = 0;
    while (ext[j] && p + 1 < cap) out[p++] = ext[j++];
    out[p] = 0;
    return 0;
}

DLL_ENTRY *DllLoad(const char *name, const char *path)
{
    DLL_ENTRY *e = DllFindByName(name);
    if (e) return e;

    int n = DllReadFile(path, gDllFile, DLL_FILE_MAX);
    if (n <= 0) { KdPrintf("[DLL] read fail: %s\n", path); return 0; }

    IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)gDllFile;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) { KdPrint("[DLL] bad MZ\n"); return 0; }
    uint8_t *nt = gDllFile + dos->e_lfanew;
    if (*(uint32_t *)nt != IMAGE_NT_SIGNATURE) { KdPrint("[DLL] bad NT\n"); return 0; }
    IMAGE_FILE_HEADER *fh = (IMAGE_FILE_HEADER *)(nt + 4);
    IMAGE_OPTIONAL_HEADER64 *oh =
        (IMAGE_OPTIONAL_HEADER64 *)((uint8_t *)fh + sizeof(IMAGE_FILE_HEADER));
    if (oh->Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC) { KdPrint("[DLL] not PE32+\n"); return 0; }
    if (oh->SizeOfImage > DLL_IMG_MAX) { KdPrint("[DLL] image too big\n"); return 0; }

    IMAGE_SECTION_HEADER *sh =
        (IMAGE_SECTION_HEADER *)((uint8_t *)oh + fh->SizeOfOptionalHeader);

    uint64_t base = oh->ImageBase;
    if (DllFindByBase(base)) {
        KdPrintf("[DLL] base collision 0x%llx for %s\n",
                 (unsigned long long)base, name);
        return 0;
    }

    char imp_names[DLL_DEPS_MAX][64];
    int  imp_cnt = 0;
    if (oh->NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_IMPORT) {
        IMAGE_DATA_DIRECTORY d = oh->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
        if (d.VirtualAddress && d.Size) {
            IMAGE_IMPORT_DESCRIPTOR *imp =
                (IMAGE_IMPORT_DESCRIPTOR *)(gDllFile + d.VirtualAddress);
            for (; imp->Name && imp_cnt < DLL_DEPS_MAX; imp++) {
                CopyI(imp_names[imp_cnt++],
                      (const char *)(gDllFile + imp->Name), 64);
            }
        }
    }

    e = DllAllocSlot();
    if (!e) return 0;
    CopyI(e->Name, name, DLL_NAME_MAX);
    CopyI(e->Path, path, DLL_PATH_MAX);
    e->Base = base;          
    e->RefCount = 0;         

    for (int i = 0; i < imp_cnt; i++) {
        DLL_ENTRY *dep = DllFindByName(imp_names[i]);
        if (!dep) {
            char dp[DLL_PATH_MAX];
            BuildImportPath(imp_names[i], dp, DLL_PATH_MAX);
            dep = DllLoad(imp_names[i], dp);
            if (!dep) {
                KdPrintf("[DLL] dep fail: %s -> %s\n", name, imp_names[i]);
                MemZero(e, sizeof(*e));
                return 0;
            }
        }
        e->Deps[e->DepCount++] = dep;
    }

    if (DllReadFile(path, gDllFile, DLL_FILE_MAX) <= 0) {
        MemZero(e, sizeof(*e)); return 0;
    }
    dos = (IMAGE_DOS_HEADER *)gDllFile;
    nt  = gDllFile + dos->e_lfanew;
    fh  = (IMAGE_FILE_HEADER *)(nt + 4);
    oh  = (IMAGE_OPTIONAL_HEADER64 *)((uint8_t *)fh + sizeof(IMAGE_FILE_HEADER));
    sh  = (IMAGE_SECTION_HEADER *)((uint8_t *)oh + fh->SizeOfOptionalHeader);

    MemZero(gDllImage, oh->SizeOfImage);
    uint32_t hdrsz = oh->SizeOfHeaders;
    if (hdrsz > DLL_FILE_MAX) hdrsz = DLL_FILE_MAX;
    if (hdrsz > oh->SizeOfImage) hdrsz = oh->SizeOfImage;
    MemCopy(gDllImage, gDllFile, hdrsz);

    for (uint16_t i = 0; i < fh->NumberOfSections; i++) {
        IMAGE_SECTION_HEADER *s = &sh[i];
        if (!s->SizeOfRawData) continue;
        if (s->PointerToRawData + s->SizeOfRawData > DLL_FILE_MAX) {
            MemZero(e, sizeof(*e)); return 0;
        }
        if (s->VirtualAddress + s->SizeOfRawData > oh->SizeOfImage) {
            MemZero(e, sizeof(*e)); return 0;
        }
        MemCopy(gDllImage + s->VirtualAddress,
                gDllFile + s->PointerToRawData,
                s->SizeOfRawData);
    }

    if (oh->NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_IMPORT) {
        IMAGE_DATA_DIRECTORY d = oh->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
        if (d.VirtualAddress && d.Size) {
            IMAGE_IMPORT_DESCRIPTOR *imp =
                (IMAGE_IMPORT_DESCRIPTOR *)(gDllImage + d.VirtualAddress);
            for (; imp->Name; imp++) {
                const char *depname = (const char *)(gDllImage + imp->Name);
                DLL_ENTRY *dep = DllFindByName(depname);
                if (!dep) {
                    KdPrintf("[DLL] missing dep: %s\n", depname);
                    MemZero(e, sizeof(*e)); return 0;
                }
                uint64_t *orig = (uint64_t *)(gDllImage + imp->OriginalFirstThunk);
                uint64_t *iat  = (uint64_t *)(gDllImage + imp->FirstThunk);
                for (int k = 0; orig[k]; k++) {
                    uint64_t v = orig[k];
                    if (v & IMAGE_ORDINAL_FLAG64) {
                        KdPrintf("[DLL] ordinal import unsupported: %s\n", depname);
                        MemZero(e, sizeof(*e)); return 0;
                    }
                    IMAGE_IMPORT_BY_NAME *ibn =
                        (IMAGE_IMPORT_BY_NAME *)(gDllImage + (uint32_t)v);
                    void *addr = DllGetProcAddress(dep, ibn->Name);
                    if (!addr) {
                        KdPrintf("[DLL] missing export: %s!%s\n", depname, ibn->Name);
                        MemZero(e, sizeof(*e)); return 0;
                    }
                    iat[k] = (uint64_t)(uintptr_t)addr;
                }
            }
        }
    }

    e->Base = base;
    e->Size = oh->SizeOfImage;
    ParseExports(e, gDllImage, oh);

    uint32_t pages = (oh->SizeOfImage + 4095) / 4096;
    if (pages > DLL_PAGES_MAX) { MemZero(e, sizeof(*e)); return 0; }
    for (uint32_t i = 0; i < pages; i++) {
        void *pg = PmmAllocatePage();
        if (!pg) {
            for (uint32_t j = 0; j < i; j++)
                PmmFreePage((void *)(uintptr_t)e->PhysPages[j]);
            MemZero(e, sizeof(*e));
            return 0;
        }
        MemCopy(pg, gDllImage + i * 4096, 4096);
        e->PhysPages[i] = (uint64_t)(uintptr_t)pg;
    }
    e->PageCount = pages;

    KdPrintf("[DLL] loaded %s base=0x%llx size=0x%x pages=%u exports=%u\n",
             name, (unsigned long long)base, oh->SizeOfImage,
             pages, e->ExportCount);
    return e;
}

NTSTATUS DllEnsureMapped(DLL_ENTRY *e, uint64_t Pml4Phys)
{
    if (!e || !Pml4Phys || !e->Base) return STATUS_INVALID_PARAMETER;

    if (MiIsMapped(Pml4Phys, e->Base)) return STATUS_SUCCESS;   

    for (uint32_t i = 0; i < e->DepCount; i++) {
        NTSTATUS st = DllEnsureMapped(e->Deps[i], Pml4Phys);
        if (!NT_SUCCESS(st)) return st;
    }

    for (uint32_t i = 0; i < e->PageCount; i++) {
        uint64_t va = e->Base + (uint64_t)i * 4096;
        if (MiMapPage(Pml4Phys, va, e->PhysPages[i], 1 | 2 | 4) != 0)
            return STATUS_UNSUCCESSFUL;
    }
    e->RefCount++;
    return STATUS_SUCCESS;
}

void DllReleaseForProcess(uint64_t Pml4Phys)
{
    for (int i = 0; i < DLL_MAX_LOADED; i++) {
        DLL_ENTRY *e = &gDllTable[i];
        if (!e->Base) continue;
        if (!MiIsMapped(Pml4Phys, e->Base)) continue;

        if (e->RefCount > 0) e->RefCount--;
        if (e->RefCount == 0) {
            for (uint32_t j = 0; j < e->PageCount; j++)
                PmmFreePage((void *)(uintptr_t)e->PhysPages[j]);
            MemZero(e, sizeof(*e));
        }
    }
}