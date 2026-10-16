
#include "shared.h"
#include <rt.h>
#include <dbg/print/kdprint.h>

extern void PmmFreePages(void *Address, uint32_t Count);

void ObReferenceObject(void *Body)
{
    if (!Body) return;
    OBJECT_HEADER *hdr = OBJ_HEADER(Body);
    if (!hdr->Type) return;
    __sync_fetch_and_add(&hdr->RefCount, 1);
}

void ObDereferenceObject(void *Body)
{
    if (!Body) return;
    OBJECT_HEADER *hdr = OBJ_HEADER(Body);
    if (!hdr->Type) return;

    int32_t prev = __sync_fetch_and_sub(&hdr->RefCount, 1);
    if (prev > 1) return;  

    OBJECT_TYPE *type  = hdr->Type;
    uint32_t     pages = hdr->PageCount;

    KdPrintf("[Ob] -%s hdr=0x%llx pages=%u\n",
             type->Name, (unsigned long long)(uintptr_t)hdr, pages);

    if (type->DeleteProcedure)
        type->DeleteProcedure(Body);

    hdr->Type = 0;
    if (pages > 0)
        PmmFreePages(hdr, pages);
}