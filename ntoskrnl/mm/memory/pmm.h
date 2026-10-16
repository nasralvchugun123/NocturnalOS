
#ifndef MM_PMM_H
#define MM_PMM_H

#include <stdint.h>
#include <stddef.h>
#include <boot_info.h>

#define PMM_PAGE_SIZE 4096ULL
#define PMM_PAGE_MASK (PMM_PAGE_SIZE - 1ULL)

void      PmmInitSystem(BOOT_INFO *BootInfo);

void     *PmmAllocatePage(void);
void     *PmmAllocatePages(uint32_t Count);
void      PmmFreePage(void *Page);
void      PmmFreePages(void *Pages, uint32_t Count);

uint64_t  PmmGetTotalPages(void);
uint64_t  PmmGetFreePages(void);
uint64_t  PmmGetUsedPages(void);

#endif