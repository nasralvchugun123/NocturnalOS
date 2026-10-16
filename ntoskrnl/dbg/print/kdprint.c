#include "kdprint.h"
#include <RTL/rt.h>
#include <stdint.h>
#include <ds/fb/fb.h>
#include <ke/spinlock.h>

 #define KD_ENABLE_SERIAL 1

static KSPINLOCK gKdPrintLock = KSPINLOCK_INIT;

#ifdef KD_ENABLE_SERIAL
static inline void outb(uint16_t p, uint8_t v)
{ __asm__ volatile ("outb %0, %1" :: "a"(v), "Nd"(p)); }

static inline uint8_t inb(uint16_t p)
{ uint8_t v; __asm__ volatile ("inb %1, %0" : "=a"(v) : "Nd"(p)); return v; }
#endif

VOID KdInitSerial(VOID)
{
#ifdef KD_ENABLE_SERIAL
    outb(0x3F8 + 1, 0x00);
    outb(0x3F8 + 3, 0x80);
    outb(0x3F8 + 0, 0x01);  
    outb(0x3F8 + 1, 0x00);
    outb(0x3F8 + 3, 0x03);
    outb(0x3F8 + 2, 0xC7);
    outb(0x3F8 + 4, 0x0B);
#endif
}

static void kd_putc(char c)
{
#ifdef KD_ENABLE_SERIAL
    while (!(inb(0x3F8 + 5) & 0x20)) { }
    outb(0x3F8, (uint8_t)c);
#endif
    if (FbIsReady()) FbPutChar(c);
}

VOID KdPrint(const char *s)
{
    uint64_t flags = KeAcquireSpinLockIrqSave(&gKdPrintLock);
    while (*s) {
        if (*s == '\n') kd_putc('\r');
        kd_putc(*s++);
    }
    KeReleaseSpinLockIrqRestore(&gKdPrintLock, flags);
}

VOID KdPrintf(const char *fmt, ...)
{
    char buf[512];
    __builtin_va_list ap;
    __builtin_va_start(ap, fmt);
    RtlVsprintf(buf, sizeof(buf), fmt, ap);
    __builtin_va_end(ap);
    KdPrint(buf);
}