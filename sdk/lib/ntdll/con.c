#include "internal.h"
#include <RTL/rt.h>

NTSYSAPI NTSTATUS NTAPI
NtDisplayString(const char *String)
{
    return NT_SYSCALL1(SYS_NT_DISPLAY_STRING, String);
}

NTSYSAPI NTSTATUS NTAPI
NtWriteConsole(const char *String)
{
    return NT_SYSCALL1(SYS_NT_WRITE_CONSOLE, String);
}

NTSYSAPI NTSTATUS NTAPI
NtReadConsole(char *Buf, ULONG MaxLen)
{
    return NT_SYSCALL2(SYS_NT_READ_CONSOLE, Buf, MaxLen);
}

NTSYSAPI ULONG NTAPI
NtGetTickCount(void)
{
    return (ULONG)NtSyscall(SYS_NT_GET_TICK_COUNT, 0,0,0, 0,0,0, 0,0,0);
}