#include "pmm.h"
#include <rt.h>
#include <dbg/print/kdprint.h>
#include <ke/spinlock.h>

#define BITMAP_SET(b,i)  ((b)[(i)>>3] |=  (uint8_t)(1u << ((i)&7)))
#define BITMAP_CLR(b,i)  ((b)[(i)>>3] &= (uint8_t)~(1u << ((i)&7)))
#define BITMAP_GET(b,i)  (((b)[(i)>>3] >> ((i)&7)) & 1u)

static KSPINLOCK gPmmLock = KSPINLOCK_INIT;

static uint8_t  *gBitmap      = 0;
static uint64_t  gTotalPages  = 0;
static uint64_t  gFreePages   = 0;
static uint64_t  gHighestPhys = 0;
static uint64_t  gLastAlloc   = 0;

uint64_t PmmGetTotalPages(void) { return gTotalPages; }
uint64_t PmmGetFreePages(void)  { return gFreePages;  }
uint64_t PmmGetUsedPages(void)  { return gTotalPages - gFreePages; }

void PmmFreePage(void *page) { PmmFreePages(page, 1); }
void *PmmAllocatePage(void) { return PmmAllocatePages(1); }


static inline BOOT_MEMORY_DESCRIPTOR *desc_at(uint8_t *base, uint64_t stride, uint64_t idx)
{
    return (BOOT_MEMORY_DESCRIPTOR *)(base + idx * stride);
}

static void mark_used(uint64_t base, uint64_t size)
{
    uint64_t first = base >> 12;
    uint64_t last  = (base + size + PMM_PAGE_MASK) >> 12;
    if (last > gTotalPages) last = gTotalPages;
    for (uint64_t p = first; p < last; p++) {
        if (!BITMAP_GET(gBitmap, p)) { BITMAP_SET(gBitmap, p); gFreePages--; }
    }
}

static void mark_free(uint64_t base, uint64_t size)
{
    uint64_t first = (base + PMM_PAGE_MASK) >> 12;
    uint64_t last  = (base + size) >> 12;
    if (last > gTotalPages) last = gTotalPages;
    for (uint64_t p = first; p < last; p++) {
        if (BITMAP_GET(gBitmap, p)) { BITMAP_CLR(gBitmap, p); gFreePages++; }
    }
}

void PmmInitSystem(BOOT_INFO *bi)
{
    KdPrint("[MM] PmmInitSystem: start\n");

    if (!bi || !bi->MemoryMap || !bi->MemoryMapDescriptorSize) {
        KdPrint("[MM] PMM: no memory map, halting\n");
        for (;;) __asm__ volatile ("hlt");
    }

    uint8_t *map_base = (uint8_t *)(uintptr_t)bi->MemoryMap;
    uint64_t stride   = bi->MemoryMapDescriptorSize;
    uint64_t count    = bi->MemoryMapSize / stride;

    for (uint64_t i = 0; i < count; i++) {
        BOOT_MEMORY_DESCRIPTOR *d = desc_at(map_base, stride, i);
        if (d->Type != BOOT_MEM_CONVENTIONAL) continue;

        uint64_t end = d->PhysicalStart + (d->NumberOfPages << 12);
        if (end > gHighestPhys) gHighestPhys = end;
    }

    gTotalPages = gHighestPhys >> 12;
    uint64_t bitmap_size = (gTotalPages + 7) / 8;
    bitmap_size = (bitmap_size + PMM_PAGE_MASK) & ~PMM_PAGE_MASK;

    KdPrintf("[MM] PMM: highest phys = 0x%llx (%llu MB)\n",
             (unsigned long long)gHighestPhys,
             (unsigned long long)(gHighestPhys / (1024 * 1024)));
    KdPrintf("[MM] PMM: total pages = %llu, bitmap = %llu bytes\n",
             (unsigned long long)gTotalPages,
             (unsigned long long)bitmap_size);

    uint64_t bitmap_phys  = 0;
    int      bitmap_found = 0;
    for (uint64_t i = 0; i < count; i++) {
        BOOT_MEMORY_DESCRIPTOR *d = desc_at(map_base, stride, i);
        if (d->Type != BOOT_MEM_CONVENTIONAL) continue;
        if (d->PhysicalStart < 0x100000ULL) continue;
        uint64_t sz = d->NumberOfPages << 12;
        if (sz >= bitmap_size) {
            bitmap_phys  = d->PhysicalStart;
            bitmap_found = 1;
            break;
        }
    }
    if (!bitmap_found) {
        KdPrint("[MM] PMM: no room for bitmap\n");
        for (;;) __asm__ volatile ("hlt");
    }
    KdPrintf("[MM] PMM: picking bitmap region at 0x%llx\n",
             (unsigned long long)bitmap_phys);

    gBitmap = (uint8_t *)(uintptr_t)bitmap_phys;
    for (uint64_t i = 0; i < bitmap_size; i++) gBitmap[i] = 0xFF;
    gFreePages = 0;

    for (uint64_t i = 0; i < count; i++) {
        BOOT_MEMORY_DESCRIPTOR *d = desc_at(map_base, stride, i);
        uint64_t base = d->PhysicalStart;
        uint64_t sz   = d->NumberOfPages << 12;
        if (d->Type == BOOT_MEM_CONVENTIONAL) mark_free(base, sz);
        else                                  mark_used(base, sz);
    }

    mark_used(bitmap_phys, bitmap_size);

    mark_used(0, 0x100000);

    if (bi->KernelBase && bi->KernelSize)
        mark_used(bi->KernelBase, bi->KernelSize);
    if (bi->StackTop)
        mark_used(bi->StackTop - 0x10000, 0x10000);

    KdPrintf("[MM] PMM: bitmap at phys 0x%llx\n",
             (unsigned long long)bitmap_phys);
    KdPrintf("[MM] PMM: total=%llu free=%llu used=%llu (%llu MB free)\n",
             (unsigned long long)gTotalPages,
             (unsigned long long)gFreePages,
             (unsigned long long)(gTotalPages - gFreePages),
             (unsigned long long)((gFreePages << 12) / (1024 * 1024)));
    KdPrint("[MM] PmmInitSystem: ok\n");
}

void *PmmAllocatePages(uint32_t count)
{
    if (count == 0) return 0;

    uint64_t f = KeAcquireSpinLockIrqSave(&gPmmLock);

    for (uint64_t pass = 0; pass < 2; pass++) {
        uint64_t start = (pass == 0) ? gLastAlloc : 0;
        uint64_t end   = (pass == 0) ? gTotalPages : gLastAlloc;

        uint64_t run = 0, run_start = 0;
        for (uint64_t p = start; p < end; p++) {
            if (!BITMAP_GET(gBitmap, p)) {
                if (run == 0) run_start = p;
                run++;
                if (run == count) {
                    for (uint64_t k = run_start; k < run_start + count; k++)
                        BITMAP_SET(gBitmap, k);
                    gFreePages -= count;
                    gLastAlloc = run_start + count;
                    KeReleaseSpinLockIrqRestore(&gPmmLock, f);
                    return (void *)(uintptr_t)(run_start << 12);
                }
            } else {
                run = 0;
            }
        }
    }
    KeReleaseSpinLockIrqRestore(&gPmmLock, f);
    return 0;
}

void PmmFreePages(void *pages, uint32_t count)
{
    if (!pages || !count) return;
    uint64_t f = KeAcquireSpinLockIrqSave(&gPmmLock);
    uint64_t first = ((uint64_t)(uintptr_t)pages) >> 12;
    for (uint32_t i = 0; i < count; i++) {
        uint64_t p = first + i;
        if (p >= gTotalPages) break;
        if (BITMAP_GET(gBitmap, p)) { BITMAP_CLR(gBitmap, p); gFreePages++; }
    }
    KeReleaseSpinLockIrqRestore(&gPmmLock, f);
}