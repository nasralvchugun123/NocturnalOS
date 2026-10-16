#ifndef _NTDLL_H
#define _NTDLL_H

#include "ntdefs.h"
#include "ntstatus.h"
#include "syscall.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NotificationEvent     0
#define SynchronizationEvent  1

#define INVALID_HANDLE_VALUE  ((HANDLE)(LONG_PTR)-1)
NTSYSAPI NTSTATUS NTAPI NtDisplayString(const char *String);
NTSYSAPI NTSTATUS NTAPI NtWriteConsole(const char *String);
NTSYSAPI NTSTATUS NTAPI NtReadConsole(char *Buf, ULONG MaxLen);
NTSYSAPI ULONG    NTAPI NtGetTickCount(void);

NTSYSAPI NTSTATUS NTAPI NtAllocateVirtualMemory(PVOID *BaseAddress,
                                                ULONG64 ZeroBits,
                                                ULONG64 *RegionSize,
                                                ULONG AllocationType,
                                                ULONG Protect);

NTSYSAPI NTSTATUS NTAPI NtFreeVirtualMemory(PVOID *BaseAddress,
                                            ULONG64 *RegionSize,
                                            ULONG FreeType);

NTSYSAPI NTSTATUS NTAPI NtCreateEvent(PHANDLE EventHandle,
                                      ULONG EventType,
                                      BOOLEAN InitialState,
                                      const char *Name);
NTSYSAPI NTSTATUS NTAPI NtOpenEvent(PHANDLE EventHandle, const char *Name);
NTSYSAPI NTSTATUS NTAPI NtSetEvent(HANDLE EventHandle, PLONG PreviousState);
NTSYSAPI NTSTATUS NTAPI NtResetEvent(HANDLE EventHandle, PLONG PreviousState);
NTSYSAPI NTSTATUS NTAPI NtPulseEvent(HANDLE EventHandle, PLONG PreviousState);

NTSYSAPI NTSTATUS NTAPI NtCreateMutant(PHANDLE MutantHandle,
                                       BOOLEAN InitialOwner,
                                       const char *Name);
NTSYSAPI NTSTATUS NTAPI NtOpenMutant(PHANDLE MutantHandle, const char *Name);
NTSYSAPI NTSTATUS NTAPI NtReleaseMutant(HANDLE MutantHandle,
                                        PLONG PreviousCount);

NTSYSAPI NTSTATUS NTAPI NtCreateSemaphore(PHANDLE SemaphoreHandle,
                                          LONG InitialCount,
                                          LONG MaximumCount,
                                          const char *Name);
NTSYSAPI NTSTATUS NTAPI NtOpenSemaphore(PHANDLE SemaphoreHandle,
                                        const char *Name);
NTSYSAPI NTSTATUS NTAPI NtReleaseSemaphore(HANDLE SemaphoreHandle,
                                           LONG ReleaseCount,
                                           PLONG PreviousCount);

NTSYSAPI NTSTATUS NTAPI NtWaitForSingleObject(HANDLE Handle, ULONG TimeoutMs);
NTSYSAPI NTSTATUS NTAPI NtDelayExecution(ULONG Ms);

NTSYSAPI NTSTATUS NTAPI NtClose(HANDLE Handle);

NTSYSAPI NTSTATUS NTAPI NtReadFile(const char *Path, void *Buffer, ULONG Length);
NTSYSAPI NTSTATUS NTAPI NtQueryDirectory(const char *Path, void *Buffer,
                                         ULONG MaxEntries, PULONG CountOut);

NTSYSAPI NTSTATUS NTAPI NtCreateProcess(const char *Path,
                                        const char *CmdLine,
                                        PHANDLE ProcessHandle);
NTSYSAPI NTSTATUS NTAPI NtTerminateProcess(HANDLE ProcessHandle,
                                           LONG ExitStatus);

NTSYSAPI NTSTATUS NTAPI LdrLoadDll(PCWSTR PathToFile, PULONG Flags,
                                   PUNICODE_STRING ModuleFileName,
                                   PHANDLE ModuleHandle);
NTSYSAPI NTSTATUS NTAPI LdrGetProcedureAddress(HANDLE ModuleHandle,
                                               PANSI_STRING ProcedureName,
                                               ULONG ProcedureNumber,
                                               PVOID *ProcedureAddress);
NTSYSAPI NTSTATUS NTAPI LdrUnloadDll(HANDLE ModuleHandle);
NTSYSAPI VOID     NTAPI LdrShutdownProcess(VOID);
NTSYSAPI VOID     NTAPI LdrShutdownThread(VOID);

NTSYSAPI NTSTATUS NTAPI NtCreateImageSection(const char *Name,
                                             PVOID FlattenedImage,
                                             ULONG64 Size,
                                             ULONG64 PreferredBase,
                                             PVOID *ActualBaseOut);
NTSYSAPI NTSTATUS NTAPI NtMapImageSection(const char *Name, PVOID *BaseOut);
NTSYSAPI NTSTATUS NTAPI NtUnmapImageSection(const char *Name);
NTSYSAPI NTSTATUS NTAPI NtQueryImageSection(const char *Name,
                                            PVOID *BaseOut, ULONG64 *SizeOut);
NTSYSAPI NTSTATUS NTAPI NtProtectVirtualMemory(PVOID *BaseAddress,
                                               ULONG64 *RegionSize,
                                               ULONG NewProtect,
                                               PULONG OldProtect);
NTSYSAPI NTSTATUS NTAPI NtCurrentPeb(PPEB *PebOut);
NTSYSAPI NTSTATUS NTAPI NtGetModuleHandle(const char *Name, PVOID *BaseOut);

#ifdef __cplusplus
}
#endif
#endif /* _NTDLL_H */