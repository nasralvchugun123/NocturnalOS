

#include "shared.h"
#include <rt.h>
#include <dbg/print/kdprint.h>
#include <ds/fb/fb.h>

extern void *PmmAllocatePages(uint32_t PageCount);


void *ObCreateObject(OBJECT_TYPE *Type, size_t BodySize)
{


    if (!Type || BodySize == 0) return 0;

    size_t total = sizeof(OBJECT_HEADER) + BodySize;
    uint32_t pages = (uint32_t)((total + 0xFFF) / 0x1000);
    if (pages == 0) pages = 1;

    void *mem = PmmAllocatePages(pages);
    if (!mem) {
        KdPrintf("[Ob] ObCreateObject(%s): alloc %u pages failed\n",
                 Type->Name, pages);
        return 0;
    }
    for (uint32_t i = 0; i < pages * 0x1000; i++)
        ((uint8_t *)mem)[i] = 0;

    OBJECT_HEADER *hdr = (OBJECT_HEADER *)mem;
    hdr->Type        = Type;
    hdr->RefCount    = 1;
    hdr->HandleCount = 0;
    hdr->SignalState = 0;
    hdr->PageCount   = (uint16_t)pages;
    hdr->WaiterHead  = 0;

    KdPrintf("[Ob] +%s hdr=0x%llx body=0x%llx pages=%u\n",
             Type->Name,
             (unsigned long long)(uintptr_t)hdr,
             (unsigned long long)(uintptr_t)OBJ_BODY(hdr),
             pages);

    return OBJ_BODY(hdr);
}