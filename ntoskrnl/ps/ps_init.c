#include <dbg/print/kdprint.h>
#include <ds/fb/fb.h>
#include <ke/kcpu.h>
#include "ps.h"
#include <rt.h>

void PsInitSystem(void)
{
    KdPrint("[ps] PsInitSystem: start\n");

    EPROCESS *sys = PsGetSystemProcess();
    sys->Pid = 0;
    sys->Name[0]='S'; sys->Name[1]='y'; sys->Name[2]='s'; sys->Name[3]='t';
    sys->Name[4]='e'; sys->Name[5]='m'; sys->Name[6]=0;
    sys->Pml4Phys = 0;
    sys->MainThread = 0;

    KTHREAD *bootstrap = PsCreateThread(0, 0, "bootstrap", 0);
    if (!bootstrap) {
        KdPrint("[ps] FATAL: bootstrap thread creation failed\n");
        for (;;) __asm__ volatile ("hlt");
    }
    bootstrap->State   = ThreadStateRunning;
    bootstrap->Process = sys;
    sys->MainThread    = bootstrap;

    KiSetCurrentThread(bootstrap);

    KiInitScheduler();

    KdPrint("[ps] PsInitSystem: ok\n");
}