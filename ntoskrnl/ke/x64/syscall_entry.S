.section .text
.global KiSystemCallEntry
KiSystemCallEntry:
    swapgs                              /* %gs: user TEB -> KCPU */

    movq %rsp, %gs:8                    
    movq %gs:16, %rsp                   

    pushq $0x1B                         /* SS user    */
    pushq %gs:8                         /* RSP user   */
    pushq %r11                          /* RFLAGS user*/
    pushq $0x23                         /* CS user    */
    pushq %rcx                          /* RIP user   */
    pushq $0                            /* error      */
    pushq $0x80                         /* vector     */
    pushq %rax                          /* syscall nr */

    pushq %rbx
    pushq %rcx
    pushq %rdx
    pushq %rsi
    pushq %rdi
    pushq %rbp
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    movq %rsp, %rcx
    call KiSystemCall

    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rbp
    popq %rdi
    popq %rsi
    popq %rdx
    popq %rcx
    popq %rbx
    popq %rax                           /* return value */
    addq $16, %rsp                      /* skip vector + error */

    swapgs                              /* KCPU -> user TEB */
    iretq