
#ifndef KE_EXEPT_H
#define KE_EXEPT_H

#include <stdint.h>

void KeInitializeExceptionHandling(void);
void KeDispatchException(uint32_t ExceptionCode);
uint64_t KeGetExceptionCount(void);

#endif