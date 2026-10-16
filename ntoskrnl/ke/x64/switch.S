.section .text

/* void KiSwapContext(KTHREAD *From, KTHREAD *To, uint64_t NewCr3)
 *   %rcx = From, %rdx = To, %r8 = NewCr3
 */
.global KiSwapContext
KiSwapContext:
    push %rbx
    push %rbp
    push %rdi
    push %rsi
    push %r12
    push %r13
    push %r14
    push %r15

    test %rcx, %rcx
    jz 1f
    mov %rsp, 0(%rcx)      /* From->Rsp = rsp */
1:
    test %r8, %r8
    jz 2f
    mov %r8, %cr3
2:
    mov 0(%rdx), %rsp      /* rsp = To->Rsp */

    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %rsi
    pop %rdi
    pop %rbp
    pop %rbx
    ret