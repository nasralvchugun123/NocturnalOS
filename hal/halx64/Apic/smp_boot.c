#include <dbg/print/kdprint.h>
#include <ke/kcpu.h>
#include <ke/x64/msr.h>            
#include "Apic.h"
#include "acpi.h"

extern uint64_t MmGetKernelPml4(void);
extern void    *PmmAllocatePages(uint32_t PageCount);

extern uint8_t TrampolineTemplate[];
extern uint8_t TrampolineTemplateEnd[];

static inline uint64_t rdtsc_now(void)
{
    uint32_t lo, hi;
    __asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

static void HalpStallUs(uint32_t us)
{
    uint64_t cycles = (uint64_t)us * 3000ull;
    uint64_t start  = rdtsc_now();
    while (rdtsc_now() - start < cycles)
        __asm__ volatile ("pause");
}

static void HalpPrepareTrampoline(uint64_t Cr3, uint64_t Stack,
                                  uint64_t Rdi, uint64_t Entry)
{
    uint8_t *dst  = (uint8_t *)(uintptr_t)0x8000;
    uint8_t *src  = TrampolineTemplate;
    uint32_t size = (uint32_t)(TrampolineTemplateEnd - TrampolineTemplate);

    for (uint32_t i = 0; i < size; i++) dst[i] = src[i];

    *(volatile uint64_t *)(dst + 0x08) = Cr3;
    *(volatile uint64_t *)(dst + 0x10) = Stack;
    *(volatile uint64_t *)(dst + 0x18) = Rdi;
    *(volatile uint64_t *)(dst + 0x20) = Entry;

    KdPrintf("[SMP] tramp bytes: %02x %02x %02x %02x %02x %02x %02x %02x "
         "%02x %02x %02x %02x %02x %02x %02x %02x\n",
         dst[0], dst[1], dst[2], dst[3], dst[4], dst[5], dst[6], dst[7],
         dst[8], dst[9], dst[10], dst[11], dst[12], dst[13], dst[14], dst[15]);

    KdPrintf("[SMP] patched qwords 0x8008..0x8028: %llx %llx %llx %llx\n",
         *(volatile uint64_t *)(dst + 0x08),
         *(volatile uint64_t *)(dst + 0x10),
         *(volatile uint64_t *)(dst + 0x18),
         *(volatile uint64_t *)(dst + 0x20));

}

extern void KiApEntry(uint32_t CpuIndex);
static int HalpStartAp(uint32_t TargetApicId, uint32_t CpuIndex)
{
    void *stack = PmmAllocatePages(4);
    if (!stack) {
        KdPrintf("[SMP] cpu%u: no stack\n", CpuIndex);
        return -1;
    }
    uint64_t stack_top = (((uint64_t)(uintptr_t)stack + 4 * 4096) & ~0xFULL) - 0x18;

    KdPrintf("[SMP] AP stack=0x%llx..0x%llx\n",
             (unsigned long long)(uintptr_t)stack,
             (unsigned long long)stack_top);

    *(volatile uint32_t *)(uintptr_t)0x81D0 = 0;
    *(volatile uint32_t *)(uintptr_t)0x81D4 = 0;

    uint64_t cr3 = MmGetKernelPml4();
    HalpPrepareTrampoline(cr3, stack_top, (uint64_t)CpuIndex,
                          (uint64_t)(uintptr_t)&KiApEntry);

    HalpSendIpi(TargetApicId, LAPIC_ICR_INIT | LAPIC_ICR_ASSERT | LAPIC_ICR_LEVEL);
    HalpStallUs(10000);

    HalpSendIpi(TargetApicId, LAPIC_ICR_SIPI | (0x8000u >> 12));
    HalpStallUs(200);

    HalpSendIpi(TargetApicId, LAPIC_ICR_SIPI | (0x8000u >> 12));

    for (int i = 0; i < 1000000; i++) {
        if (gCpus[CpuIndex].Online) return 0;
        __asm__ volatile ("pause");
    }

    uint32_t m1 = *(volatile uint32_t *)(uintptr_t)0x81D0;
    uint32_t m2 = *(volatile uint32_t *)(uintptr_t)0x81D4;
    KdPrintf("[SMP] cpu%u (apic %u): timeout, m1=0x%08x m2=0x%08x\n",
             CpuIndex, TargetApicId, m1, m2);
    return -1;
}

void HalpInitOProcessors(void)
{
    uint32_t tsize = (uint32_t)(TrampolineTemplateEnd - TrampolineTemplate);
    KdPrintf("[SMP] trampoline template size = %u bytes\n", tsize);

    MADT_INFO *m = HalpGetMadtInfo();
    uint32_t self_apic = HalpLapicGetId();
    uint32_t idx = 1;

    KdPrintf("[SMP] BSP apic_id=%u, madt has %u cpu(s)\n",
             self_apic, (unsigned)m->Count);

    for (uint32_t i = 0; i < m->Count; i++) {
        if (!(m->Cpus[i].Flags & ACPI_MADT_LAPIC_ENABLED)) continue;
        if (m->Cpus[i].ApicId == self_apic) continue;
        if (idx >= KCpuMaxCount) break;

        KdPrintf("[SMP] starting AP apic_id=%u as cpu%u...\n",
                 (unsigned)m->Cpus[i].ApicId, idx);

        if (HalpStartAp(m->Cpus[i].ApicId, idx) == 0) {
            KdPrintf("[SMP]   cpu%u online\n", idx);
            gCpuCount++;
            idx++;
        } else {
            KdPrint("[SMP] stopping AP bring-up after timeout\n");
            break;
        }
    }

    KdPrintf("[SMP] online CPUs: %u\n", (unsigned)gCpuCount);
}