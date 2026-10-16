#include "internal.h"

NTSYSAPI NTSTATUS NTAPI
NtCreateImageSection(const char *Name, PVOID FlattenedImage,
                     ULONG64 Size, ULONG64 PreferredBase, PVOID *ActualBaseOut)
{
    return NT_SYSCALL5(SYS_NT_CREATE_IMAGE_SECTION,
                       Name, FlattenedImage, Size, PreferredBase, ActualBaseOut);
}

NTSYSAPI NTSTATUS NTAPI
NtMapImageSection(const char *Name, PVOID *BaseOut)
{
    return NT_SYSCALL2(SYS_NT_MAP_IMAGE_SECTION, Name, BaseOut);
}

NTSYSAPI NTSTATUS NTAPI
NtUnmapImageSection(const char *Name)
{
    return NT_SYSCALL1(SYS_NT_UNMAP_IMAGE_SECTION, Name);
}

NTSYSAPI NTSTATUS NTAPI
NtQueryImageSection(const char *Name, PVOID *BaseOut, ULONG64 *SizeOut)
{
    return NT_SYSCALL3(SYS_NT_QUERY_IMAGE_SECTION, Name, BaseOut, SizeOut);
}

NTSYSAPI NTSTATUS NTAPI
NtCurrentPeb(PPEB *PebOut)
{
    return NT_SYSCALL1(SYS_NT_CURRENT_PEB, PebOut);
}

NTSYSAPI NTSTATUS NTAPI
NtGetModuleHandle(const char *Name, PVOID *BaseOut)
{
    return NT_SYSCALL2(SYS_NT_GET_MODULE_HANDLE, Name, BaseOut);
}