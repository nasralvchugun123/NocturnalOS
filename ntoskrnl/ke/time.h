
#ifndef KE_TIME_H
#define KE_TIME_H

#include <stdint.h>

typedef struct {
    uint32_t LowPart;
    int32_t HighPart;
} LARGE_INTEGER;

LARGE_INTEGER KeQueryPerformanceCounter(void);

#endif
