#include "io.h"
#include <rt.h>
#include <dbg/print/kdprint.h>

void IoInitSystem(void)
{
    KdPrint("[IO] IoInitSystem\n");
    KdPrint("[IO]   I/O Manager ready\n");
    KdPrintf("[IO]   max drivers=%u devices=%u irp stack=%u\n",
             IO_MAX_DRIVERS, IO_MAX_DEVICES, IO_MAX_IRP_STACK);
}