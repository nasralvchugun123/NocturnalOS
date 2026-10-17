
#include "idt.h"
#include "../arch/ke_arch.h"
#include <rt.h>
#include <dbg/print/kdprint.h>
#include <ds/fb/fb.h>

typedef struct {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  ist;
    uint8_t  type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t reserved;
} __attribute__((packed)) IDT_ENTRY;

typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) IDTR;

static IDT_ENTRY gIdt[256];
static IDTR      gIdtr;

// isr.S stubs 
extern void isr0(void);  extern void isr1(void);  extern void isr2(void);
extern void isr3(void);  extern void isr4(void);  extern void isr5(void);
extern void isr6(void);  extern void isr7(void);  extern void isr8(void);
extern void isr9(void);  extern void isr10(void); extern void isr11(void);
extern void isr12(void); extern void isr13(void); extern void isr14(void);
extern void isr15(void); extern void isr16(void); extern void isr17(void);
extern void isr18(void); extern void isr19(void); extern void isr20(void);
extern void isr21(void); extern void isr22(void); extern void isr23(void);
extern void isr24(void); extern void isr25(void); extern void isr26(void);
extern void isr27(void); extern void isr28(void); extern void isr29(void);
extern void isr30(void); extern void isr31(void); 

extern void isr32(void); extern void isr33(void); extern void isr34(void);
extern void isr35(void); extern void isr36(void); extern void isr37(void);
extern void isr38(void); extern void isr39(void); extern void isr40(void);
extern void isr41(void); extern void isr42(void); extern void isr43(void);
extern void isr44(void); extern void isr45(void); extern void isr46(void);
extern void isr47(void); extern void isr128(void);extern void isr64(void);

extern void isr_default(void);

static void idt_set(int idx, uint64_t handler, uint16_t sel, uint8_t ist, uint8_t type)
{
    gIdt[idx].offset_low  = (uint16_t)(handler & 0xFFFF);
    gIdt[idx].selector    = sel;
    gIdt[idx].ist         = ist & 0x7;
    gIdt[idx].type_attr   = type;
    gIdt[idx].offset_mid  = (uint16_t)((handler >> 16) & 0xFFFF);
    gIdt[idx].offset_high = (uint32_t)((handler >> 32) & 0xFFFFFFFF);
    gIdt[idx].reserved    = 0;
}

void KiInitializeIdt(void)
{
    KdPrint("[Ke] KiInitializeIdt: building IDT\n");

    for (int i = 0; i < 256; i++) {
        gIdt[i].offset_low  = 0;
        gIdt[i].selector    = 0;
        gIdt[i].ist         = 0;
        gIdt[i].type_attr   = 0;
        gIdt[i].offset_mid  = 0;
        gIdt[i].offset_high = 0;
        gIdt[i].reserved    = 0;
    }

    uint8_t attr = 0x8E;

    void (*stubs[48])(void) = {
        isr0,  isr1,  isr2,  isr3,  isr4,  isr5,  isr6,  isr7,
        isr8,  isr9,  isr10, isr11, isr12, isr13, isr14, isr15,
        isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23,
        isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31,
        isr32, isr33, isr34, isr35, isr36, isr37, isr38, isr39,
        isr40, isr41, isr42, isr43, isr44, isr45, isr46, isr47,
    };

   for (int i = 0; i < 48; i++) {
        idt_set(i, (uint64_t)(uintptr_t)stubs[i], KGDT_KCODE64, 0, attr);
    }
    idt_set(2,  (uint64_t)(uintptr_t)isr2,  KGDT_KCODE64, 1, attr);
    idt_set(8,  (uint64_t)(uintptr_t)isr8,  KGDT_KCODE64, 1, attr);
    idt_set(14, (uint64_t)(uintptr_t)isr14, KGDT_KCODE64, 2, attr);

   for (int i = 48; i < 256; i++) {
        idt_set(i, (uint64_t)(uintptr_t)isr_default, KGDT_KCODE64, 0, attr);
    }

    idt_set(64, (uint64_t)(uintptr_t)isr64, KGDT_KCODE64, 0, attr);

    idt_set(0x80, (uint64_t)(uintptr_t)isr128, KGDT_KCODE64, 0, 0xEE);
    
    gIdtr.limit = sizeof(gIdt) - 1;
    gIdtr.base  = (uint64_t)&gIdt;

    __asm__ volatile ("lidt %0" :: "m"(gIdtr));

    KdPrint("[Ke] KiInitializeIdt: ok\n");
}

void KiReloadIdt(void)
{
    __asm__ volatile ("lidt %0" :: "m"(gIdtr));
}