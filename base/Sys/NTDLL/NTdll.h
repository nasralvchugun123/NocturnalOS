#ifndef _NTDLL_USER_H
#define _NTDLL_USER_H

#include <stdint.h>
#include <nt/ntdefs.h>
#include <nt/ntstatus.h>
#include <nt/ntdll.h>

typedef struct _LDRP_CTX {
    PPEB                  Peb;
    PLDR_DATA_TABLE_ENTRY MainExe;
    PLDR_DATA_TABLE_ENTRY Ntdll;
    LDR_DATA_TABLE_ENTRY  Entries[64];
    uint32_t              EntryCount;
} LDRP_CTX;

extern LDRP_CTX gLdr;

void LdrpEnsurePeb(void);
void *LdrpHeapAlloc(uint64_t Size);
void  LdrpHeapFree(void *Ptr);
void  LdrpAppendEntry(PLDR_DATA_TABLE_ENTRY E);

PLDR_DATA_TABLE_ENTRY LdrpFindByName(const char *Name);
PLDR_DATA_TABLE_ENTRY LdrpAllocEntry(void);

void LdrpStrToUnicode(UNICODE_STRING *us, WCHAR *buf, uint32_t cap,
                      const char *src);

NTSTATUS LdrpParseAndCreate(const char *Path,
                            PVOID *BaseOut, ULONG *SizeOut,
                            PVOID *EntryOut, PLDR_DATA_TABLE_ENTRY *EntryRecOut);

NTSTATUS LdrpResolveImports(PVOID ImageBase);

NTSTATUS LdrpCallDllMain(PVOID Base, PVOID Entry, ULONG Reason, PVOID Reserved);

#endif /* _NTDLL_USER_H */