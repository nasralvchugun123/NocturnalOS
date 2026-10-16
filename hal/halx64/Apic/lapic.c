#include <dbg/print/kdprint.h>
#include "Apic.h"
#include <rt.h>

#define MSR_APIC_BASE       0x1B
#define MSR_APIC_BASE_EN    (1ULL << 11)

static volatile uint32_t *gLapic = 0;

static inline uint64_t rdmsr(uint32_t msr)
{
    uint32_t lo, hi;
    __asm__ volatile ("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
    return ((uint64_t)hi << 32) | lo;
}

static inline void wrmsr(uint32_t msr, uint64_t value)
{
    __asm__ volatile ("wrmsr" :: "c"(msr),
                      "a"((uint32_t)(value & 0xFFFFFFFF)),
                      "d"((uint32_t)(value >> 32)));
}

static inline void lapic_write(uint32_t reg, uint32_t val)
{
    gLapic[reg / 4] = val;
}

static inline uint32_t lapic_read(uint32_t reg)
{
    return gLapic[reg / 4];
}

void HalpLapicInit(void)
{
    KdPrint("[Hal] LAPIC init\n");

    uint64_t base = rdmsr(MSR_APIC_BASE);
    KdPrintf("[Hal] LAPIC MSR base=0x%llx\n", (unsigned long long)base);

    base |= MSR_APIC_BASE_EN;
    wrmsr(MSR_APIC_BASE, base);

    uint64_t lapic_phys = base & 0xFFFFFFFFFFFFF000ULL;
    gLapic = (volatile uint32_t *)(uintptr_t)lapic_phys;
    KdPrintf("[Hal] LAPIC base=0x%llx\n", (unsigned long long)lapic_phys);

    lapic_write(LAPIC_TPR, 0);

    lapic_write(LAPIC_SVR, LAPIC_SVR_ENABLE | 0xFF);

    lapic_write(LAPIC_LVT_LINT0,  1 << 16);   /* masked */
    lapic_write(LAPIC_LVT_LINT1,  1 << 16);
    lapic_write(LAPIC_LVT_ERROR,  1 << 16);

    lapic_write(LAPIC_TIMER_DIV,  0x3);        /* /16 */
    lapic_write(LAPIC_LVT_TIMER,  0x20 | (1u << 17  ));  
    lapic_write(LAPIC_TIMER_INIT, 62500);      

    KdPrintf("[Hal] LAPIC ID=%u, timer armed at 100 Hz\n",
             (unsigned)(lapic_read(LAPIC_ID) >> 24));
}

void HalpLapicSendEoi(void)
{
    if (gLapic) lapic_write(LAPIC_EOI, 0);
}

uint32_t HalpLapicGetId(void)
{
    if (!gLapic) return 0;
    return lapic_read(LAPIC_ID) >> 24;
}

void HalpLapicInitAp(void)
{
    uint64_t base = rdmsr(MSR_APIC_BASE);
    base |= MSR_APIC_BASE_EN;
    wrmsr(MSR_APIC_BASE, base);

    uint64_t lapic_phys = base & 0xFFFFFFFFFFFFF000ULL;
    gLapic = (volatile uint32_t *)(uintptr_t)lapic_phys;

    lapic_write(LAPIC_TPR, 0);
    lapic_write(LAPIC_SVR, LAPIC_SVR_ENABLE | 0xFF);

    lapic_write(LAPIC_LVT_LINT0, 1u << 16);
    lapic_write(LAPIC_LVT_LINT1, 1u << 16);
    lapic_write(LAPIC_LVT_ERROR, 1u << 16);

    lapic_write(LAPIC_TIMER_DIV, 0x3);
    lapic_write(LAPIC_LVT_TIMER, 0x20 | (1u << 17));
    lapic_write(LAPIC_TIMER_INIT, 62500);

    KdPrintf("[SMP]   AP LAPIC id=%u timer armed\n",
             (unsigned)(lapic_read(LAPIC_ID) >> 24));
}

void HalpSendIpi(uint32_t TargetApicId, uint32_t IcrLow)
{
    lapic_write(LAPIC_ICR_HIGH, (TargetApicId & 0xFFu) << 24);
    lapic_write(LAPIC_ICR_LOW, IcrLow);
    for (int i = 0; i < 100000; i++) {
        if (!(lapic_read(LAPIC_ICR_LOW) & (1u << 12))) return;
        __asm__ volatile("pause");
    }
}