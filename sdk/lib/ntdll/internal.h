#ifndef _NTDLL_INTERNAL_H
#define _NTDLL_INTERNAL_H

#include <nt/ntdefs.h>
#include <nt/ntstatus.h>
#include <nt/syscall.h>

extern ULONG64 NtSyscall(ULONG64 nr,
                         ULONG64 a1, ULONG64 a2, ULONG64 a3,
                         ULONG64 a4, ULONG64 a5, ULONG64 a6,
                         ULONG64 a7, ULONG64 a8, ULONG64 a9);

#define NT_SYSCALL0(nr) \
    ((NTSTATUS)NtSyscall((ULONG64)(nr), 0,0,0, 0,0,0, 0,0,0))

#define NT_SYSCALL1(nr, a1) \
    ((NTSTATUS)NtSyscall((ULONG64)(nr), (ULONG64)(a1), 0,0, 0,0,0, 0,0,0))

#define NT_SYSCALL2(nr, a1,a2) \
    ((NTSTATUS)NtSyscall((ULONG64)(nr), (ULONG64)(a1),(ULONG64)(a2), 0, 0,0,0, 0,0,0))

#define NT_SYSCALL3(nr, a1,a2,a3) \
    ((NTSTATUS)NtSyscall((ULONG64)(nr), (ULONG64)(a1),(ULONG64)(a2),(ULONG64)(a3), 0,0,0, 0,0,0))

#define NT_SYSCALL4(nr, a1,a2,a3,a4) \
    ((NTSTATUS)NtSyscall((ULONG64)(nr), (ULONG64)(a1),(ULONG64)(a2),(ULONG64)(a3), \
                         (ULONG64)(a4), 0,0, 0,0,0))

#define NT_SYSCALL5(nr, a1,a2,a3,a4,a5) \
    ((NTSTATUS)NtSyscall((ULONG64)(nr), (ULONG64)(a1),(ULONG64)(a2),(ULONG64)(a3), \
                         (ULONG64)(a4),(ULONG64)(a5), 0, 0,0,0))

#define NT_SYSCALL6(nr, a1,a2,a3,a4,a5,a6) \
    ((NTSTATUS)NtSyscall((ULONG64)(nr), (ULONG64)(a1),(ULONG64)(a2),(ULONG64)(a3), \
                         (ULONG64)(a4),(ULONG64)(a5),(ULONG64)(a6), 0,0,0))

#define NT_SYSCALL7(nr, a1,a2,a3,a4,a5,a6,a7) \
    ((NTSTATUS)NtSyscall((ULONG64)(nr), (ULONG64)(a1),(ULONG64)(a2),(ULONG64)(a3), \
                         (ULONG64)(a4),(ULONG64)(a5),(ULONG64)(a6), \
                         (ULONG64)(a7), 0,0))

#define NT_SYSCALL8(nr, a1,a2,a3,a4,a5,a6,a7,a8) \
    ((NTSTATUS)NtSyscall((ULONG64)(nr), (ULONG64)(a1),(ULONG64)(a2),(ULONG64)(a3), \
                         (ULONG64)(a4),(ULONG64)(a5),(ULONG64)(a6), \
                         (ULONG64)(a7),(ULONG64)(a8), 0))

#define NT_SYSCALL9(nr, a1,a2,a3,a4,a5,a6,a7,a8,a9) \
    ((NTSTATUS)NtSyscall((ULONG64)(nr), (ULONG64)(a1),(ULONG64)(a2),(ULONG64)(a3), \
                         (ULONG64)(a4),(ULONG64)(a5),(ULONG64)(a6), \
                         (ULONG64)(a7),(ULONG64)(a8),(ULONG64)(a9)))
#endif