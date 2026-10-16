.section .text
.balign 32

.global TrampolineTemplate
.global TrampolineTemplateEnd

/* Layout от 0x8000:
 *   0x000  EB 2E   
 *   0x008  CR3
 *   0x010  RSP
 *   0x018  RDI
 *   0x020  ENTRY
 *   0x030  16-bit real mode
 *   0x080  32-bit protected
 *   0x100  64-bit long mode
 *   0x180  GDT
 *   0x1A0  GDTR
 *   0x1B0  end
 */

TrampolineTemplate:
    .byte 0xEB, 0x2E

    .org 0x08
    .quad 0                             /* CR3   */
    .quad 0                             /* RSP   */
    .quad 0                             /* RDI   */
    .quad 0                             /* ENTRY */

    .org 0x30
.code16
TrampReal:
    cli
    cld
    mov     $0x03F8, %dx
    mov     $0x41, %al
    out     %al, %dx

    xor     %ax, %ax
    mov     %ax, %ds
    mov     %ax, %es
    mov     %ax, %ss

    lgdt    0x81A0

    mov     %cr0, %eax
    or      $1, %eax
    mov     %eax, %cr0
    ljmp    $0x08, $0x8080
    .org 0x80

.code32
TrampProt:
    mov     $0x10, %ax
    mov     %ax, %ds
    mov     %ax, %es
    mov     %ax, %ss
    mov     %ax, %fs
    mov     %ax, %gs

    mov     $0x03F8, %dx
    mov     $0x42, %al
    out     %al, %dx

    mov     %cr4, %eax
    or      $0x20, %eax
    mov     %eax, %cr4

    mov     0x8008, %eax
    mov     %eax, %cr3

    mov     $0xC0000080, %ecx
    rdmsr
    or      $0x100, %eax
    wrmsr

    mov     %cr0, %eax
    or      $0x80000000, %eax
    mov     %eax, %cr0

    ljmp    $0x18, $0x8100
    .org 0x100

.code64
TrampLong:
    mov     $0x03F8, %dx
    mov     $0x43, %al
    out     %al, %dx

    mov     $0x8010, %r11
    mov     (%r11), %rsp

    mov     $0x8018, %r11
    mov     (%r11), %rdi

    mov     $0x8020, %r11
    mov     (%r11), %rax

    push    %rax
    mov     $0x03F8, %dx
    mov     $0x44, %al
    out     %al, %dx
    pop     %rax

    jmp     *%rax

    .org 0x180
    .quad 0x0000000000000000
    .quad 0x00CF9A000000FFFF
    .quad 0x00CF92000000FFFF
    .quad 0x00AF9A000000FFFF

    .org 0x1A0
    .word 0x001F
    .long 0x00008180

TrampolineTemplateEnd: