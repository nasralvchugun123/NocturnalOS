#include "internal.h"

NTSYSAPI NTSTATUS NTAPI
NtClose(HANDLE Handle)
{
    return NT_SYSCALL1(SYS_NT_CLOSE, Handle);
}
