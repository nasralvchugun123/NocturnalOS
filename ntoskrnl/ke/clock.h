
#ifndef KE_CLOCK_H
#define KE_CLOCK_H

#include <stdint.h>

void KeInitializeClock(uint32_t Interval);
void KeUpdateClock(void);
uint64_t KeGetClockInterruptCount(void);

#endif