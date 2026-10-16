#include "NTdll.h"
#include <nt/pe.h>
#include <rt.h>

static uint8_t gFile[4 * 1024 * 1024];
static uint8_t gImage[4 * 1024 * 1024];

static void MemZeroLocal(void *d, uint64_t n)
{ uint8_t *p = d; while (n--) *p++ = 0; }

static void MemCopyLocal(void *d, const void *s, uint64_t n)
{ uint8_t *dp = d; const uint8_t *sp = s; while (n--) *dp++ = *sp++; }

NTSTATUS LdrpParseAndCreate(const char *Path,
                            PVOID *BaseOut, ULONG *SizeOut,
                            PVOID *EntryOut, PLDR_DATA_TABLE_ENTRY *EntryRecOut)
{
    int n = (int)NtReadFile(Path, gFile, sizeof(gFile));
    if (n <= 0) return STATUS_OBJECT_NAME_NOT_FOUND;

    IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)gFile;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return STATUS_INVALID_IMAGE_FORMAT;

    uint8_t *nt = gFile + dos->e_lfanew;
    if (*(uint32_t *)nt != IMAGE_NT_SIGNATURE) return STATUS_INVALID_IMAGE_FORMAT;

    IMAGE_FILE_HEADER *fh = (IMAGE_FILE_HEADER *)(nt + 4);
    IMAGE_OPTIONAL_HEADER64 *oh =
        (IMAGE_OPTIONAL_HEADER64 *)((uint8_t *)fh + sizeof(IMAGE_FILE_HEADER));
    if (oh->Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
        return STATUS_INVALID_IMAGE_FORMAT;
    if (oh->SizeOfImage > sizeof(gImage))
        return STATUS_INVALID_IMAGE_FORMAT;

    IMAGE_SECTION_HEADER *sh =
        (IMAGE_SECTION_HEADER *)((uint8_t *)oh + fh->SizeOfOptionalHeader);

    MemZeroLocal(gImage, oh->SizeOfImage);
    uint32_t hdrsz = oh->SizeOfHeaders;
    if (hdrsz > (uint32_t)n) hdrsz = (uint32_t)n;
    if (hdrsz > oh->SizeOfImage) hdrsz = oh->SizeOfImage;
    MemCopyLocal(gImage, gFile, hdrsz);

    for (uint16_t i = 0; i < fh->NumberOfSections; i++) {
        IMAGE_SECTION_HEADER *s = &sh[i];
        if (!s->SizeOfRawData) continue;
        if ((uint64_t)s->PointerToRawData + s->SizeOfRawData > (uint64_t)n)
            return STATUS_INVALID_IMAGE_FORMAT;
        if ((uint64_t)s->VirtualAddress + s->SizeOfRawData > oh->SizeOfImage)
            return STATUS_INVALID_IMAGE_FORMAT;
        MemCopyLocal(gImage + s->VirtualAddress,
                     gFile + s->PointerToRawData, s->SizeOfRawData);
    }

    const char *name = Path;
    for (const char *p = Path; *p; p++)
        if (*p == '\\' || *p == '/') name = p + 1;

    PVOID base = 0;
    NTSTATUS st = NtCreateImageSection(name, gImage, oh->SizeOfImage,
                                       oh->ImageBase, &base);
    if (!NT_SUCCESS(st)) return st;

    PLDR_DATA_TABLE_ENTRY rec = LdrpAllocEntry();
    if (!rec) return STATUS_INSUFFICIENT_RESOURCES;
    rec->DllBase = base;
    rec->EntryPoint = (PVOID)((uint8_t *)base + oh->AddressOfEntryPoint);
    rec->SizeOfImage = oh->SizeOfImage;
    rec->ObsoleteLoadCount = 1;

    WCHAR *wbuf = (WCHAR *)LdrpHeapAlloc(64 * sizeof(WCHAR));
    if (!wbuf) return STATUS_NO_MEMORY;
    LdrpStrToUnicode(&rec->BaseDllName, wbuf, 64, name);

    WCHAR *wfull = (WCHAR *)LdrpHeapAlloc(128 * sizeof(WCHAR));
    if (!wfull) return STATUS_NO_MEMORY;
    LdrpStrToUnicode(&rec->FullDllName, wfull, 128, Path);

    *BaseOut = base;
    *SizeOut = oh->SizeOfImage;
    *EntryOut = rec->EntryPoint;
    *EntryRecOut = rec;
    return STATUS_SUCCESS;
}