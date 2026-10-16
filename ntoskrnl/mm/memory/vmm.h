#ifndef MM_VMM_H
#define MM_VMM_H

#include <stdint.h>

#define PTE_PRESENT     (1ULL << 0)
#define PTE_WRITABLE    (1ULL << 1)
#define PTE_USER        (1ULL << 2)
#define PTE_PWT         (1ULL << 3)
#define PTE_PCD         (1ULL << 4)
#define PTE_ACCESSED    (1ULL << 5)
#define PTE_DIRTY       (1ULL << 6)
#define PTE_PAGESIZE    (1ULL << 7)
#define PTE_GLOBAL      (1ULL << 8)
#define PTE_NX          (1ULL << 63)

#define PTE_ADDR_MASK   0x000FFFFFFFFFF000ULL

#define PML4_INDEX(v)   (((v) >> 39) & 0x1FF)
#define PDPT_INDEX(v)   (((v) >> 30) & 0x1FF)
#define PD_INDEX(v)     (((v) >> 21) & 0x1FF)
#define PT_INDEX(v)     (((v) >> 12) & 0x1FF)

#define VMM_IDENTITY_GB    4
#define VMM_IDENTITY_SIZE  ((uint64_t)VMM_IDENTITY_GB << 30)

#define USER_CODE_ADDR     0x0000008000000000ULL    /* 512 GB        */
#define USER_PARAMS_ADDR   0x0000008000100000ULL    /* 512 GB + 1 MB */
#define USER_STACK_TOP     0x0000008000400000ULL    /* 512 GB + 4 MB */
#define USER_STACK_MAX     0x0000000000200000ULL    /* 2 MB growth   */
#define USER_HEAP_BASE     0x0000008000600000ULL    /* 512 GB + 6 MB */
#define USER_HEAP_END      0x000000C000000000ULL    /* 768 GB        */
#define USER_PEB_ADDR      0x0000008000800000ULL    /* 512 GB + 8 MB  */
#define USER_TEB_ADDR      0x0000008000900000ULL    /* 512 GB + 9 MB  */

void     MiInitVmm(void);

uint64_t MmGetKernelPml4(void);
uint64_t MmGetCurrentPml4(void);
void     MmSwitchAddressSpace(uint64_t Pml4Phys);

uint64_t MmCreateAddressSpace(void);
void     MmFreeAddressSpace(uint64_t Pml4Phys);

int      MiMapPage(uint64_t Pml4Phys, uint64_t Virt, uint64_t Phys, uint64_t Flags);
int      MiUnmapPage(uint64_t Pml4Phys, uint64_t Virt, uint64_t *PhysOut);
int      MiIsMapped(uint64_t Pml4Phys, uint64_t Virt);

#endif