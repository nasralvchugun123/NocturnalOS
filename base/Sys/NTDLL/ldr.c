#include "NTdll.h"
#include <rt.h>
#include <nt/pe.h>

void LdrpEnsurePeb(void)
{
    if (gLdr.Peb) return;
    void *peb;
    __asm__ volatile ("movq %%gs:0x60, %0" : "=r"(peb));
    gLdr.Peb = (PPEB)peb;
}

void LdrpStrToUnicode(UNICODE_STRING *us, WCHAR *buf, uint32_t cap,
                      const char *src)
{
    uint32_t i = 0;
    while (src[i] && i + 1 < cap) { buf[i] = (WCHAR)(uint8_t)src[i]; i++; }
    buf[i] = 0;
    us->Buffer = buf;
    us->Length = (USHORT)(i * 2);
    us->MaximumLength = (USHORT)((i + 1) * 2);
}

void LdrpAppendEntry(PLDR_DATA_TABLE_ENTRY E)
{
    LdrpEnsurePeb();
    PPEB_LDR_DATA Ldr = gLdr.Peb->Ldr;
    LIST_ENTRY *tail;

    tail = Ldr->InLoadOrderModuleList.Blink;
    E->InLoadOrderLinks.Flink = &Ldr->InLoadOrderModuleList;
    E->InLoadOrderLinks.Blink = tail;
    tail->Flink = &E->InLoadOrderLinks;
    Ldr->InLoadOrderModuleList.Blink = &E->InLoadOrderLinks;

    tail = Ldr->InMemoryOrderModuleList.Blink;
    E->InMemoryOrderLinks.Flink = &Ldr->InMemoryOrderModuleList;
    E->InMemoryOrderLinks.Blink = tail;
    tail->Flink = &E->InMemoryOrderLinks;
    Ldr->InMemoryOrderModuleList.Blink = &E->InMemoryOrderLinks;

    tail = Ldr->InInitializationOrderModuleList.Blink;
    E->InInitializationOrderLinks.Flink = &Ldr->InInitializationOrderModuleList;
    E->InInitializationOrderLinks.Blink = tail;
    tail->Flink = &E->InInitializationOrderLinks;
    Ldr->InInitializationOrderModuleList.Blink = &E->InInitializationOrderLinks;
}

PLDR_DATA_TABLE_ENTRY LdrpFindByName(const char *Name)
{
    LdrpEnsurePeb();
    if (!gLdr.Peb || !gLdr.Peb->Ldr) return 0;
    LIST_ENTRY *head = &gLdr.Peb->Ldr->InLoadOrderModuleList;
    for (LIST_ENTRY *p = head->Flink; p != head; p = p->Flink) {
        PLDR_DATA_TABLE_ENTRY e =
            (PLDR_DATA_TABLE_ENTRY)((uint8_t *)p -
                __builtin_offsetof(LDR_DATA_TABLE_ENTRY, InLoadOrderLinks));
        if (!e->BaseDllName.Buffer) continue;
        const char *w = (const char *)e->BaseDllName.Buffer;
        const char *a = Name;
        int eq = 1;
        while (*w && *a) {
            char cw = *w; if (cw >= 'A' && cw <= 'Z') cw += 32;
            char ca = *a; if (ca >= 'A' && ca <= 'Z') ca += 32;
            if (cw != ca) { eq = 0; break; }
            w++; a++;
        }
        if (eq && !*w && !*a) return e;
    }
    return 0;
}

PLDR_DATA_TABLE_ENTRY LdrpAllocEntry(void)
{
    if (gLdr.EntryCount >= 64) return 0;
    PLDR_DATA_TABLE_ENTRY e = &gLdr.Entries[gLdr.EntryCount++];
    for (uint32_t i = 0; i < sizeof(*e); i++) ((uint8_t *)e)[i] = 0;
    return e;
}

static int EndsWithDll(const char *p)
{
    int n = 0;
    while (p[n]) n++;
    if (n < 4) return 0;
    return p[n-4] == '.' &&
           (p[n-3] == 'd' || p[n-3] == 'D') &&
           (p[n-2] == 'l' || p[n-2] == 'L') &&
           (p[n-1] == 'l' || p[n-1] == 'L');
}

NTSTATUS NTAPI LdrLoadDll(PCWSTR PathToFile, PULONG Flags,
                          PUNICODE_STRING ModuleFileName,
                          PHANDLE ModuleHandle)
{
    (void)PathToFile; (void)Flags;
    LdrpEnsurePeb();
    if (!ModuleFileName || !ModuleHandle || !ModuleFileName->Buffer)
        return STATUS_INVALID_PARAMETER;

    char ascii[512];
    uint32_t len = ModuleFileName->Length / 2;
    if (len >= sizeof(ascii) - 1) return STATUS_INVALID_PARAMETER;
    for (uint32_t i = 0; i < len; i++)
        ascii[i] = (char)(uint8_t)ModuleFileName->Buffer[i];
    ascii[len] = 0;

    const char *shortname = ascii;
    for (const char *q = ascii; *q; q++)
        if (*q == '\\' || *q == '/') shortname = q + 1;
    PLDR_DATA_TABLE_ENTRY exist = LdrpFindByName(shortname);
    if (exist) {
        exist->ObsoleteLoadCount++;
        *ModuleHandle = (HANDLE)exist->DllBase;
        return STATUS_SUCCESS;
    }

    char path[512];
    uint32_t p = 0;

    if (ascii[0] == '\\' || ascii[0] == '/') {
        for (uint32_t i = 0; ascii[i] && p + 1 < sizeof(path); i++)
            path[p++] = ascii[i];
    } else {
        const char *pre = "\\NocturnalOS\\System\\Core\\Sys64";
        for (uint32_t i = 0; pre[i] && p + 1 < sizeof(path); i++)
            path[p++] = pre[i];
        if (p + 1 < sizeof(path)) path[p++] = '\\';
        for (uint32_t i = 0; ascii[i] && p + 1 < sizeof(path); i++)
            path[p++] = ascii[i];
    }

    if (!EndsWithDll(path)) {
        const char dll[] = ".dll";
        for (uint32_t i = 0; dll[i] && p + 1 < sizeof(path); i++)
            path[p++] = dll[i];
    }
    path[p] = 0;

    PVOID   base  = 0;
    ULONG   size  = 0;
    PVOID   entry = 0;
    PLDR_DATA_TABLE_ENTRY rec = 0;
    NTSTATUS st = LdrpParseAndCreate(path, &base, &size, &entry, &rec);
    if (!NT_SUCCESS(st)) return st;

    LdrpAppendEntry(rec);

    st = LdrpResolveImports(base);
    if (!NT_SUCCESS(st)) return st;

    if (entry) LdrpCallDllMain(base, entry, DLL_PROCESS_ATTACH, 0);

    *ModuleHandle = (HANDLE)base;
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI LdrGetProcedureAddress(HANDLE ModuleHandle,
                                      PANSI_STRING ProcedureName,
                                      ULONG ProcedureNumber,
                                      PVOID *ProcedureAddress)
{
    (void)ProcedureNumber;
    if (!ModuleHandle || !ProcedureName || !ProcedureAddress)
        return STATUS_INVALID_PARAMETER;
    if (!ProcedureName->Buffer) return STATUS_INVALID_PARAMETER;

    uint8_t *img = (uint8_t *)ModuleHandle;
    IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)img;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return STATUS_INVALID_PARAMETER;

    IMAGE_NT_HEADERS64 *nt = (IMAGE_NT_HEADERS64 *)(img + dos->e_lfanew);
    IMAGE_OPTIONAL_HEADER64 *oh = &nt->OptionalHeader;
    if (oh->NumberOfRvaAndSizes <= IMAGE_DIRECTORY_ENTRY_EXPORT)
        return STATUS_NOT_FOUND;

    IMAGE_DATA_DIRECTORY d = oh->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if (!d.VirtualAddress) return STATUS_NOT_FOUND;

    IMAGE_EXPORT_DIRECTORY *ed = (IMAGE_EXPORT_DIRECTORY *)(img + d.VirtualAddress);
    uint32_t *names = (uint32_t *)(img + ed->AddressOfNames);
    uint16_t *ords  = (uint16_t *)(img + ed->AddressOfNameOrdinals);
    uint32_t *fns   = (uint32_t *)(img + ed->AddressOfFunctions);

    const char *want = ProcedureName->Buffer;
    for (uint32_t i = 0; i < ed->NumberOfNames; i++) {
        const char *nm = (const char *)(img + names[i]);
        const char *a = nm, *b = want;
        int eq = 1;
        while (*a && *b) {
            char ca = *a, cb = *b;
            if (ca >= 'A' && ca <= 'Z') ca += 32;
            if (cb >= 'A' && cb <= 'Z') cb += 32;
            if (ca != cb) { eq = 0; break; }
            a++; b++;
        }
        if (eq && !*a && !*b) {
            *ProcedureAddress = img + fns[ords[i]];
            return STATUS_SUCCESS;
        }
    }
    return STATUS_NOT_FOUND;
}

NTSTATUS NTAPI LdrUnloadDll(HANDLE ModuleHandle)
{
    (void)ModuleHandle;
    return STATUS_SUCCESS;
}

void NTAPI LdrShutdownProcess(VOID) { }
void NTAPI LdrShutdownThread(VOID)  { }