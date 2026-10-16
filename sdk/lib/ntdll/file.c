#include "internal.h"

NTSYSAPI 
NTSTATUS 
NTAPI
NtReadFile(const char *Path, void *Buffer, ULONG Length)
{
    return NT_SYSCALL3(SYS_NT_READ_FILE, Path, Buffer, Length);
}

NTSYSAPI 
NTSTATUS 
NTAPI
NtQueryDirectory(const char *Path, void *Buffer, ULONG MaxEntries,
                 PULONG CountOut)
{
    return NT_SYSCALL4(SYS_NT_QUERY_DIRECTORY,
                       Path, Buffer, MaxEntries, CountOut);
}