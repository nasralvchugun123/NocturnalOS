#include <dbg/print/kdprint.h>
#include <ds/fb/fb.h>
#include "ke.h"
#include <rt.h>
#include "clock.h"
#include "exept.h"
#include "arch/ke_arch.h"

extern void HalpPicInit(void);
extern void HalEnableSystemInterrupt(uint8_t Irq);

void KeInitSystem(void)
{
    KdPrint("[Ke] KeInitSystem: start\n");

    KiInitializeGdt();
    KiInitializeTss();
    KiInitializeIdt();

    KeInitializeClock(100);
    KeInitializeExceptionHandling();

    HalEnableSystemInterrupt(1);

    KdPrint("[Ke] KeInitSystem: ok\n");
}