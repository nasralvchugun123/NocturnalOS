#ifndef KE_KCPU_H
#define KE_KCPU_H

#include <stdint.h>
#include "x64/gdt.h"
#include "spinlock.h"

#define KCpuMaxCount       128

#define KCpuGsMsrBase       0xC0000101u
#define KCpuGsMsrKernelBase 0xC0000102u

#define KCPU_OFF_UserRspSave   0
#define KCPU_OFF_Rsp0          8
#define KCPU_OFF_CurrentThread 16

struct _KTHREAD;

typedef struct _KCPU {
    struct _KCPU     *Self;             /* 0x00 */
    uint64_t          UserRspSave;      /* 0x08 */
    uint64_t          Rsp0;             /* 0x10 */
    struct _KTHREAD  *CurrentThread;    /* 0x18 */
    uint32_t          CpuId;            /* 0x20 */
    uint32_t          ApicId;           /* 0x24 */

    KSPINLOCK         RunQueueLock;    
    struct _KTHREAD  *ReadyHead;
    struct _KTHREAD  *ReadyTail;
    struct _KTHREAD  *IdleThread;
    int32_t           Quantum;
    int32_t           NoPreempt;   
    volatile int32_t  Online;
    uint64_t          TickCount;

    KGDT_ENTRY        Gdt[KGDT_ENTRIES];
    KTSS              Tss;
    uint8_t           IstStack[8192] __attribute__((aligned(16)));
} __attribute__((aligned(64))) KCPU;


extern KCPU     gCpus[KCpuMaxCount];
extern uint32_t gCpuCount;

KCPU *KeGetCpuByIndex(uint32_t Index);
void  KeInitializeProcessorStruct(KCPU *Cpu, uint32_t Index, uint32_t ApicId);

void  KCpuInitBsp(void);

static inline KCPU *KeCurrentCpu(void)
{
    KCPU *c;
    __asm__ volatile ("movq %%gs:0, %0" : "=r"(c));
    return c;
}

static inline uint32_t KeCurrentCpuId(void)
{
    return KeCurrentCpu()->CpuId;
}

#endif