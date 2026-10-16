#include "spinlock.h"

void KeInitializeSpinLock(KSPINLOCK *Lock)
{
    Lock->Lock = 0;
}

void KeAcquireSpinLock(KSPINLOCK *Lock)
{
    while (AtomicExchange32((volatile uint32_t *)&Lock->Lock, 1) != 0) {
        while (Lock->Lock) {
            __asm__ volatile ("pause" ::: "memory");
        }
    }
}

void KeReleaseSpinLock(KSPINLOCK *Lock)
{
    AtomicMemoryBarrier();
    Lock->Lock = 0;
}

uint64_t KeAcquireSpinLockIrqSave(KSPINLOCK *Lock)
{
    uint64_t flags = KiReadFlags();
    KiDisableInterrupts();
    KeAcquireSpinLock(Lock);
    return flags;
}

void KeReleaseSpinLockIrqRestore(KSPINLOCK *Lock, uint64_t Flags)
{
    KeReleaseSpinLock(Lock);
    KiWriteFlags(Flags);
}