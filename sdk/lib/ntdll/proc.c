#include "internal.h"

NTSYSAPI 
NTSTATUS 
    NTAPI
NtCreateProcess(const char *Path, const char *CmdLine, PHANDLE ProcessHandle)
{
    return NT_SYSCALL3(SYS_NT_CREATE_PROCESS, Path, CmdLine, ProcessHandle);
}

NTSYSAPI 
NTSTATUS 
NTAPI
NtTerminateProcess(HANDLE ProcessHandle, LONG ExitStatus)
{
    return NT_SYSCALL2(SYS_NT_TERMINATE_PROCESS,
                       ProcessHandle, (ULONG64)(LONG64)ExitStatus);
}