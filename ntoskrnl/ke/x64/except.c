#include <dbg/print/kdprint.h>
#include <ds/fb/fb.h>
#include "../arch/except.h"
#include "../arch/ke_arch.h"
#include <rt.h>
#include "../../ke/kbd.h"
#include "../../mm/mm.h"
#include "../../ps/ps.h"
#include <ke/kcpu.h>
#include <halx64/Apic/Apic.h>

extern void HalEndOfInterrupt(uint8_t Irq);
extern void KeUpdateClock(void);

void KiSystemCall(INTERRUPT_FRAME *frame);

static const char *gExcNames[32] = {
    "Divide Error",         "Debug",
    "NMI",                  "Breakpoint",
    "Overflow",             "Bound Range",
    "Invalid Opcode",       "Device Not Available",
    "Double Fault",         "Coprocessor Segment Overrun",
    "Invalid TSS",          "Segment Not Present",
    "Stack Fault",          "General Protection",
    "Page Fault",           "Reserved",
    "x87 FPU",              "Alignment Check",
    "Machine Check",        "SIMD FP",
    "Virtualization",       "Control Protection",
    "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved",
    "Hypervisor Injection", "VMM Communication",
    "Security",             "Reserved"
};

static void dump_frame(INTERRUPT_FRAME *f)
{
    KdPrintf("  vector=%llu error=0x%llx\n",
             (unsigned long long)f->vector, (unsigned long long)f->error);
    KdPrintf("  RIP=0x%llx CS=0x%llx RFLAGS=0x%llx\n",
             (unsigned long long)f->rip, (unsigned long long)f->cs,
             (unsigned long long)f->rflags);
    if ((f->cs & 3) == 3) {
        KdPrintf("  RSP=0x%llx SS=0x%llx\n",
                 (unsigned long long)f->rsp, (unsigned long long)f->ss);
    }
    KdPrintf("  RAX=0x%llx RBX=0x%llx RCX=0x%llx RDX=0x%llx\n",
             (unsigned long long)f->rax, (unsigned long long)f->rbx,
             (unsigned long long)f->rcx, (unsigned long long)f->rdx);
    KdPrintf("  RSI=0x%llx RDI=0x%llx RBP=0x%llx\n",
             (unsigned long long)f->rsi, (unsigned long long)f->rdi,
             (unsigned long long)f->rbp);
    KdPrintf("  R8 =0x%llx R9 =0x%llx R10=0x%llx R11=0x%llx\n",
             (unsigned long long)f->r8, (unsigned long long)f->r9,
             (unsigned long long)f->r10, (unsigned long long)f->r11);
    KdPrintf("  R12=0x%llx R13=0x%llx R14=0x%llx R15=0x%llx\n",
             (unsigned long long)f->r12, (unsigned long long)f->r13,
             (unsigned long long)f->r14, (unsigned long long)f->r15);
}

static void KiPanicException(INTERRUPT_FRAME *f)
{
    const char *name = (f->vector < 32) ? gExcNames[f->vector] : "Unknown";

    KdPrint("\n///////// KERNEL PANIC //////////\n");
    KdPrintf("Exception #%llu: %s\n",
             (unsigned long long)f->vector, name);

    dump_frame(f);

    if (f->vector == 14) {
        uint64_t cr2;
        __asm__ volatile ("mov %%cr2, %0" : "=r"(cr2));
        KdPrintf("  CR2 (fault addr) = 0x%llx\n", (unsigned long long)cr2);
        KdPrintf("  PF reason: %s %s %s\n",
                 (f->error & 1) ? "protection" : "not-present",
                 (f->error & 2) ? "write" : "read",
                 (f->error & 4) ? "user" : "kernel");
    }

    KdPrint("[Ke] halted.\n");
    for (;;) __asm__ volatile ("hlt");
}

static void KiHandleIrq(INTERRUPT_FRAME *f, uint8_t irq)
{
    (void)f;

    HalEndOfInterrupt(irq);

    switch (irq) {
    case 0:
        KeUpdateClock();
        break;
    case 1: {
        uint8_t sc;
        __asm__ volatile ("inb $0x60, %0" : "=a"(sc));
        KbdHandleScancode(sc);
        break;
    }
    default:
        KdPrintf("[Ke] IRQ%u unhandled\n", irq);
        break;
    }
}

static int KiIsUserFault(INTERRUPT_FRAME *f)
{
    if ((f->cs & 3) != 3) return 0;
    switch (f->vector) {
    case 2:   /* NMI */
    case 8:   /* Double Fault */
    case 18:  /* Machine Check */
        return 0;
    default:
        return 1;
    }
}

extern void PsTerminateCurrentThread(void);

static int KiGrowUserStack(uint64_t Cr2)
{
    KTHREAD *t = KeGetCurrentThread();
    if (!t || !t->Process) return 0;
    EPROCESS *p = t->Process;
    if (!p->Pml4Phys || !p->UserStackTop) return 0;

    uint64_t top   = p->UserStackTop;
    uint64_t limit = (top > USER_STACK_MAX) ? (top - USER_STACK_MAX) : 0;
    if (Cr2 >= top || Cr2 < limit) return 0;

    uint64_t va = Cr2 & ~0xFFFULL;
    if (MiIsMapped(p->Pml4Phys, va)) return 0;

    void *phys = PmmAllocatePage();
    if (!phys) return 0;
    uint8_t *p8 = (uint8_t *)phys;
    for (int i = 0; i < 4096; i++) p8[i] = 0;

    if (MiMapPage(p->Pml4Phys, va, (uint64_t)(uintptr_t)phys,
                  PTE_PRESENT | PTE_WRITABLE | PTE_USER) != 0) {
        PmmFreePage(phys);
        return 0;
    }

    KdPrintf("[usr] stack grow -> va=0x%llx\n", (unsigned long long)va);
    return 1;
}

static void KiHandleUserFault(INTERRUPT_FRAME *f)
{
    if (f->vector == 14) {
        uint64_t cr2;
        __asm__ volatile ("mov %%cr2, %0" : "=r"(cr2));

        if ((f->error & 4) && !(f->error & 1) && KiGrowUserStack(cr2))
            return;
    }

    const char *name = (f->vector < 32) ? gExcNames[f->vector] : "Unknown";

    KdPrintf("\n[usr] fault #%llu (%s)\n",
             (unsigned long long)f->vector, name);
    KdPrintf("[usr]   RIP=0x%llx CS=0x%llx RFLAGS=0x%llx\n",
             (unsigned long long)f->rip, (unsigned long long)f->cs,
             (unsigned long long)f->rflags);
    KdPrintf("[usr]   RSP=0x%llx SS=0x%llx error=0x%llx\n",
             (unsigned long long)f->rsp, (unsigned long long)f->ss,
             (unsigned long long)f->error);
    KdPrintf("[usr]   RAX=0x%llx RBX=0x%llx RCX=0x%llx RDX=0x%llx\n",
             (unsigned long long)f->rax, (unsigned long long)f->rbx,
             (unsigned long long)f->rcx, (unsigned long long)f->rdx);
    KdPrintf("[usr]   RSI=0x%llx RDI=0x%llx RBP=0x%llx\n",
             (unsigned long long)f->rsi, (unsigned long long)f->rdi,
             (unsigned long long)f->rbp);
    KdPrintf("[usr]   R8=0x%llx R9=0x%llx R10=0x%llx R11=0x%llx\n",
             (unsigned long long)f->r8, (unsigned long long)f->r9,
             (unsigned long long)f->r10, (unsigned long long)f->r11);
    KdPrintf("[usr]   R12=0x%llx R13=0x%llx R14=0x%llx R15=0x%llx\n",
             (unsigned long long)f->r12, (unsigned long long)f->r13,
             (unsigned long long)f->r14, (unsigned long long)f->r15);

    if (f->rip < 0x100000000ULL) {
        uint8_t *b = (uint8_t *)(uintptr_t)f->rip;
        KdPrintf("[usr]   bytes@RIP:");
        for (int i = 0; i < 16; i++)
            KdPrintf(" %02x", b[i]);
        KdPrint("[usr]   \n");
    }

    if (f->rsp < 0x100000000ULL) {
        uint64_t *sp = (uint64_t *)(uintptr_t)f->rsp;
        KdPrintf("[usr]   stack @RSP=0x%llx:\n",
                 (unsigned long long)f->rsp);
        for (int i = 0; i < 12; i++)
            KdPrintf("[usr]     [+0x%02x] = 0x%016llx\n",
                     i * 8, (unsigned long long)sp[i]);
    }

    {
        uint32_t kgs_lo, kgs_hi;
        __asm__ volatile ("rdmsr"
                          : "=a"(kgs_lo), "=d"(kgs_hi)
                          : "c"(0xC0000102u));
        KdPrintf("[usr]   user GS_BASE was = 0x%08x%08x "
                 "(want 0x000000008000900000)\n",
                 kgs_hi, kgs_lo);
    }

    if (f->vector == 14) {
        uint64_t cr2;
        __asm__ volatile ("mov %%cr2, %0" : "=r"(cr2));
        KdPrintf("[usr]   CR2=0x%llx reason=%s %s %s\n",
                 (unsigned long long)cr2,
                 (f->error & 1) ? "protection" : "not-present",
                 (f->error & 2) ? "write" : "read",
                 (f->error & 4) ? "user" : "kernel");
    }

    KdPrint("[usr] terminating user thread\n");
    PsTerminateCurrentThread();
    for (;;) __asm__ volatile ("hlt");
}

void KiIsrCommon(INTERRUPT_FRAME *frame)
{
    if (frame->vector >= 0x20 && frame->vector < 0x30) {
        static int gIrqDbg = 0;
        if (gIrqDbg++ < 5)
            KdPrintf("[irqdbg] vec=0x%llx cs=0x%llx rip=0x%llx\n",
                     (unsigned long long)frame->vector,
                     (unsigned long long)frame->cs,
                     (unsigned long long)frame->rip);
    }

    if (frame->vector == 0x80) {
        KiSystemCall(frame);
        return;
    }

    if (frame->vector < 32) {
        if (KiIsUserFault(frame)) {
            KiHandleUserFault(frame);
            return;
        }
        KiPanicException(frame);
    } 
    else if (frame->vector >= 0x20 && frame->vector < 0x30) {
        KiHandleIrq(frame, (uint8_t)(frame->vector - 0x20));
    } 
    else if (frame->vector == 64) {
        HalpLapicSendEoi();
        KeCurrentCpu()->Quantum = 0; 
        
    }
    else {
        KdPrintf("[Ke] unhandled vector %llu, halting\n",
                 (unsigned long long)frame->vector);
        for (;;) __asm__ volatile ("hlt");
    }
}