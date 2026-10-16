







/*
мне лень читать че это так что пускай тут будет он даже не собирается 
*/

/*
#include "../../ntoskrnl.h"
#include <stdio.h>
#include <string.h>

#define MM_MAX_PAGES 2048
#define MM_PAGE_SIZE 4096

typedef struct {
    void *Address;
    uint32_t Size;
    uint32_t Flags;
    uint8_t InUse;
} MM_PAGE_ENTRY;

static MM_PAGE_ENTRY gPageTable[MM_MAX_PAGES];
static uint32_t gPageCount = 0;
static uint32_t gTotalAllocated = 0;

void MmInitializeMemoryManager(void)
{
    gPageCount = 0;
    gTotalAllocated = 0;
    memset(gPageTable, 0, sizeof(gPageTable));
    printf("[MM] Memory Manager initialized - Max %d pages (%dMB)\n", 
           MM_MAX_PAGES, (MM_MAX_PAGES * MM_PAGE_SIZE) / (1024 * 1024));
}

void *MmAllocatePages(uint32_t PageCount)
{
    if (gPageCount + PageCount > MM_MAX_PAGES) {
        printf("[MM] Error: Not enough pages (requested %u, available %u)\n", 
               PageCount, MM_MAX_PAGES - gPageCount);
        return NULL;
    }
    
    // Find contiguous free pages 
    uint32_t free_count = 0;
    uint32_t start_idx = 0;
    
    for (uint32_t i = 0; i < MM_MAX_PAGES; i++) {
        if (!gPageTable[i].InUse) {
            if (free_count == 0)
                start_idx = i;
            free_count++;
            if (free_count == PageCount)
                break;
        } else {
            free_count = 0;
        }
    }
    
    if (free_count < PageCount) {
        printf("[MM] Error: Cannot find %u contiguous free pages\n", PageCount);
        return NULL;
    }
    
    // Allocate pages 
    void *AllocAddr = (void *)(uintptr_t)(0x100000ULL + ((uintptr_t)start_idx * MM_PAGE_SIZE));
    uint32_t AllocSize = PageCount * MM_PAGE_SIZE;
    
    for (uint32_t i = start_idx; i < start_idx + PageCount; i++) {
        gPageTable[i].Address = (void *)((uintptr_t)AllocAddr + ((i - start_idx) * MM_PAGE_SIZE));
        gPageTable[i].Size = MM_PAGE_SIZE;
        gPageTable[i].Flags = 0x03;  // Present + Writable 
        gPageTable[i].InUse = 1;
    }
    
    gPageCount += PageCount;
    gTotalAllocated += AllocSize;
    
    printf("[MM] Allocated %u pages (%u bytes) at %p\n", PageCount, AllocSize, AllocAddr);
    return AllocAddr;
}

void MmFreePages(void *Address, uint32_t PageCount)
{
    if (!Address) {
        printf("[MM] Error: Freeing NULL pointer\n");
        return;
    }
    
    // Find and free pages
    uint32_t freed = 0;
    for (uint32_t i = 0; i < MM_MAX_PAGES && freed < PageCount; i++) {
        if (gPageTable[i].InUse && gPageTable[i].Address == Address) {
            gPageTable[i].InUse = 0;
            gPageCount--;
            gTotalAllocated -= gPageTable[i].Size;
            freed++;
            Address = (void *)((uintptr_t)Address + MM_PAGE_SIZE);
        }
    }
    
    if (freed > 0) {
        printf("[MM] Freed %u pages\n", freed);
    }
}

// Get memory statistics 
uint32_t MmGetTotalMemory(void)
{
    return MM_MAX_PAGES * MM_PAGE_SIZE;
}

uint32_t MmGetUsedMemory(void)
{
    return gTotalAllocated;
}

uint32_t MmGetFreeMemory(void)
{
    return MmGetTotalMemory() - MmGetUsedMemory();
}

*/