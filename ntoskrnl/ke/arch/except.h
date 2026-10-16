
#ifndef KE_ARCH_EXCEPT_H
#define KE_ARCH_EXCEPT_H

#include <stdint.h>

typedef struct {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t vector;
    uint64_t error;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;    
    uint64_t ss;    
} INTERRUPT_FRAME;

void KiIsrCommon(INTERRUPT_FRAME *Frame);

#endif