.section .text

/* void KiEnterUserMode(uint64_t EntryPoint,     // RCX
 *                      uint64_t UserStackTop,   // RDX
 *                      uint64_t Params);        // R8
 *
 * After iretq in user:
 *   RIP = EntryPoint
 *   RSP = UserStackTop
 *   RCX = Params   <- first arg (msabi)
 *
 * Caller MUST have:
 *   - IA32_KERNEL_GS_BASE = user TEB (via wrmsr)
 *   - interrupts disabled (cli)
 */
.global KiEnterUserMode
KiEnterUserMode:
    mov %rdx, %rax          /* rax = user RSP      */
    mov %rcx, %rdx          /* rdx = entry point   */
    mov %r8,  %rcx          /* rcx = params        */

    mov $0x1B, %r11         /* user SS             */
    mov $0x23, %r10         /* user CS             */
    mov $0x202, %r9  /*0x202       /* RFLAGS: IF=1 | bit1 */

    cli
    push %r11               /* SS      */
    push %rax               /* RSP     */
    push %r9                /* RFLAGS  */
    push %r10               /* CS      */
    push %rdx               /* RIP     */

    swapgs
    iretq