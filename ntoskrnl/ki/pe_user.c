
#include "pe_user.h"
#include <rt.h>
#include "../mm/mm.h"
#include "../dll/dll.h"
#include <nt/pe.h>
#include <dbg/print/kdprint.h>
#include <ds/fb/fb.h>

extern int      MiMapPage(uint64_t Pml4Phys, uint64_t Virt, uint64_t Phys, uint64_t Flags);
extern int      MiIsMapped(uint64_t Pml4, uint64_t Virt);
extern uint64_t MmGetKernelPml4(void);
extern void    *PmmAllocatePage(void);

#define EXE_IMG_MAX (4u * 1024u * 1024u)
static uint8_t gExeImage[EXE_IMG_MAX];

static void MemZero(void *d, uint64_t n)
{ uint8_t *p = d; while (n--) *p++ = 0; }
static void MemCopy(void *d, const void *s, uint64_t n)
{ uint8_t *dp = d; const uint8_t *sp = s; while (n--) *dp++ = *sp++; }

static int ApplyRelocs(uint8_t *img, IMAGE_OPTIONAL_HEADER64 *oh, uint64_t delta)
{
    if (!delta) return 0;
    if (oh->NumberOfRvaAndSizes <= IMAGE_DIRECTORY_ENTRY_BASERELOC) return 0;
    IMAGE_DATA_DIRECTORY d = oh->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
    if (!d.VirtualAddress || !d.Size) return 0;

    uint32_t off = 0;
    while (off + 8 <= d.Size) {
        IMAGE_BASE_RELOCATION *blk =
            (IMAGE_BASE_RELOCATION *)(img + d.VirtualAddress + off);
        if (blk->SizeOfBlock < 8) break;
        uint32_t n = (blk->SizeOfBlock - 8) / 2;
        uint16_t *ents = (uint16_t *)((uint8_t *)blk + 8);
        for (uint32_t i = 0; i < n; i++) {
            uint16_t type = ents[i] >> 12;
            uint16_t ofs  = ents[i] & 0x0FFF;
            uint8_t *p = img + blk->VirtualAddress + ofs;
            switch (type) {
                case 0: break;
                case 10: *(uint64_t *)p += delta; break;
                case 3:  *(uint32_t *)p += (uint32_t)delta; break;
                default: return -1;
            }
        }
        off += blk->SizeOfBlock;
    }
    return 0;
}

int UserLoadPE(const uint8_t *FileBuf, uint32_t FileSize,
               uint64_t TargetBase, uint64_t Pml4Phys, USER_PE_RESULT *Out)
{
    if (!FileBuf || !Out) return -1;
    if (FileSize < sizeof(IMAGE_DOS_HEADER)) return -1;

    IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)FileBuf;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) { KdPrint("[PE] bad MZ\n"); return -2; }
    if ((uint64_t)dos->e_lfanew + 4 + sizeof(IMAGE_FILE_HEADER) +
        sizeof(IMAGE_OPTIONAL_HEADER64) > FileSize) return -3;

    uint8_t *nt = (uint8_t *)FileBuf + dos->e_lfanew;
    if (*(uint32_t *)nt != IMAGE_NT_SIGNATURE) { KdPrint("[PE] bad NT\n"); return -4; }
    IMAGE_FILE_HEADER *fh = (IMAGE_FILE_HEADER *)(nt + 4);
    if (fh->Machine != IMAGE_FILE_MACHINE_AMD64) return -5;
    IMAGE_OPTIONAL_HEADER64 *oh =
        (IMAGE_OPTIONAL_HEADER64 *)((uint8_t *)fh + sizeof(IMAGE_FILE_HEADER));
    if (oh->Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC) return -6;
    if (oh->SizeOfImage == 0 || oh->SizeOfImage > EXE_IMG_MAX) return -7;

    IMAGE_SECTION_HEADER *sh =
        (IMAGE_SECTION_HEADER *)((uint8_t *)oh + fh->SizeOfOptionalHeader);

    uint64_t base  = TargetBase ? TargetBase : oh->ImageBase;
    uint64_t kpml4 = Pml4Phys ? Pml4Phys : MmGetKernelPml4();
    int64_t  delta = (int64_t)(base - oh->ImageBase);

    KdPrintf("[PE] base=0x%llx preferred=0x%llx delta=%lld size=0x%x\n",
             (unsigned long long)base,
             (unsigned long long)oh->ImageBase,
             (long long)delta, oh->SizeOfImage);

    MemZero(gExeImage, oh->SizeOfImage);
    uint32_t hdrsz = oh->SizeOfHeaders;
    if (hdrsz > FileSize) hdrsz = FileSize;
    if (hdrsz > oh->SizeOfImage) hdrsz = oh->SizeOfImage;
    MemCopy(gExeImage, FileBuf, hdrsz);

    for (uint16_t i = 0; i < fh->NumberOfSections; i++) {
        IMAGE_SECTION_HEADER *s = &sh[i];
        if (!s->SizeOfRawData) continue;
        if ((uint64_t)s->PointerToRawData + s->SizeOfRawData > FileSize) return -8;
        if ((uint64_t)s->VirtualAddress + s->SizeOfRawData > oh->SizeOfImage) return -9;
        MemCopy(gExeImage + s->VirtualAddress,
                FileBuf + s->PointerToRawData,
                s->SizeOfRawData);
    }

    if (delta != 0) {
        if (ApplyRelocs(gExeImage, oh, (uint64_t)delta) != 0) {
            KdPrint("[PE] reloc failed\n");
            return -10;
        }
    }

    if (oh->NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_IMPORT) {
        IMAGE_DATA_DIRECTORY d = oh->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
        if (d.VirtualAddress && d.Size) {
            IMAGE_IMPORT_DESCRIPTOR *imp =
                (IMAGE_IMPORT_DESCRIPTOR *)(gExeImage + d.VirtualAddress);
            for (; imp->Name; imp++) {
                const char *depname = (const char *)(gExeImage + imp->Name);

                DLL_ENTRY *dep = DllFindByName(depname);
                if (!dep) {
                    char dp[DLL_PATH_MAX];
                    uint32_t p = 0;
                    const char *pre = "\\NocturnalOS\\system\\libs\\";
                    while (pre[p] && p + 1 < DLL_PATH_MAX) { dp[p] = pre[p]; p++; }
                    uint32_t j = 0;
                    while (depname[j] && p + 1 < DLL_PATH_MAX) dp[p++] = depname[j++];
                    const char *ext = ".DLL";
                    j = 0;
                    while (ext[j] && p + 1 < DLL_PATH_MAX) dp[p++] = ext[j++];
                    dp[p] = 0;

                    dep = DllLoad(depname, dp);
                    if (!dep) {
                        KdPrintf("[PE] import dep not found: %s\n", depname);
                        return -11;
                    }
                }

                NTSTATUS st = DllEnsureMapped(dep, kpml4);
                if (!NT_SUCCESS(st)) {
                    KdPrintf("[PE] DllEnsureMapped(%s) failed\n", depname);
                    return -12;
                }

                uint32_t oft = imp->OriginalFirstThunk ? imp->OriginalFirstThunk
                                                        : imp->FirstThunk;
                uint64_t *orig = (uint64_t *)(gExeImage + oft);
                uint64_t *iat  = (uint64_t *)(gExeImage + imp->FirstThunk);
                for (int k = 0; orig[k]; k++) {
                    uint64_t v = orig[k];
                    if (v & IMAGE_ORDINAL_FLAG64) {
                        KdPrintf("[PE] ordinal import unsupported: %s\n", depname);
                        return -13;
                    }
                    IMAGE_IMPORT_BY_NAME *ibn =
                        (IMAGE_IMPORT_BY_NAME *)(gExeImage + (uint32_t)v);
                    void *addr = DllGetProcAddress(dep, ibn->Name);
                    if (!addr) {
                        KdPrintf("[PE] missing export: %s!%s\n",
                                 depname, ibn->Name);
                        return -14;
                    }
                    iat[k] = (uint64_t)(uintptr_t)addr;
                }
            }
        }
    }

    uint32_t pages = (oh->SizeOfImage + 4095) / 4096;
    for (uint32_t i = 0; i < pages; i++) {
        void *phys = PmmAllocatePage();
        if (!phys) { KdPrint("[PE] phys alloc failed\n"); return -15; }
        MemCopy(phys, gExeImage + i * 4096, 4096);

        uint64_t va = base + (uint64_t)i * 4096;
        if (MiMapPage(kpml4, va, (uint64_t)(uintptr_t)phys, 1 | 2 | 4) != 0) {
            KdPrintf("[PE] MiMapPage failed va=0x%llx\n", (unsigned long long)va);
            return -16;
        }
    }

    Out->EntryPoint     = base + oh->AddressOfEntryPoint;
    Out->ImageBase      = base;
    Out->ImageSize      = oh->SizeOfImage;
    Out->SectionsLoaded = fh->NumberOfSections;

    KdPrintf("[PE] loaded entry=0x%llx base=0x%llx size=0x%x pages=%u\n",
             (unsigned long long)Out->EntryPoint,
             (unsigned long long)Out->ImageBase,
             (unsigned)oh->SizeOfImage, pages);
    return 0;
}