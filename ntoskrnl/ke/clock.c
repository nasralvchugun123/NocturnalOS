#include "../ntoskrnl.h"
#include <rt.h>
#include <ke/kcpu.h>
#include <nt/atomic.h>

typedef struct {
    volatile int64_t InterruptCount;
    uint64_t ExceptionCount;
} CLOCK_STATE;

static CLOCK_STATE gClockState = {0, 0};
static uint32_t    gClockInterval = 0;

void KeInitializeClock(uint32_t Interval)
{
    gClockInterval = Interval;
    gClockState.InterruptCount = 0;
}

extern void KiDispatchTick(void);
extern void KeIncrementTick(void);

void KeUpdateClock(void)
{
    KCPU *cpu = KeCurrentCpu();
    cpu->TickCount++;

    if (cpu->CpuId == 0) {
        (void)AtomicIncrement64(&gClockState.InterruptCount);
        KeIncrementTick();
    }

    KiDispatchTick();
}

uint64_t KeGetClockInterruptCount(void)
{
    return (uint64_t)AtomicLoadAcquire64(&gClockState.InterruptCount);
}