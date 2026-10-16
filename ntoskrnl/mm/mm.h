
#ifndef MM_H
#define MM_H

#include <stdint.h>
#include <stddef.h>
#include "boot_info.h"
#include "memory/pmm.h"
#include "memory/vmm.h"

void  MmInitSystem(BOOT_INFO *BootInfo);

void *MmAllocatePages(uint32_t PageCount);
void  MmFreePages(void *Address, uint32_t PageCount);

#endif