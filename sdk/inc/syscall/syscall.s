.intel_syntax noprefix
    .text
    .global NtSyscall

/*  ULONG64 NtSyscall(ULONG64 nr,
 *                     ULONG64 a1, ULONG64 a2, ULONG64 a3,
 *                     ULONG64 a4, ULONG64 a5, ULONG64 a6);
 *
 *  msabi:  rcx=nr, rdx=a1, r8=a2, r9=a3,
 *          [rsp+40]=a4, [rsp+48]=a5, [rsp+56]=a6
 */
NtSyscall:
    push    rdi
    push    rsi

    mov     rax, rcx            /* nr */
    mov     rdi, rdx            /* a1 */
    mov     rsi, r8             /* a2 */
    mov     rdx, r9             /* a3 */
    mov     r10, [rsp + 56]     /* a4 */
    mov     r8,  [rsp + 64]     /* a5 */
    mov     r9,  [rsp + 72]     /* a6 */
    syscall

    pop     rsi
    pop     rdi
    ret