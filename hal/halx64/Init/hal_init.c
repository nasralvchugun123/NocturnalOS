#include <dbg/print/kdprint.h>
#include "pic.h"
#include "hal_irq.h"
#include "../Apic/Apic.h"
#include <rt.h>
#include "../PCI/pci.h"

void HalInitSystem(void)
{
    KdPrint("[Hal] HalInitSystem: start\n");

    HalpPicInit();
    HalpPicDisable();

    HalpLapicInit();

    HalpIoapicInit();
    HalpPciInit();

    KdPrint("[Hal] HalInitSystem: ok\n");
}

void HalInitializeProcessor(void)
{
    //!
}