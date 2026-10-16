#include "internal.h"

NTSYSAPI NTSTATUS NTAPI
NtCreateMutant(PHANDLE MutantHandle, BOOLEAN InitialOwner, const char *Name)
{
    return NT_SYSCALL3(SYS_NT_CREATE_MUTANT,
                       MutantHandle, (ULONG64)InitialOwner, Name);
}

NTSYSAPI NTSTATUS NTAPI
NtOpenMutant(PHANDLE MutantHandle, const char *Name)
{
    return NT_SYSCALL2(SYS_NT_OPEN_MUTANT, MutantHandle, Name);
}

NTSYSAPI NTSTATUS NTAPI
NtReleaseMutant(HANDLE MutantHandle, PLONG PreviousCount)
{
    return NT_SYSCALL2(SYS_NT_RELEASE_MUTANT, MutantHandle, PreviousCount);
}

NTSYSAPI NTSTATUS NTAPI
NtCreateEvent(PHANDLE EventHandle, ULONG EventType, BOOLEAN InitialState,
              const char *Name)
{
    return NT_SYSCALL4(SYS_NT_CREATE_EVENT,
                       EventHandle, EventType, (ULONG64)InitialState, Name);
}

NTSYSAPI NTSTATUS NTAPI
NtOpenEvent(PHANDLE EventHandle, const char *Name)
{
    return NT_SYSCALL2(SYS_NT_OPEN_EVENT, EventHandle, Name);
}

NTSYSAPI NTSTATUS NTAPI
NtSetEvent(HANDLE EventHandle, PLONG PreviousState)
{
    return NT_SYSCALL2(SYS_NT_SET_EVENT, EventHandle, PreviousState);
}

NTSYSAPI NTSTATUS NTAPI
NtResetEvent(HANDLE EventHandle, PLONG PreviousState)
{
    return NT_SYSCALL2(SYS_NT_RESET_EVENT, EventHandle, PreviousState);
}

NTSYSAPI NTSTATUS NTAPI
NtPulseEvent(HANDLE EventHandle, PLONG PreviousState)
{
    return NT_SYSCALL2(SYS_NT_PULSE_EVENT, EventHandle, PreviousState);
}

NTSYSAPI NTSTATUS NTAPI
NtCreateSemaphore(PHANDLE SemaphoreHandle, LONG InitialCount,
                  LONG MaximumCount, const char *Name)
{
    return NT_SYSCALL4(SYS_NT_CREATE_SEMAPHORE,
                       SemaphoreHandle, (ULONG64)InitialCount,
                       (ULONG64)MaximumCount, Name);
}

NTSYSAPI NTSTATUS NTAPI
NtOpenSemaphore(PHANDLE SemaphoreHandle, const char *Name)
{
    return NT_SYSCALL2(SYS_NT_OPEN_SEMAPHORE, SemaphoreHandle, Name);
}

NTSYSAPI NTSTATUS NTAPI
NtReleaseSemaphore(HANDLE SemaphoreHandle, LONG ReleaseCount,
                   PLONG PreviousCount)
{
    return NT_SYSCALL3(SYS_NT_RELEASE_SEMAPHORE,
                       SemaphoreHandle, (ULONG64)ReleaseCount, PreviousCount);
}

NTSYSAPI NTSTATUS NTAPI
NtWaitForSingleObject(HANDLE Handle, ULONG TimeoutMs)
{
    return NT_SYSCALL2(SYS_NT_WAIT_FOR_SINGLE_OBJECT, Handle, TimeoutMs);
}

NTSYSAPI NTSTATUS NTAPI
NtDelayExecution(ULONG Ms)
{
    return NT_SYSCALL1(SYS_NT_DELAY_EXECUTION, Ms);
}