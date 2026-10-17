#include "ps.h"
#include "rt.h"
#include "../../../../mm/mm.h"
#include "../../../../ob/shared.h"
#include <dbg/print/kdprint.h>
#include <ke/kcpu.h>
#include <ke/spinlock.h>
#include <halx64/Apic/Apic.h>

#define QUANTUM_TICKS    10
#define IPI_VECTOR_RESCHEDULE 0x40
#define WAKEUP_INFINITE  0xFFFFFFFFFFFFFFFFULL

extern uint64_t MmGetKernelPml4(void);
extern uint64_t KeGetTickCount(void);
static KSPINLOCK gBlockedLock = KSPINLOCK_INIT;
static KTHREAD  *gBlockedHead = 0;

KTHREAD *KeGetCurrentThread(void) { return KeCurrentCpu()->CurrentThread; }
void KiSetCurrentThread(KTHREAD *t) { KeCurrentCpu()->CurrentThread = t; }

static volatile uint32_t gNextCpuIndex = 0;

uint32_t KiGetNextCpuForThread(void)
{
    uint32_t start_idx = __atomic_fetch_add(&gNextCpuIndex, 1, __ATOMIC_RELAXED);
    
    for (uint32_t i = 0; i < KCpuMaxCount; i++) {
        uint32_t idx = (start_idx + i) % KCpuMaxCount;
        if (gCpus[idx].Online) {
            return idx;
        }
    }
    return 0; 
}


void KiInsertReadyThreadOnCpu(uint32_t CpuIndex, KTHREAD *t)
{
    if (!t) return;
    if (CpuIndex >= KCpuMaxCount) CpuIndex = 0;

    KCPU *cpu = &gCpus[CpuIndex];
    uint64_t f = KeAcquireSpinLockIrqSave(&cpu->RunQueueLock);

    for (KTHREAD *queued = cpu->ReadyHead; queued; queued = queued->Next) {
        if (queued == t) {
            KeReleaseSpinLockIrqRestore(&cpu->RunQueueLock, f);
            return;
        }
    }

    t->Next = 0;
    t->State = ThreadStateReady;
    t->LastCpuIndex = CpuIndex;

    if (!cpu->ReadyHead) {
        cpu->ReadyHead = cpu->ReadyTail = t;
    } else {
        cpu->ReadyTail->Next = t;
        cpu->ReadyTail       = t;
    }

    KeReleaseSpinLockIrqRestore(&cpu->RunQueueLock, f);

    uint32_t my_cpu = KeCurrentCpu()->CpuId; 

    if (CpuIndex != my_cpu) {
        uint32_t icr = 0x40; 
        HalpSendIpi(cpu->ApicId, icr);
    }
}

void KiInsertReadyThread(KTHREAD *t)
{
    KiInsertReadyThreadOnCpu(t->LastCpuIndex, t);
}

static KTHREAD *KiDequeueReadyLocked(KCPU *cpu)
{
    KTHREAD *t = cpu->ReadyHead;
    if (!t) return 0;
    cpu->ReadyHead = t->Next;
    if (!cpu->ReadyHead) cpu->ReadyTail = 0;
    t->Next = 0;
    return t;
}

void KiBlockCurrentThread(uint64_t WakeupTick)
{
    uint64_t f = KeAcquireSpinLockIrqSave(&gBlockedLock);

    KTHREAD *cur = KeGetCurrentThread();
    if (cur && cur->State == ThreadStateRunning) {
        cur->State       = ThreadStateBlocked;
        cur->WakeupTick  = WakeupTick;
        cur->BlockedNext = gBlockedHead;
        gBlockedHead     = cur;
    }

    KeReleaseSpinLockIrqRestore(&gBlockedLock, f);
}

uint64_t KiUnblockExpired(void)
{
    uint64_t woken = 0;

    uint64_t f = KeAcquireSpinLockIrqSave(&gBlockedLock);
    uint64_t now = KeGetTickCount();

    KTHREAD *prev = 0;
    KTHREAD *cur  = gBlockedHead;

    while (cur) {
        KTHREAD *next = cur->BlockedNext;
        if (cur->WakeupTick != WAKEUP_INFINITE && cur->WakeupTick <= now) {
            if (prev) prev->BlockedNext = next;
            else      gBlockedHead      = next;
            cur->BlockedNext = 0;
            cur->WakeupTick  = 0;

            if (cur->WaitObject) {
                cur->WaitSignaled = 0;
                ObUnwaitThread(cur->WaitObject, cur);
                cur->WaitObject = 0;
            }

            KeReleaseSpinLockIrqRestore(&gBlockedLock, f);
            KiInsertReadyThread(cur);
            f = KeAcquireSpinLockIrqSave(&gBlockedLock);

            woken++;
        } else {
            prev = cur;
        }
        cur = next;
    }

    KeReleaseSpinLockIrqRestore(&gBlockedLock, f);
    return woken;
}

void KiWakeThread(KTHREAD *t)
{
    if (!t) return;

    uint64_t f = KeAcquireSpinLockIrqSave(&gBlockedLock);

    if (t->State != ThreadStateBlocked) {
        KeReleaseSpinLockIrqRestore(&gBlockedLock, f);
        return;
    }

    KTHREAD *prev = 0;
    KTHREAD *cur  = gBlockedHead;
    while (cur) {
        if (cur == t) {
            if (prev) prev->BlockedNext = cur->BlockedNext;
            else      gBlockedHead      = cur->BlockedNext;
            cur->BlockedNext = 0;
            break;
        }
        prev = cur;
        cur  = cur->BlockedNext;
    }

    if (!cur) {
        KeReleaseSpinLockIrqRestore(&gBlockedLock, f);
        return;
    }

    t->WaitSignaled = 1;
    t->WakeupTick   = 0;
    t->WaitObject   = 0;

    KeReleaseSpinLockIrqRestore(&gBlockedLock, f);

    KiInsertReadyThread(t);
}

static void KiIdleProc(void *arg)
{
    (void)arg;
    for (;;) {
        __asm__ volatile ("cli");
        if (KeCurrentCpu()->ReadyHead) {
            __asm__ volatile ("sti");
            KiScheduleNext();
        } else {
            __asm__ volatile ("sti");
            __asm__ volatile ("hlt");
        }
    }
}

KTHREAD *KiCreateIdleThreadForCurrentCpu(void)
{
    KTHREAD *idle = PsCreateThread(KiIdleProc, 0, "idle", 0);
    if (!idle) return 0;
    idle->LastCpuIndex = KeCurrentCpu()->CpuId; 
    
    KeCurrentCpu()->IdleThread = idle;
    return idle;
}
void KiInitScheduler(void)
{
    if (!KiCreateIdleThreadForCurrentCpu()) {
        KdPrint("[sched] FATAL: BSP idle thread creation failed\n");
        for (;;) __asm__ volatile ("hlt");
    }
}

static void KiUpdateUserGsBase(KTHREAD *next)
{
    if (next && next->Process && next->Process->Teb) {
        uint64_t teb = next->Process->Teb;
        __asm__ volatile ("wrmsr"
                          :: "c"(0xC0000102u),
                             "a"((uint32_t)(teb & 0xFFFFFFFF)),
                             "d"((uint32_t)(teb >> 32))
                          : "memory");
    }
}

static void KiSwitchTo(KTHREAD *cur, KTHREAD *next)
{
    next->State = ThreadStateRunning;
    KeCurrentCpu()->CurrentThread = next;
    KeCurrentCpu()->Quantum       = QUANTUM_TICKS;

    uint64_t cr3 = (next->Process && next->Process->Pml4Phys)
                 ? next->Process->Pml4Phys
                 : MmGetKernelPml4();

    if (next->KernelStackTop)
        KiSetTssRsp0(next->KernelStackTop);

    KiUpdateUserGsBase(next);

    KiSwapContext(cur, next, cr3);
}
void KiScheduleNext(void)
{
    KCPU *cpu = KeCurrentCpu();
    KTHREAD *cur = cpu->CurrentThread;

    uint64_t f = KeAcquireSpinLockIrqSave(&cpu->RunQueueLock);
    KTHREAD *next = KiDequeueReadyLocked(cpu);
    KeReleaseSpinLockIrqRestore(&cpu->RunQueueLock, f);

    if (!next) next = cpu->IdleThread;
    if (!next) for (;;) __asm__ volatile ("hlt");

    if (next == cur) {
        next->State = ThreadStateRunning;
        return;
    }

    if (cur && cur->State == ThreadStateRunning)
        KiInsertReadyThreadOnCpu(cpu->CpuId, cur);

    KiSwitchTo(cur, next);
}

void KiDispatchTick(void)
{
    KCPU *cpu = KeCurrentCpu();

    if (cpu->NoPreempt > 0) {
        if (cpu->Quantum > 0) cpu->Quantum--;
        return;
    }

    KiUnblockExpired();

    KTHREAD *cur = cpu->CurrentThread;
    if (!cur) return;

    if (cpu->Quantum > 0) {
        cpu->Quantum--;
        return;
    }
    cpu->Quantum = QUANTUM_TICKS;

    uint64_t f = KeAcquireSpinLockIrqSave(&cpu->RunQueueLock);
    KTHREAD *next = KiDequeueReadyLocked(cpu);
    KeReleaseSpinLockIrqRestore(&cpu->RunQueueLock, f);

    if (!next) next = cpu->IdleThread;
    if (!next || next == cur) return;

    if (cur->State == ThreadStateRunning)
        KiInsertReadyThreadOnCpu(cpu->CpuId, cur);

    KiSwitchTo(cur, next);
}