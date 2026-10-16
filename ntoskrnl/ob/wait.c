#include <dbg/print/kdprint.h>
#include <ds/fb/fb.h>
#include "shared.h"
#include "../ps/ps.h"
#include <rt.h>

extern uint64_t KeGetTickCount(void);
extern void     KiWakeThread(KTHREAD *t);
extern void     KiBlockCurrentThread(uint64_t WakeupTick);
extern void     KiScheduleNext(void);
extern KTHREAD *KeGetCurrentThread(void);

#define OB_TIMEOUT_INFINITE  0xFFFFFFFFFFFFFFFFULL

static int ObpTrySatisfy(OBJECT_HEADER *Hdr, KTHREAD *W)
{
    if (Hdr->Type->SatisfyProcedure)
        return Hdr->Type->SatisfyProcedure(Hdr, W);
    return Hdr->SignalState ? 1 : 0;
}

void ObSignalObject(OBJECT_HEADER *Hdr)
{
    if (!Hdr) return;

        Hdr->SignalState = 1;

    KTHREAD *prev = 0;
    KTHREAD *cur  = Hdr->WaiterHead;

    while (cur) {
        KTHREAD *next = cur->WaiterNext;

        if (ObpTrySatisfy(Hdr, cur)) {
            if (prev) prev->WaiterNext = next;
            else      Hdr->WaiterHead  = next;
            cur->WaiterNext = 0;
            cur->WaitObject = 0;
            cur->WaitSignaled = 1;
            KiWakeThread(cur);
        } else {
            prev = cur;
        }
        cur = next;
    }
}

void ObResetObject(OBJECT_HEADER *Hdr)
{
    if (!Hdr) return;
    Hdr->SignalState = 0;
}

void ObUnwaitThread(OBJECT_HEADER *Hdr, KTHREAD *Thread)
{
    if (!Hdr || !Thread) return;

    KTHREAD *prev = 0;
    KTHREAD *cur  = Hdr->WaiterHead;
    while (cur) {
        if (cur == Thread) {
            if (prev) prev->WaiterNext = cur->WaiterNext;
            else      Hdr->WaiterHead  = cur->WaiterNext;
            cur->WaiterNext = 0;
            cur->WaitObject = 0;
            return;
        }
        prev = cur;
        cur  = cur->WaiterNext;
    }
}

int ObWaitForSingleObject(OBJECT_HEADER *Hdr, uint64_t TimeoutTicks)
{
    if (!Hdr) return -1;

    KTHREAD *cur = KeGetCurrentThread();
    if (!cur) return -1;

    cur->WaitStatus   = STATUS_WAIT_0;
    cur->WaitSignaled = 0;

    if (ObpTrySatisfy(Hdr, cur))
        return cur->WaitStatus;

    cur->WaitObject = Hdr;
    cur->WaiterNext = Hdr->WaiterHead;
    Hdr->WaiterHead = cur;

    uint64_t wake;
    if (TimeoutTicks == OB_TIMEOUT_INFINITE) {
        wake = OB_TIMEOUT_INFINITE;
    } else {
        uint64_t now = KeGetTickCount();
        wake = now + TimeoutTicks;
        if (wake == OB_TIMEOUT_INFINITE) wake--;
    }

    KiBlockCurrentThread(wake);
    KiScheduleNext();

    if (cur->WaitSignaled)
        return cur->WaitStatus;

    return STATUS_TIMEOUT;
}

void ObpThreadTerminated(KTHREAD *t)
{
    if (!t || !t->Process) return;

    EPROCESS *p = t->Process;

    for (uint32_t i = 0; i < PS_MAX_HANDLES; i++) {
        OBJECT_HEADER *hdr = p->Handles[i].Object;
        if (!hdr) continue;
        if (hdr->Type != &ObpMutantType) continue;

        KMUTANT *m = (KMUTANT *)OBJ_BODY(hdr);
        if (m->Owner == t) {
            KdPrintf("[Ob] mutant abandoned by thread '%s'\n", t->Name);
            m->Owner     = 0;
            m->Count     = 0;
            m->Abandoned = 1;
            hdr->SignalState = 1;
            ObSignalObject(hdr);
        }
    }
}