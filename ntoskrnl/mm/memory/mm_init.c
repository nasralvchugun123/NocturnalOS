
#include "../mm.h"
#include <rt.h>
#include <dbg/print/kdprint.h>

void MmInitSystem(BOOT_INFO *BootInfo)
{
    KdPrint("[MM] MmInitSystem: start\n");
    PmmInitSystem(BootInfo);
    MiInitVmm(BootInfo);
    KdPrint("[MM] MmInitSystem: ok\n");
}

void *MmAllocatePages(uint32_t PageCount)
{
    return PmmAllocatePages(PageCount);
}

void MmFreePages(void *Address, uint32_t PageCount)
{
    PmmFreePages(Address, PageCount);
}