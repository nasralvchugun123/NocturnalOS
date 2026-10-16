#include "gdt.h"
#include "../arch/ke_arch.h"
#include "../kcpu.h"
#include <rt.h>
#include <dbg/print/kdprint.h>
#include <ds/fb/fb.h>

static void gdt_set(KGDT_ENTRY *gdt, int idx, uint32_t base, uint32_t limit,
                    uint8_t access, uint8_t flags)
{
    gdt[idx].base_low       = (uint16_t)(base & 0xFFFF);
    gdt[idx].base_mid       = (uint8_t)((base >> 16) & 0xFF);
    gdt[idx].base_hi        = (uint8_t)((base >> 24) & 0xFF);
    gdt[idx].limit_low      = (uint16_t)(limit & 0xFFFF);
    gdt[idx].flags_limit_hi = (uint8_t)(((limit >> 16) & 0x0F) | (flags & 0xF0));
    gdt[idx].access         = access;
}

void KiBuildGdtForCpu(KCPU *cpu)
{
    KGDT_ENTRY *gGdt = cpu->Gdt;
    KTSS       *gTss = &cpu->Tss;

    for (int i = 0; i < KGDT_ENTRIES; i++) {
        gGdt[i].limit_low = 0; gGdt[i].base_low = 0; gGdt[i].base_mid = 0;
        gGdt[i].access = 0; gGdt[i].flags_limit_hi = 0; gGdt[i].base_hi = 0;
    }

    gdt_set(gGdt, 1, 0, 0xFFFFF, 0x9A, 0xA0);
    gdt_set(gGdt, 2, 0, 0xFFFFF, 0x92, 0xC0);
    gdt_set(gGdt, 3, 0, 0xFFFFF, 0xF2, 0xC0);
    gdt_set(gGdt, 4, 0, 0xFFFFF, 0xFA, 0xA0);
    {
        KGDT_TSS_ENTRY *te = (KGDT_TSS_ENTRY *)&gGdt[5];
        /* IST1 top */
        cpu->Tss.ist[0] = (uint64_t)(uintptr_t)&cpu->IstStack[4096];
        /* IST2 top */
        cpu->Tss.ist[1] = (uint64_t)(uintptr_t)&cpu->IstStack[8192];

        uint64_t base  = (uint64_t)(uintptr_t)gTss;
        uint32_t limit = (uint32_t)(sizeof(KTSS) - 1);
        te->limit_low      = (uint16_t)(limit & 0xFFFF);
        te->base_low       = (uint16_t)(base & 0xFFFF);
        te->base_mid       = (uint8_t)((base >> 16) & 0xFF);
        te->access         = 0x89;
        te->flags_limit_hi = (uint8_t)((limit >> 16) & 0x0F);
        te->base_hi        = (uint8_t)((base >> 24) & 0xFF);
        te->base_upper     = (uint32_t)(base >> 32);
        te->reserved       = 0;
    }
}

static void KiLoadGdt(KCPU *cpu)
{
    KGDTR gdtr;
    gdtr.limit = (uint16_t)(sizeof(cpu->Gdt) - 1);
    gdtr.base  = (uint64_t)(uintptr_t)cpu->Gdt;

    __asm__ volatile (
        "lgdt %0\n\t"
        "pushq $0x08\n\t"
        "leaq 1f(%%rip), %%rax\n\t"
        "pushq %%rax\n\t"
        "lretq\n\t"
        "1:\n\t"
        "mov $0x10, %%ax\n\t"
        "mov %%ax, %%ds\n\t"
        "mov %%ax, %%es\n\t"
        "mov %%ax, %%ss\n\t"
        "mov %%ax, %%fs\n\t"
        :
        : "m"(gdtr)
        : "rax", "memory"
    );
}

void KiInitializeGdt(void)
{
    KCPU *cpu = KeCurrentCpu();
    KdPrintf("[Ke] KiInitializeGdt: building per-CPU GDT for cpu %u\n",
             (unsigned)cpu->CpuId);
    KiBuildGdtForCpu(cpu);
    KiLoadGdt(cpu);
    KdPrint("[Ke] KiInitializeGdt: ok\n");
}

void KiInitializeTss(void)
{
    KCPU *cpu = KeCurrentCpu();
    KTSS *tss = &cpu->Tss;

    for (uint64_t i = 0; i < sizeof(KTSS); i++)
        ((uint8_t *)tss)[i] = 0;
    tss->iomap_base = sizeof(KTSS);
    /* KiBuildGdtForCpu sets these before the TSS is zeroed here. Restore them. */
    tss->ist[0] = (uint64_t)(uintptr_t)&cpu->IstStack[4096];
    tss->ist[1] = (uint64_t)(uintptr_t)&cpu->IstStack[8192];

    uint16_t sel = KGDT_TSS;
    __asm__ volatile ("ltr %0" :: "rm"(sel));

    KdPrintf("[Ke] KiInitializeTss: ok on cpu %u\n", (unsigned)cpu->CpuId);
}

void KiSetTssRsp0(uint64_t Rsp0)
{
    KCPU *cpu = KeCurrentCpu();
    cpu->Tss.rsp0 = Rsp0;
    cpu->Rsp0     = Rsp0;
}