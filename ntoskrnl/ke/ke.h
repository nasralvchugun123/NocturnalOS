
#ifndef KE_H
#define KE_H

#include <stdint.h>

void KeInitializeClock(uint32_t Interval);
void KeUpdateClock(void);
uint64_t KeGetClockInterruptCount(void);

void KeInitializeExceptionHandling(void);
void KeDispatchException(uint32_t ExceptionCode);
uint64_t KeGetExceptionCount(void);

void KeFatalException(uint32_t Code, const char *Message);
void KeInitSystem(void);
void KiInitializeGdt(void);
void KiInitializeTss(void);
void KiInitializeIdt(void);

#endif
