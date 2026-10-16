#ifndef KE_SPINLOCK_H
#define KE_SPINLOCK_H

#include <stdint.h>
#include <nt/atomic.h>

typedef struct {
    volatile int32_t Lock;
} KSPINLOCK;

#define KSPINLOCK_INIT { 0 }

static inline uint64_t KiReadFlags(void)
{
    uint64_t f;
    __asm__ volatile ("pushfq; popq %0" : "=r"(f) :: "memory");
    return f;
}

static inline void KiWriteFlags(uint64_t f)
{
    __asm__ volatile ("pushq %0; popfq" :: "r"(f) : "memory", "cc");
}

static inline void KiDisableInterrupts(void) { __asm__ volatile ("cli" ::: "memory"); }
static inline void KiEnableInterrupts(void)  { __asm__ volatile ("sti" ::: "memory"); }

void     KeInitializeSpinLock(KSPINLOCK *Lock);
void     KeAcquireSpinLock(KSPINLOCK *Lock);
void     KeReleaseSpinLock(KSPINLOCK *Lock);

uint64_t KeAcquireSpinLockIrqSave(KSPINLOCK *Lock);
void     KeReleaseSpinLockIrqRestore(KSPINLOCK *Lock, uint64_t Flags);

#endif