#include "NTdll.h"
#include <rt.h>

LDRP_CTX gLdr;

static uint8_t  gHeapBuf[256 * 1024];
static uint64_t gHeapUsed;

void *LdrpHeapAlloc(uint64_t Size)
{
    Size = (Size + 15) & ~15ULL;
    if (gHeapUsed + Size > sizeof(gHeapBuf)) return 0;
    void *p = &gHeapBuf[gHeapUsed];
    gHeapUsed += Size;
    return p;
}

void LdrpHeapFree(void *Ptr) { (void)Ptr; }
    
__attribute__((noreturn))
void LdrpInitializeThunk(void *Arg)
{
    (void)Arg;
    for (;;) __asm__ volatile ("hlt");
}
