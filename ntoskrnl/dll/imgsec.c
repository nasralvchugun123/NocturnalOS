#include "imgsec.h"
#include "../mm/mm.h"
#include <rt.h>
#include <dbg/print/kdprint.h>

extern void  *PmmAllocatePage(void);
extern void   PmmFreePage(void *Page);

static IMAGE_SECTION_ENTRY gSections[IMG_SEC_MAX];

static int StrEq(const char *a, const char *b)
{ while (*a && *a == *b) { a++; b++; } return *a == *b; }

static void StrCopyN(char *d, const char *s, uint32_t cap)
{
    uint32_t i = 0;
    while (s[i] && i + 1 < cap) { d[i] = s[i]; i++; }
    d[i] = 0;
}

void ImgSecInit(void)
{
    for (int i = 0; i < IMG_SEC_MAX; i++) gSections[i].InUse = 0;
    KdPrint("[ImgSec] table ready\n");
}

IMAGE_SECTION_ENTRY *ImgSecFind(const char *Name)
{
    for (int i = 0; i < IMG_SEC_MAX; i++)
        if (gSections[i].InUse && StrEq(gSections[i].Name, Name))
            return &gSections[i];
    return 0;
}

IMAGE_SECTION_ENTRY *ImgSecFindByBase(uint64_t Base)
{
    for (int i = 0; i < IMG_SEC_MAX; i++)
        if (gSections[i].InUse && gSections[i].Base == Base)
            return &gSections[i];
    return 0;
}

static IMAGE_SECTION_ENTRY *AllocSlot(void)
{
    for (int i = 0; i < IMG_SEC_MAX; i++)
        if (!gSections[i].InUse) return &gSections[i];
    return 0;
}
static int RangeFree(uint64_t Base, uint64_t Size)
{
    for (int i = 0; i < IMG_SEC_MAX; i++) {
        if (!gSections[i].InUse) continue;
        uint64_t a0 = gSections[i].Base;
        uint64_t a1 = a0 + gSections[i].Size;
        uint64_t b0 = Base;
        uint64_t b1 = Base + Size;
        if (b0 < a1 && b1 > a0) return 0;
    }
    return 1;
}

NTSTATUS ImgSecCreate(const char *Name, const void *Flattened,
                      uint64_t Size, uint64_t PreferredBase,
                      IMAGE_SECTION_ENTRY **Out)
{
    if (!Name || !Flattened || !Size || !Out) return STATUS_INVALID_PARAMETER;
    if (ImgSecFind(Name)) return STATUS_OBJECT_NAME_COLLISION;

    uint32_t pages = (uint32_t)((Size + 4095) / 4096);
    if (pages == 0 || pages > IMG_SEC_MAX_PAGES) return STATUS_INVALID_PARAMETER;

    uint64_t base = PreferredBase;
    if (!base) {
        return STATUS_INVALID_PARAMETER;
    }
    if (base & 0xFFF) return STATUS_INVALID_PARAMETER;
    if (!RangeFree(base, (uint64_t)pages * 4096)) return STATUS_OBJECT_NAME_COLLISION;

    IMAGE_SECTION_ENTRY *e = AllocSlot();
    if (!e) return STATUS_INSUFFICIENT_RESOURCES;

    for (uint32_t i = 0; i < pages; i++) {
        void *pg = PmmAllocatePage();
        if (!pg) {
            for (uint32_t j = 0; j < i; j++) PmmFreePage((void *)(uintptr_t)e->PhysPages[j]);
            return STATUS_NO_MEMORY;
        }
        uint8_t *p8 = (uint8_t *)pg;
        for (int k = 0; k < 4096; k++) p8[k] = 0;

        const uint8_t *src = (const uint8_t *)Flattened + (uint64_t)i * 4096;
        uint64_t left = Size - (uint64_t)i * 4096;
        uint64_t copy = left >= 4096 ? 4096 : left;
        for (uint64_t k = 0; k < copy; k++) p8[k] = src[k];

        e->PhysPages[i] = (uint64_t)(uintptr_t)pg;
    }

    StrCopyN(e->Name, Name, IMG_SEC_NAME_MAX);
    e->Base       = base;
    e->Size       = Size;
    e->PageCount  = pages;
    e->RefCount   = 0;
    e->MappedCount = 0;
    e->InUse      = 1;

    *Out = e;
    KdPrintf("[ImgSec] created '%s' base=0x%llx size=0x%llx pages=%u\n",
             Name, (unsigned long long)base, (unsigned long long)Size, pages);
    return STATUS_SUCCESS;
}

NTSTATUS ImgSecMap(IMAGE_SECTION_ENTRY *E, uint64_t Pml4)
{
    if (!E || !Pml4) return STATUS_INVALID_PARAMETER;

    for (uint32_t i = 0; i < E->MappedCount; i++)
        if (E->MappedPml4[i] == (uint32_t)(Pml4 >> 12))
            return STATUS_SUCCESS;  

    for (uint32_t i = 0; i < E->PageCount; i++) {
        uint64_t va = E->Base + (uint64_t)i * 4096;
        if (MiMapPage(Pml4, va, E->PhysPages[i], PTE_PRESENT|PTE_WRITABLE|PTE_USER) != 0)
            return STATUS_UNSUCCESSFUL;
    }

    if (E->MappedCount < IMG_SEC_MAX_MAPS) {
        E->MappedPml4[E->MappedCount++] = (uint32_t)(Pml4 >> 12);
    }
    E->RefCount++;
    return STATUS_SUCCESS;
}

NTSTATUS ImgSecUnmap(IMAGE_SECTION_ENTRY *E, uint64_t Pml4)
{
    if (!E || !Pml4) return STATUS_INVALID_PARAMETER;

    int found = 0;
    for (uint32_t i = 0; i < E->MappedCount; i++) {
        if (E->MappedPml4[i] == (uint32_t)(Pml4 >> 12)) {
            for (uint32_t j = i; j + 1 < E->MappedCount; j++)
                E->MappedPml4[j] = E->MappedPml4[j + 1];
            E->MappedCount--;
            found = 1;
            break;
        }
    }
    if (!found) return STATUS_NOT_FOUND;

    for (uint32_t i = 0; i < E->PageCount; i++) {
        uint64_t va = E->Base + (uint64_t)i * 4096;
        uint64_t phys = 0;
        MiUnmapPage(Pml4, va, &phys);
    }

    if (E->RefCount > 0) E->RefCount--;
    if (E->RefCount == 0) {
        for (uint32_t i = 0; i < E->PageCount; i++)
            PmmFreePage((void *)(uintptr_t)E->PhysPages[i]);
        KdPrintf("[ImgSec] freed '%s'\n", E->Name);
        E->InUse = 0;
    }
    return STATUS_SUCCESS;
}

void ImgSecReleaseAllForPml4(uint64_t Pml4)
{
    for (int i = 0; i < IMG_SEC_MAX; i++) {
        IMAGE_SECTION_ENTRY *e = &gSections[i];
        if (!e->InUse) continue;
        for (uint32_t j = 0; j < e->MappedCount; j++) {
            if (e->MappedPml4[j] == (uint32_t)(Pml4 >> 12)) {
                ImgSecUnmap(e, Pml4);
                break;
            }
        }
    }
}