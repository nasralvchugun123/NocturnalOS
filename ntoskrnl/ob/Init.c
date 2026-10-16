
#include "shared.h"
#include "../ps/ps.h"
#include <rt.h>
#include <dbg/print/kdprint.h>
#include <ds/fb/fb.h>

static int DirectorySatisfy(OBJECT_HEADER *Hdr, struct _KTHREAD *W)
{
    (void)Hdr; (void)W;
    return 0;
}

OBJECT_TYPE ObpDirectoryType = {
    .Name = "Directory", .TypeId = 6,
    .DeleteProcedure = 0, .SatisfyProcedure = DirectorySatisfy,
};

OBJECT_HEADER *ObpBaseNamedObjects = 0;

static int GenericSatisfy(OBJECT_HEADER *Hdr, struct _KTHREAD *W)
{
    (void)W;
    return Hdr->SignalState ? 1 : 0;
}

static int MutantSatisfy(OBJECT_HEADER *Hdr, struct _KTHREAD *W)
{
    KMUTANT *m = (KMUTANT *)OBJ_BODY(Hdr);

    if (m->Owner == 0) {
        m->Owner = W;
        m->Count = 1;
        W->WaitStatus = m->Abandoned ? STATUS_ABANDONED_WAIT_0
                                     : STATUS_WAIT_0;
        m->Abandoned = 0;
        Hdr->SignalState = 0;
        return 1;
    }
    if (m->Owner == W) {
        m->Count++;
        W->WaitStatus = STATUS_WAIT_0;
        return 1;
    }
    return 0;
}

static int EventSatisfy(OBJECT_HEADER *Hdr, struct _KTHREAD *W)
{
    (void)W;
    KEVENT *e = (KEVENT *)OBJ_BODY(Hdr);

    if (!Hdr->SignalState) return 0;

    if (e->EventType == SynchronizationEvent)
        Hdr->SignalState = 0;   

    return 1;
}

static int SemaphoreSatisfy(OBJECT_HEADER *Hdr, struct _KTHREAD *W)
{
    (void)W;
    KSEMAPHORE *s = (KSEMAPHORE *)OBJ_BODY(Hdr);

    if (s->Count > 0) {
        s->Count--;
        if (s->Count == 0)
            Hdr->SignalState = 0;
        return 1;
    }
    return 0;
}

OBJECT_TYPE ObpProcessType = {
    .Name = "Process", .TypeId = 1,
    .DeleteProcedure = 0, .SatisfyProcedure = GenericSatisfy,
};

OBJECT_TYPE ObpThreadType = {
    .Name = "Thread", .TypeId = 2,
    .DeleteProcedure = 0, .SatisfyProcedure = GenericSatisfy,
};

OBJECT_TYPE ObpMutantType = {
    .Name = "Mutant", .TypeId = 3,
    .DeleteProcedure = 0, .SatisfyProcedure = MutantSatisfy,
};

OBJECT_TYPE ObpEventType = {
    .Name = "Event", .TypeId = 4,
    .DeleteProcedure = 0, .SatisfyProcedure = EventSatisfy,
};

OBJECT_TYPE ObpSemaphoreType = {
    .Name = "Semaphore", .TypeId = 5,
    .DeleteProcedure = 0, .SatisfyProcedure = SemaphoreSatisfy,
};

void ObInitSystem(void)
{
    KdPrint("[Ob] ObInitSystem: start\n");
    KdPrintf("[Ob] registered: %s id=%u\n", ObpProcessType.Name,   ObpProcessType.TypeId);
    KdPrintf("[Ob] registered: %s id=%u\n", ObpThreadType.Name,    ObpThreadType.TypeId);
    KdPrintf("[Ob] registered: %s id=%u\n", ObpMutantType.Name,    ObpMutantType.TypeId);
    KdPrintf("[Ob] registered: %s id=%u\n", ObpEventType.Name,     ObpEventType.TypeId);
    KdPrintf("[Ob] registered: %s id=%u\n", ObpSemaphoreType.Name, ObpSemaphoreType.TypeId);

    OBJECT_DIRECTORY *bno = (OBJECT_DIRECTORY *)ObCreateObject(&ObpDirectoryType,
                                                                sizeof(OBJECT_DIRECTORY));
    if (!bno) {
        KdPrint("[Ob] FATAL: cannot create \\BaseNamedObjects\n");
        return;
    }
    bno->EntryCount = 0;
    ObpBaseNamedObjects = OBJ_HEADER(bno);
    KdPrint("[Ob] created \\BaseNamedObjects\n");

    KdPrint("[Ob] ObInitSystem: ok\n");
}