#include "kcpu.h"
#include "x64/msr.h"
#include <dbg/print/kdprint.h>

KCPU     gCpus[KCpuMaxCount];
uint32_t gCpuCount = 0;

KCPU *KeGetCpuByIndex(uint32_t Index)
{
    if (Index >= KCpuMaxCount) return 0;
    return &gCpus[Index];
}

void KeInitializeProcessorStruct(KCPU *Cpu, uint32_t Index, uint32_t ApicId)
{
    uint8_t *p = (uint8_t *)Cpu;
    for (uint64_t i = 0; i < sizeof(KCPU); i++) p[i] = 0;
    Cpu->Self    = Cpu;
    Cpu->CpuId   = Index;
    Cpu->ApicId  = ApicId;
    Cpu->Quantum = 10;
    KeInitializeSpinLock(&Cpu->RunQueueLock);   
}

void KCpuInitBsp(void)
{
    KdPrint("[KCPU] KCpuInitBsp: start\n");

    KeInitializeProcessorStruct(&gCpus[0], 0, 0);
    gCpus[0].Online = 1;
    gCpuCount       = 1;

    wrmsr(KCpuGsMsrBase, (uint64_t)(uintptr_t)&gCpus[0]);
    wrmsr(KCpuGsMsrKernelBase, 0);

    KdPrintf("[KCPU] BSP: KCPU=0x%llx id=%u, gs check=%u\n",
             (unsigned long long)(uintptr_t)&gCpus[0],
             (unsigned)KeCurrentCpu()->CpuId,
             (unsigned)(KeCurrentCpu() == &gCpus[0]));

    KdPrint("[KCPU] KCpuInitBsp: ok\n");
}