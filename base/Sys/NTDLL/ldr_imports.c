#include "NTdll.h"
#include <nt/pe.h>
#include <rt.h>

NTSTATUS LdrpCallDllMain(PVOID Base, PVOID Entry, ULONG Reason, PVOID Reserved)
{
    typedef BOOLEAN (*DllMainFn)(PVOID, ULONG, PVOID);
    if (!Entry) return STATUS_SUCCESS;
    DllMainFn fn = (DllMainFn)Entry;
    if (!fn((PVOID)Base, Reason, Reserved))
        return STATUS_UNSUCCESSFUL;
    return STATUS_SUCCESS;
}

NTSTATUS LdrpResolveImports(PVOID ImageBase)
{
    uint8_t *img = (uint8_t *)ImageBase;
    IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)img;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return STATUS_INVALID_IMAGE_FORMAT;

    IMAGE_NT_HEADERS64 *nt = (IMAGE_NT_HEADERS64 *)(img + dos->e_lfanew);
    IMAGE_OPTIONAL_HEADER64 *oh = &nt->OptionalHeader;
    if (oh->NumberOfRvaAndSizes <= IMAGE_DIRECTORY_ENTRY_IMPORT)
        return STATUS_SUCCESS;
    IMAGE_DATA_DIRECTORY d = oh->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (!d.VirtualAddress || !d.Size) return STATUS_SUCCESS;

    IMAGE_IMPORT_DESCRIPTOR *imp =
        (IMAGE_IMPORT_DESCRIPTOR *)(img + d.VirtualAddress);
    for (; imp->Name; imp++) {
        const char *depname = (const char *)(img + imp->Name);

        PLDR_DATA_TABLE_ENTRY dep = LdrpFindByName(depname);
        if (!dep) {
            UNICODE_STRING us;
            WCHAR wbuf[64];
            uint32_t j = 0;
            while (depname[j] && j < 63) {
                wbuf[j] = (WCHAR)(uint8_t)depname[j]; j++;
            }
            wbuf[j] = 0;
            us.Buffer = wbuf;
            us.Length = (USHORT)(j * 2);
            us.MaximumLength = (USHORT)((j + 1) * 2);

            HANDLE h = 0;
            NTSTATUS st = LdrLoadDll(0, 0, &us, &h);
            if (!NT_SUCCESS(st)) return st;
            dep = LdrpFindByName(depname);
            if (!dep) return STATUS_UNSUCCESSFUL;
        }

        uint32_t oft = imp->OriginalFirstThunk ? imp->OriginalFirstThunk
                                               : imp->FirstThunk;
        uint64_t *orig = (uint64_t *)(img + oft);
        uint64_t *iat  = (uint64_t *)(img + imp->FirstThunk);

        for (int k = 0; orig[k]; k++) {
            uint64_t v = orig[k];
            if (v & IMAGE_ORDINAL_FLAG64) return STATUS_NOT_IMPLEMENTED;
            IMAGE_IMPORT_BY_NAME *ibn =
                (IMAGE_IMPORT_BY_NAME *)(img + (uint32_t)v);
            ANSI_STRING as;
            as.Buffer = ibn->Name;
            as.Length = 0;
            while (ibn->Name[as.Length]) as.Length++;
            as.MaximumLength = as.Length + 1;

            PVOID addr = 0;
            NTSTATUS st = LdrGetProcedureAddress((HANDLE)dep->DllBase,
                                                 &as, 0, &addr);
            if (!NT_SUCCESS(st)) return st;
            iat[k] = (uint64_t)(uintptr_t)addr;
        }
    }
    return STATUS_SUCCESS;
}