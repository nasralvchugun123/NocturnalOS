#include <dbg/print/kdprint.h>
#include <ke/kcpu.h>
#include <ke/x64/msr.h>
#include "rt.h"
#include "ps.h"

extern void KiInitializeGdt(void);
extern void KiInitializeTss(void);
extern void KiReloadIdt(void);
extern void HalpLapicInitAp(void);
extern uint32_t HalpLapicGetId(void);
extern KTHREAD *KiCreateIdleThreadForCurrentCpu(void);

void KiApEntry(uint32_t CpuIndexIgnored)
{
    (void)CpuIndexIgnored;
    uint32_t CpuIndex = *(volatile uint32_t *)(uintptr_t)0x8018;
    if (CpuIndex >= KCpuMaxCount) for (;;) __asm__ volatile ("cli; hlt");

    KCPU *cpu = &gCpus[CpuIndex];
    KeInitializeProcessorStruct(cpu, CpuIndex, 0);
    wrmsr(0xC0000101u, (uint64_t)(uintptr_t)cpu);

    KiInitializeGdt();
    KiInitializeTss();
    KiReloadIdt();
    HalpLapicInitAp();
    cpu->ApicId = HalpLapicGetId();

    if (!KiCreateIdleThreadForCurrentCpu()) {
        for (;;) __asm__ volatile ("cli; hlt");
    }

    __asm__ volatile ("mfence" ::: "memory");
    cpu->Online = 1;
    KdPrintf("[SMP]   cpu%u online\n", CpuIndex);

    __asm__ volatile ("sti");
    KiScheduleNext();

    for (;;) __asm__ volatile ("hlt");
}