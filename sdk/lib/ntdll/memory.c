#include "internal.h"

NTSYSAPI NTSTATUS NTAPI
NtAllocateVirtualMemory(PVOID *BaseAddress, ULONG64 ZeroBits,
                        ULONG64 *RegionSize, ULONG AllocationType,
                        ULONG Protect)
{
    return NT_SYSCALL5(SYS_NT_ALLOCATE_VM,
                       BaseAddress, ZeroBits, RegionSize,
                       AllocationType, Protect);
}

NTSYSAPI NTSTATUS NTAPI
NtFreeVirtualMemory(PVOID *BaseAddress, ULONG64 *RegionSize, ULONG FreeType)
{
    return NT_SYSCALL3(SYS_NT_FREE_VM, BaseAddress, RegionSize, FreeType);
}

NTSYSAPI NTSTATUS NTAPI
NtProtectVirtualMemory(PVOID *BaseAddress, ULONG64 *RegionSize,
                       ULONG NewProtect, PULONG OldProtect)
{
    return NT_SYSCALL4(SYS_NT_PROTECT_VM,
                       BaseAddress, RegionSize, NewProtect, OldProtect);
}