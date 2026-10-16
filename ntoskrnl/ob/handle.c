
#include "shared.h"
#include "../ps/ps.h"
#include "rt.h"
#include <dbg/print/kdprint.h>

HANDLE ObpAllocateHandle(EPROCESS *Proc, OBJECT_HEADER *Hdr, uint32_t Access)
{
    if (!Proc || !Hdr) return (HANDLE)0;

    for (uint32_t i = 0; i < PS_MAX_HANDLES; i++) {
        if (Proc->Handles[i].Object == 0) {
            Proc->Handles[i].Object        = Hdr;
            Proc->Handles[i].GrantedAccess = Access;
            Hdr->HandleCount++;
            ObReferenceObject(OBJ_BODY(Hdr));
            return (HANDLE)(uintptr_t)(i + 1);
        }
    }

    KdPrintf("[Ob] %s: handle table full\n", Proc->Name);
    return (HANDLE)0;
}

void ObpFreeHandle(EPROCESS *Proc, HANDLE Handle)
{
    if (!Proc || Handle == 0) return;
    uint32_t idx = (uint32_t)((uintptr_t)Handle - 1);
    if (idx >= PS_MAX_HANDLES) return;

    HANDLE_ENTRY *e = &Proc->Handles[idx];
    if (!e->Object) return;

    OBJECT_HEADER *hdr = e->Object;
    e->Object        = 0;
    e->GrantedAccess = 0;

    if (hdr->HandleCount > 0) hdr->HandleCount--;

    ObDereferenceObject(OBJ_BODY(hdr));
}

OBJECT_HEADER *ObpLookupHandle(EPROCESS *Proc, HANDLE Handle,
                               OBJECT_TYPE *Expected)
{
    if (!Proc || Handle == 0) return 0;
    uint32_t idx = (uint32_t)((uintptr_t)Handle - 1);
    if (idx >= PS_MAX_HANDLES) return 0;

    HANDLE_ENTRY *e = &Proc->Handles[idx];
    if (!e->Object) return 0;
    if (!e->Object->Type) return 0;
    if (Expected && e->Object->Type != Expected) return 0;

    return e->Object;
}