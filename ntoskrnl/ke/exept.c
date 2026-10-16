
#include "../ntoskrnl.h"


typedef struct {
    uint64_t ExceptionCount;
    uint32_t LastException;
} EXCEPTION_STATE;

static EXCEPTION_STATE gExceptionState = {0, 0};

void KeInitializeExceptionHandling(void)
{
    gExceptionState.ExceptionCount = 0;
    gExceptionState.LastException = 0;
}

void KeDispatchException(uint32_t ExceptionCode)
{
    gExceptionState.ExceptionCount++;
    gExceptionState.LastException = ExceptionCode;
}

uint64_t KeGetExceptionCount(void)
{
    return gExceptionState.ExceptionCount;
}
