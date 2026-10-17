#include "vmm.h"
#include "pmm.h"
#include <rt.h>
#include <dbg/print/kdprint.h>

static uint64_t gKernelPml4 = 0;

static inline uint64_t *page_ptr(uint64_t phys)
{
    return (uint64_t *)(uintptr_t)phys;
}

static void zero_page(uint64_t phys)
{
    uint64_t *p = page_ptr(phys);
    for (int i = 0; i < 512; i++) p[i] = 0;
}

uint64_t MmGetKernelPml4(void) { return gKernelPml4; }

uint64_t MmGetCurrentPml4(void)
{
    uint64_t cr3;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(cr3));
    return cr3 & PTE_ADDR_MASK;
}

void MmSwitchAddressSpace(uint64_t Pml4Phys)
{
    __asm__ volatile ("mov %0, %%cr3" :: "r"(Pml4Phys) : "memory");
}

static void fill_pd_2mb_kernel(uint64_t pd_phys, uint64_t base)
{
    uint64_t *pd = page_ptr(pd_phys);
    for (uint64_t i = 0; i < 512; i++) {
        uint64_t phys = base + (i << 21);
        pd[i] = (phys & PTE_ADDR_MASK)
              | PTE_PRESENT | PTE_WRITABLE | PTE_PAGESIZE;
    }
}

static uint64_t range_end(uint64_t base, uint64_t size)
{
    if (!base || !size) return 0;
    if (base > UINT64_MAX - size) return UINT64_MAX;
    return base + size;
}

static uint64_t max_u64(uint64_t a, uint64_t b)
{
    return (a > b) ? a : b;
}

void MiInitVmm(BOOT_INFO *BootInfo)
{
    const uint64_t one_gib = 0x40000000ULL;
    const uint64_t identity_limit = 1ULL << 39;
    uint64_t required_end = PmmGetTotalPages() << 12;
    uint64_t context_end = 0;

    KdPrint("[MM] MiInitVmm: start\n");
    KdPrintf("[MM] VMM: old CR3=0x%llx PMM highest=0x%llx\n",
             (unsigned long long)MmGetCurrentPml4(),
             (unsigned long long)(PmmGetTotalPages() << 12));

    if (BootInfo) {
        context_end = max_u64(context_end,
            range_end(BootInfo->KernelBase, BootInfo->KernelSize));
        context_end = max_u64(context_end, BootInfo->StackTop);
        context_end = max_u64(context_end,
            range_end((uint64_t)(uintptr_t)BootInfo, sizeof(*BootInfo)));
        uint64_t memory_map_end = range_end(BootInfo->MemoryMap,
                                            BootInfo->MemoryMapSize);
        if (memory_map_end && memory_map_end < identity_limit)
            context_end = max_u64(context_end, memory_map_end);
        else if (memory_map_end >= identity_limit)
            KdPrintf("[MM] VMM: EFI memory map buffer above 512 GiB; PMM init already consumed it\n");
        if (BootInfo->FramebufferBase && BootInfo->FramebufferSize) {
            uint64_t fb_end = range_end(BootInfo->FramebufferBase,
                                        BootInfo->FramebufferSize);
            if (fb_end <= identity_limit)
                context_end = max_u64(context_end, fb_end);
            else
                KdPrintf("[MM] VMM: framebuffer range exceeds 512 GiB map: base=0x%llx size=0x%llx\n",
                         (unsigned long long)BootInfo->FramebufferBase,
                         (unsigned long long)BootInfo->FramebufferSize);
        }
    }
    required_end = max_u64(required_end, context_end);

    if (context_end >= identity_limit) {
        KdPrintf("[MM] FATAL: active boot context reaches 0x%llx; bootstrap map limit is 0x%llx\n",
                 (unsigned long long)context_end,
                 (unsigned long long)identity_limit);
        for (;;) __asm__ volatile ("cli; hlt");
    }

    uint64_t identity_gb = required_end / one_gib;
    if ((required_end % one_gib) != 0) identity_gb++;
    if (identity_gb < VMM_IDENTITY_GB_MIN)
        identity_gb = VMM_IDENTITY_GB_MIN;
    if (identity_gb > VMM_IDENTITY_GB_MAX) {
        KdPrintf("[MM] VMM: physical range 0x%llx exceeds 512 GiB; initial map capped\n",
                 (unsigned long long)required_end);
        identity_gb = VMM_IDENTITY_GB_MAX;
    }
    KdPrintf("[MM] VMM: identity map target=%llu GiB (kernel=0x%llx+0x%llx stack_top=0x%llx)\n",
             (unsigned long long)identity_gb,
             (unsigned long long)(BootInfo ? BootInfo->KernelBase : 0),
             (unsigned long long)(BootInfo ? BootInfo->KernelSize : 0),
             (unsigned long long)(BootInfo ? BootInfo->StackTop : 0));

    KdPrint("[MM] VMM step 1: allocate PML4\n");
    uint64_t pml4_phys = (uint64_t)(uintptr_t)PmmAllocatePage();
    KdPrintf("[MM] VMM PML4 phys=0x%llx\n", (unsigned long long)pml4_phys);
    KdPrint("[MM] VMM step 2: allocate PDPT\n");
    uint64_t pdpt_phys = (uint64_t)(uintptr_t)PmmAllocatePage();
    KdPrintf("[MM] VMM PDPT phys=0x%llx\n", (unsigned long long)pdpt_phys);
    if (!pml4_phys || !pdpt_phys) {
        KdPrint("[MM] MiInitVmm: PML4/PDPT alloc failed\n");
        for (;;) __asm__ volatile ("cli; hlt");
    }

    KdPrint("[MM] VMM step 3: zero PML4\n");
    zero_page(pml4_phys);
    KdPrint("[MM] VMM step 4: zero PDPT\n");
    zero_page(pdpt_phys);

    uint64_t *pml4 = page_ptr(pml4_phys);
    uint64_t *pdpt = page_ptr(pdpt_phys);

    for (uint64_t gb = 0; gb < identity_gb; gb++) {
        uint64_t pd_phys = (uint64_t)(uintptr_t)PmmAllocatePage();
        if (!pd_phys) {
            KdPrintf("[MM] MiInitVmm: PD alloc failed at GB=%llu\n",
                     (unsigned long long)gb);
            for (;;) __asm__ volatile ("cli; hlt");
        }
        if ((gb & 63) == 0)
            KdPrintf("[MM] VMM PD[%llu] phys=0x%llx\n",
                     (unsigned long long)gb,
                     (unsigned long long)pd_phys);
        zero_page(pd_phys);
        fill_pd_2mb_kernel(pd_phys, gb * one_gib);

        pdpt[gb] = (pd_phys & PTE_ADDR_MASK)
                 | PTE_PRESENT | PTE_WRITABLE;
    }

    pml4[0] = (pdpt_phys & PTE_ADDR_MASK)
            | PTE_PRESENT | PTE_WRITABLE;

    gKernelPml4 = pml4_phys;

    KdPrintf("[MM] VMM switching CR3: old=0x%llx new=0x%llx mapped=0..0x%llx\n",
             (unsigned long long)MmGetCurrentPml4(),
             (unsigned long long)gKernelPml4,
             (unsigned long long)(identity_gb * one_gib));
    MmSwitchAddressSpace(gKernelPml4);

    KdPrintf("[MM] VMM: CR3=0x%llx, identity 0..%llu GiB kernel-only\n",
             (unsigned long long)MmGetCurrentPml4(),
             (unsigned long long)identity_gb);
}

static uint64_t *walk_create_pd(uint64_t pml4_phys, uint64_t virt)
{
    uint64_t *pml4 = page_ptr(pml4_phys);

    uint64_t pi    = PML4_INDEX(virt);
    uint64_t pdpti = PDPT_INDEX(virt);
    uint64_t pdi   = PD_INDEX(virt);

    if (!(pml4[pi] & PTE_PRESENT)) {
        uint64_t pdpt = (uint64_t)(uintptr_t)PmmAllocatePage();
        if (!pdpt) return 0;
        zero_page(pdpt);
        pml4[pi] = (pdpt & PTE_ADDR_MASK)
                 | PTE_PRESENT | PTE_WRITABLE | PTE_USER;
    }
    uint64_t *pdpt = page_ptr(pml4[pi] & PTE_ADDR_MASK);

    if (!(pdpt[pdpti] & PTE_PRESENT)) {
        uint64_t pd = (uint64_t)(uintptr_t)PmmAllocatePage();
        if (!pd) return 0;
        zero_page(pd);
        pdpt[pdpti] = (pd & PTE_ADDR_MASK)
                    | PTE_PRESENT | PTE_WRITABLE | PTE_USER;
    }
    uint64_t *pd = page_ptr(pdpt[pdpti] & PTE_ADDR_MASK);

    if (pd[pdi] & PTE_PAGESIZE) {
        uint64_t new_pt = (uint64_t)(uintptr_t)PmmAllocatePage();
        if (!new_pt) return 0;
        uint64_t *pt = page_ptr(new_pt);

        uint64_t base_phys = pd[pdi] & PTE_ADDR_MASK;
        uint64_t flags = pd[pdi] & ~PTE_ADDR_MASK & ~PTE_PAGESIZE;

        for (int i = 0; i < 512; i++) {
            pt[i] = (base_phys + (uint64_t)i * 0x1000) | flags;
        }

        pd[pdi] = (new_pt & PTE_ADDR_MASK)
                | PTE_PRESENT | PTE_WRITABLE | PTE_USER;

        uint64_t aligned = virt & ~0x1FFFFFULL;
        __asm__ volatile ("invlpg (%0)" :: "r"(aligned) : "memory");
    }

    if (!(pd[pdi] & PTE_PRESENT)) {
        uint64_t pt = (uint64_t)(uintptr_t)PmmAllocatePage();
        if (!pt) return 0;
        zero_page(pt);
        pd[pdi] = (pt & PTE_ADDR_MASK)
                | PTE_PRESENT | PTE_WRITABLE | PTE_USER;
    }
    return pd;
}

int MiMapPage(uint64_t Pml4Phys, uint64_t Virt, uint64_t Phys, uint64_t Flags)
{
    if (!Pml4Phys) Pml4Phys = MmGetCurrentPml4();

    uint64_t *pd = walk_create_pd(Pml4Phys, Virt);
    if (!pd) return -1;

    uint64_t pt_phys = pd[PD_INDEX(Virt)] & PTE_ADDR_MASK;
    uint64_t *pt = page_ptr(pt_phys);

    pt[PT_INDEX(Virt)] = (Phys & PTE_ADDR_MASK)
                       | PTE_PRESENT | PTE_WRITABLE
                       | (Flags & (PTE_USER | PTE_NX | PTE_PCD |
                                  PTE_PWT  | PTE_GLOBAL));

    __asm__ volatile ("invlpg (%0)" :: "r"(Virt) : "memory");
    return 0;
}

int MiUnmapPage(uint64_t Pml4Phys, uint64_t Virt, uint64_t *PhysOut)
{
    if (PhysOut) *PhysOut = 0;
    if (!Pml4Phys) Pml4Phys = MmGetCurrentPml4();

    uint64_t *pml4 = page_ptr(Pml4Phys);
    uint64_t pi = PML4_INDEX(Virt);
    if (!(pml4[pi] & PTE_PRESENT)) return -1;

    uint64_t *pdpt = page_ptr(pml4[pi] & PTE_ADDR_MASK);
    uint64_t pdpti = PDPT_INDEX(Virt);
    if (!(pdpt[pdpti] & PTE_PRESENT)) return -1;

    uint64_t *pd = page_ptr(pdpt[pdpti] & PTE_ADDR_MASK);
    uint64_t pdi = PD_INDEX(Virt);
    if (!(pd[pdi] & PTE_PRESENT)) return -1;
    if (pd[pdi] & PTE_PAGESIZE)   return -1;

    uint64_t *pt = page_ptr(pd[pdi] & PTE_ADDR_MASK);
    uint64_t pti = PT_INDEX(Virt);

    if (!(pt[pti] & PTE_PRESENT)) return -1;

    if (PhysOut && (pt[pti] & PTE_USER))
        *PhysOut = pt[pti] & PTE_ADDR_MASK;

    pt[pti] = 0;
    __asm__ volatile ("invlpg (%0)" :: "r"(Virt) : "memory");
    return 0;
}

int MiIsMapped(uint64_t Pml4Phys, uint64_t Virt)
{
    if (!Pml4Phys) Pml4Phys = MmGetCurrentPml4();

    uint64_t *pml4 = page_ptr(Pml4Phys);
    uint64_t pi = PML4_INDEX(Virt);
    if (!(pml4[pi] & PTE_PRESENT)) return 0;

    uint64_t *pdpt = page_ptr(pml4[pi] & PTE_ADDR_MASK);
    uint64_t pdpti = PDPT_INDEX(Virt);
    if (!(pdpt[pdpti] & PTE_PRESENT)) return 0;

    uint64_t *pd = page_ptr(pdpt[pdpti] & PTE_ADDR_MASK);
    uint64_t pdi = PD_INDEX(Virt);
    if (pd[pdi] & PTE_PAGESIZE) return 1;
    if (!(pd[pdi] & PTE_PRESENT)) return 0;

    uint64_t *pt = page_ptr(pd[pdi] & PTE_ADDR_MASK);
    return (pt[PT_INDEX(Virt)] & PTE_PRESENT) ? 1 : 0;
}

uint64_t MmCreateAddressSpace(void)
{
    uint64_t pml4_phys = (uint64_t)(uintptr_t)PmmAllocatePage();
    if (!pml4_phys) return 0;
    zero_page(pml4_phys);

    uint64_t *pml4 = page_ptr(pml4_phys);
    uint64_t *ker  = page_ptr(gKernelPml4);

    pml4[0] = ker[0];

    KdPrintf("[MM] MmCreateAddressSpace: pml4=0x%llx\n",
             (unsigned long long)pml4_phys);
    return pml4_phys;
}

void MmFreeAddressSpace(uint64_t Pml4Phys)
{
    if (!Pml4Phys) return;
    if (Pml4Phys == gKernelPml4) return;

    uint64_t *pml4 = page_ptr(Pml4Phys);

    for (int pi = 1; pi < 512; pi++) {
        if (!(pml4[pi] & PTE_PRESENT)) continue;

        uint64_t pdpt_phys = pml4[pi] & PTE_ADDR_MASK;
        uint64_t *pdpt = page_ptr(pdpt_phys);

        for (int pdpti = 0; pdpti < 512; pdpti++) {
            if (!(pdpt[pdpti] & PTE_PRESENT)) continue;
            if (pdpt[pdpti] & PTE_PAGESIZE)   continue;

            uint64_t pd_phys = pdpt[pdpti] & PTE_ADDR_MASK;
            uint64_t *pd = page_ptr(pd_phys);

            for (int pdi = 0; pdi < 512; pdi++) {
                if (!(pd[pdi] & PTE_PRESENT)) continue;
                if (pd[pdi] & PTE_PAGESIZE)   continue;

                uint64_t pt_phys = pd[pdi] & PTE_ADDR_MASK;
                uint64_t *pt = page_ptr(pt_phys);

                for (int pti = 0; pti < 512; pti++) {
                    if (!(pt[pti] & PTE_PRESENT)) continue;
                    if (!(pt[pti] & PTE_USER))    continue;
                    PmmFreePage((void *)(uintptr_t)(pt[pti] & PTE_ADDR_MASK));
                }
                PmmFreePage((void *)(uintptr_t)pt_phys);
            }
            PmmFreePage((void *)(uintptr_t)pd_phys);
        }
        PmmFreePage((void *)(uintptr_t)pdpt_phys);
    }
    PmmFreePage((void *)(uintptr_t)Pml4Phys);
}