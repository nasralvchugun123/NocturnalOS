#include "nvme.h"
#include "../../../hal/halx64/PCI/pci.h"
#include <rt.h>
#include "../../../ntoskrnl/mm/mm.h"
#include <dbg/print/kdprint.h>
#include <ds/fb/fb.h>

#define NVME_CAP    0x00
#define NVME_VS     0x08
#define NVME_INTMS  0x0C
#define NVME_INTMC  0x10
#define NVME_CC     0x14
#define NVME_CSTS   0x1C
#define NVME_AQA    0x24
#define NVME_ASQ    0x28
#define NVME_ACQ    0x30

#define CC_EN         (1u << 0)
#define CC_CSS_NVM    (0u << 4)
#define CC_MPS_4K     (0u << 7)
#define CC_AMS_RR     (0u << 11)
#define CC_IOSQES_64  (6u << 16)
#define CC_IOCQES_16  (4u << 20)

#define CSTS_RDY   (1u << 0)
#define CSTS_CFS   (1u << 1)

#define ADM_CREATE_SQ   0x01
#define ADM_CREATE_CQ   0x05
#define ADM_IDENTIFY    0x06
#define ADM_SET_FEAT    0x09

#define IO_WRITE   0x01
#define IO_READ    0x02

#define IDENTIFY_NSID   0x00
#define IDENTIFY_CTRL   0x01

#define ADMIN_Q_DEPTH   16
#define IO_Q_DEPTH      16
#define PAGE_SIZE       4096

#define FEAT_NUM_QUEUES 0x07

#define NVME_MMIO_VIRT   0x90000000ULL   
#define NVME_MMIO_SIZE   0x4000          
typedef struct {
    uint32_t cdw0;
    uint32_t nsid;
    uint32_t cdw2, cdw3;
    uint64_t mptr;
    uint64_t prp1;
    uint64_t prp2;
    uint32_t cdw10, cdw11, cdw12, cdw13, cdw14, cdw15;
} __attribute__((packed)) NVME_CMD;

typedef struct {
    uint32_t cdw0;
    uint32_t reserved;
    uint16_t sq_head;
    uint16_t sq_id;
    uint16_t cid;
    uint16_t status;
} __attribute__((packed)) NVME_CQE;

static NVME_DEVICE       gNvme;
static volatile uint8_t *gRegs         = 0;
static uint16_t          gCid          = 1;
static uint32_t          gLastCqeCdw0  = 0;

static inline uint32_t mmio_rd32(uint32_t off)
{
    return *(volatile uint32_t *)(gRegs + off);
}
static inline void mmio_wr32(uint32_t off, uint32_t v)
{
    *(volatile uint32_t *)(gRegs + off) = v;
}
static inline uint64_t mmio_rd64(uint32_t off)
{
    uint32_t lo = mmio_rd32(off);
    uint32_t hi = mmio_rd32(off + 4);
    return ((uint64_t)hi << 32) | lo;
}
static inline void mmio_wr64(uint32_t off, uint64_t v)
{
    mmio_wr32(off, (uint32_t)(v & 0xFFFFFFFF));
    mmio_wr32(off + 4, (uint32_t)(v >> 32));
}

static inline void ring_doorbell(uint16_t qid, uint16_t idx,
                                 uint32_t stride, int is_sq)
{
    __asm__ volatile ("mfence" ::: "memory");
    uint32_t off = 0x1000 + ((2 * qid) + (is_sq ? 0 : 1)) * stride;
    mmio_wr32(off, idx);
    __asm__ volatile ("mfence" ::: "memory");
}

static int nvme_enable_msix(PCI_DEVICE *pci)
{
    uint8_t cap = HalpPciReadConfig(pci->Bus, pci->Device, pci->Function,
                                    0x34, 1) & 0xFC;
    KdPrintf("[NVMe] PCI cap list start = 0x%x\n", cap);

    for (int guard = 0; cap && guard < 48; guard++) {
        uint8_t id = HalpPciReadConfig(pci->Bus, pci->Device, pci->Function,
                                       cap, 1);
        KdPrintf("[NVMe]   cap @0x%02x id=0x%02x\n", cap, id);

        if (id == 0x11) {  
            uint32_t ctl32 = HalpPciReadConfig(pci->Bus, pci->Device,
                                               pci->Function, cap, 4);
            KdPrintf("[NVMe] MSI-X found, ctl32_old=0x%08x\n", ctl32);


            ctl32 |= (1u << 31);         
            ctl32 &= ~(3u << 29);        

            HalpPciWriteConfig(pci->Bus, pci->Device, pci->Function,
                               cap, 4, ctl32);

            uint32_t verify = HalpPciReadConfig(pci->Bus, pci->Device,
                                                pci->Function, cap, 4);
            KdPrintf("[NVMe] MSI-X enabled, ctl32=0x%08x (verify=0x%08x)\n",
                     ctl32, verify);

            if (!(verify & (1u << 31))) {
                KdPrint("[NVMe] WARNING: MSI-X Enable bit did not stick\n");
                return -1;
            }
            return 0;
        }
        cap = HalpPciReadConfig(pci->Bus, pci->Device, pci->Function,
                                cap + 1, 1) & 0xFC;
    }

    KdPrint("[NVMe] MSI-X capability not found\n");
    return -1;
}

static int setup_admin_queue(void)
{
    gNvme.AdminSqVirt = MmAllocatePages(1);
    gNvme.AdminCqVirt = MmAllocatePages(1);
    if (!gNvme.AdminSqVirt || !gNvme.AdminCqVirt) return -1;

    gNvme.AdminSqPhys = (uint64_t)(uintptr_t)gNvme.AdminSqVirt;
    gNvme.AdminCqPhys = (uint64_t)(uintptr_t)gNvme.AdminCqVirt;

    for (uint64_t i = 0; i < PAGE_SIZE; i++) {
        ((uint8_t *)gNvme.AdminSqVirt)[i] = 0;
        ((uint8_t *)gNvme.AdminCqVirt)[i] = 0;
    }

    gNvme.AdminSqTail  = 0;
    gNvme.AdminCqHead  = 0;
    gNvme.AdminCqPhase = 1;

    uint32_t aqa = (ADMIN_Q_DEPTH - 1) | ((uint32_t)(ADMIN_Q_DEPTH - 1) << 16);
    mmio_wr32(NVME_AQA, aqa);
    mmio_wr64(NVME_ASQ, gNvme.AdminSqPhys);
    mmio_wr64(NVME_ACQ, gNvme.AdminCqPhys);
    return 0;
}

static int admin_cmd(NVME_CMD *cmd, void *data_buf, uint32_t data_size)
{
    if (data_buf) {
        cmd->prp1 = (uint64_t)(uintptr_t)data_buf;
        if (data_size > PAGE_SIZE)
            cmd->prp2 = cmd->prp1 + PAGE_SIZE;
    }
    cmd->cdw0 = (cmd->cdw0 & 0xFF) | (gCid++ << 16);

    NVME_CMD *sq = (NVME_CMD *)gNvme.AdminSqVirt;
    sq[gNvme.AdminSqTail] = *cmd;

    gNvme.AdminSqTail = (gNvme.AdminSqTail + 1) % ADMIN_Q_DEPTH;
    ring_doorbell(0, gNvme.AdminSqTail, gNvme.DoorbellStride, 1);

    NVME_CQE *cq = (NVME_CQE *)gNvme.AdminCqVirt;
    for (int spin = 0; spin < 10000000; spin++) {
        NVME_CQE *e = &cq[gNvme.AdminCqHead];
        if ((e->status & 1) == gNvme.AdminCqPhase) {
            uint16_t status = e->status >> 1;
            gLastCqeCdw0 = e->cdw0;

            if (status) {
                KdPrintf("[NVMe] cmd err: status=0x%x SC=%u SCT=%u DNR=%u "
                         "cdw10=0x%x cdw11=0x%x\n",
                         status, status & 0xFF, (status >> 8) & 7,
                         (status >> 14) & 1, cmd->cdw10, cmd->cdw11);
            }

            gNvme.AdminCqHead = (gNvme.AdminCqHead + 1) % ADMIN_Q_DEPTH;
            if (gNvme.AdminCqHead == 0) gNvme.AdminCqPhase ^= 1;
            ring_doorbell(0, gNvme.AdminCqHead, gNvme.DoorbellStride, 0);

            return status == 0 ? 0 : -(int)status;
        }
        __asm__ volatile ("pause");
    }
    return -2;
}

static int nvme_reset_enable(void)
{
    uint32_t cc = mmio_rd32(NVME_CC);
    cc &= ~CC_EN;
    mmio_wr32(NVME_CC, cc);

    for (int i = 0; i < 1000000; i++) {
        if (!(mmio_rd32(NVME_CSTS) & CSTS_RDY)) break;
        __asm__ volatile ("pause");
    }

    if (setup_admin_queue() != 0) return -1;

    cc = CC_EN | CC_CSS_NVM | CC_MPS_4K | CC_AMS_RR
       | CC_IOSQES_64 | CC_IOCQES_16;
    mmio_wr32(NVME_CC, cc);

    for (int i = 0; i < 10000000; i++) {
        if (mmio_rd32(NVME_CSTS) & CSTS_RDY) return 0;
        __asm__ volatile ("pause");
    }
    return -1;
}

static int nvme_identify(uint32_t nsid, uint32_t cns, void *buf)
{
    NVME_CMD cmd = {0};
    cmd.cdw0  = ADM_IDENTIFY;
    cmd.nsid  = nsid;
    cmd.cdw10 = cns;
    return admin_cmd(&cmd, buf, 4096);
}
static int nvme_create_io_cq(void)
{
    gNvme.IoCqVirt = MmAllocatePages(1);
    if (!gNvme.IoCqVirt) return -1;
    gNvme.IoCqPhys = (uint64_t)(uintptr_t)gNvme.IoCqVirt;
    for (uint64_t i = 0; i < PAGE_SIZE; i++)
        ((uint8_t *)gNvme.IoCqVirt)[i] = 0;
    gNvme.IoCqHead  = 0;
    gNvme.IoCqPhase = 1;

    uint16_t cqid       = 1;
    uint16_t qsize      = IO_Q_DEPTH - 1;      
    uint16_t cq_flags   = (1u << 0) | (1u << 1); 
    uint16_t irq_vector = 1;

    NVME_CMD cmd = {0};
    cmd.cdw0  = ADM_CREATE_CQ;
    cmd.prp1  = gNvme.IoCqPhys;

    cmd.cdw10 = (uint32_t)cqid | ((uint32_t)qsize << 16);

    cmd.cdw11 = (uint32_t)cq_flags | ((uint32_t)irq_vector << 16);

    KdPrintf("[NVMe] CQ create: cqid=%u qsize=%u flags=0x%x iv=%u "
             "cdw10=0x%x cdw11=0x%x phys=0x%llx\n",
             cqid, qsize, cq_flags, irq_vector,
             cmd.cdw10, cmd.cdw11,
             (unsigned long long)gNvme.IoCqPhys);
    return admin_cmd(&cmd, 0, 0);
}

static int nvme_create_io_sq(void)
{
    gNvme.IoSqVirt = MmAllocatePages(1);
    if (!gNvme.IoSqVirt) return -1;
    gNvme.IoSqPhys = (uint64_t)(uintptr_t)gNvme.IoSqVirt;
    for (uint64_t i = 0; i < PAGE_SIZE; i++)
        ((uint8_t *)gNvme.IoSqVirt)[i] = 0;
    gNvme.IoSqTail = 0;

    uint16_t sqid     = 1;
    uint16_t qsize    = IO_Q_DEPTH - 1;     
    uint16_t sq_flags = (1u << 0);         
    uint16_t cqid     = 1;

    NVME_CMD cmd = {0};
    cmd.cdw0  = ADM_CREATE_SQ;
    cmd.prp1  = gNvme.IoSqPhys;

    cmd.cdw10 = (uint32_t)sqid | ((uint32_t)qsize << 16);

    cmd.cdw11 = (uint32_t)sq_flags | ((uint32_t)cqid << 16);

    KdPrintf("[NVMe] SQ create: sqid=%u cqid=%u qsize=%u flags=0x%x "
             "cdw10=0x%x cdw11=0x%x phys=0x%llx\n",
             sqid, cqid, qsize, sq_flags,
             cmd.cdw10, cmd.cdw11,
             (unsigned long long)gNvme.IoSqPhys);
    return admin_cmd(&cmd, 0, 0);
}

static int io_rw(uint8_t opcode, uint64_t lba, uint32_t count, void *buf)
{
    NVME_CMD cmd = {0};
    cmd.cdw0  = opcode;
    cmd.nsid  = *(uint32_t *)gNvme.Nsid;

    uint64_t addr  = (uint64_t)(uintptr_t)buf;
    uint32_t bytes = count * gNvme.LbaSizeBytes;
    uint64_t off   = addr & 0xFFF;

    cmd.prp1 = addr;
    if (off + bytes > 4096) {
        cmd.prp2 = (addr + 4096) & ~0xFFFULL;
        if (off + bytes > 8192) return -1;
    }

  //  KdPrintf("[NVMe] io_rw op=%u lba=%llu cnt=%u prp1=0x%llx prp2=0x%llx\n",
    //         opcode, (unsigned long long)lba, count,
     //        (unsigned long long)cmd.prp1, (unsigned long long)cmd.prp2);

    cmd.cdw10 = (uint32_t)(lba & 0xFFFFFFFF);
    cmd.cdw11 = (uint32_t)(lba >> 32);
    cmd.cdw12 = (count - 1) & 0xFFFF;
    cmd.cdw0  = (cmd.cdw0 & 0xFF) | (gCid++ << 16);

    NVME_CMD *sq = (NVME_CMD *)gNvme.IoSqVirt;
    sq[gNvme.IoSqTail] = cmd;
    gNvme.IoSqTail = (gNvme.IoSqTail + 1) % IO_Q_DEPTH;
    ring_doorbell(1, gNvme.IoSqTail, gNvme.DoorbellStride, 1);

    NVME_CQE *cq = (NVME_CQE *)gNvme.IoCqVirt;
    for (int spin = 0; spin < 50000000; spin++) {
        NVME_CQE *e = &cq[gNvme.IoCqHead];
        if ((e->status & 1) == gNvme.IoCqPhase) {
            uint16_t status = e->status >> 1;
            if (status) {
                KdPrintf("[NVMe] io err: op=%u lba=%llu status=0x%x "
                         "SC=%u SCT=%u DNR=%u\n",
                         opcode, (unsigned long long)lba, status,
                         status & 0xFF, (status >> 8) & 7, (status >> 14) & 1);
            }
            gNvme.IoCqHead = (gNvme.IoCqHead + 1) % IO_Q_DEPTH;
            if (gNvme.IoCqHead == 0) gNvme.IoCqPhase ^= 1;
            ring_doorbell(1, gNvme.IoCqHead, gNvme.DoorbellStride, 0);
            return status == 0 ? 0 : -(int)status;
        }
        __asm__ volatile ("pause");
    }
    return -2;
}

int NvmeReadSectors(uint64_t Lba, uint32_t Count, void *Buf)
{
    return io_rw(IO_READ, Lba, Count, Buf);
}

int NvmeWriteSectors(uint64_t Lba, uint32_t Count, const void *Buf)
{
    return io_rw(IO_WRITE, Lba, Count, (void *)Buf);
}

void NvmeGetInfo(NVME_DEVICE *Out)
{
    if (Out) *Out = gNvme;
}

int NvmeInit(void)
{
    KdPrint("[NVMe] init\n");

    PCI_DEVICE pci;
    int found = 0;
    for (uint16_t bus = 0; bus < 256 && !found; bus++) {
        for (uint8_t dev = 0; dev < 32 && !found; dev++) {
            for (uint8_t fn = 0; fn < 8 && !found; fn++) {
                PCI_DEVICE d;
                if (PciReadDevice((uint8_t)bus, dev, fn, &d) != 0) {
                    if (fn == 0) break;
                    continue;
                }
                if (d.ClassCode == 0x01 && d.SubClass == 0x08) {
                    pci = d;
                    found = 1;
                }
                if (fn == 0 && !(d.HeaderType & 0x80)) break;
            }
        }
    }
    if (!found) {
        KdPrint("[NVMe] controller not found\n");
        return -1;
    }
    KdPrintf("[NVMe] found %04x:%04x at %02x:%02x.%x\n",
             pci.VendorId, pci.DeviceId, pci.Bus, pci.Device, pci.Function);

    uint32_t cmd_reg = HalpPciReadConfig(pci.Bus, pci.Device, pci.Function,
                                         PCI_COMMAND, 2);
    cmd_reg |= PCI_CMD_MEM | PCI_CMD_BUS_MASTER;
    HalpPciWriteConfig(pci.Bus, pci.Device, pci.Function, PCI_COMMAND, 2, cmd_reg);

    nvme_enable_msix(&pci);

    gNvme.Bar0Phys = pci.Bar[0];
    KdPrintf("[NVMe] BAR0 phys=0x%llx\n", (unsigned long long)gNvme.Bar0Phys);
    if (gNvme.Bar0Phys == 0) return -1;

    extern int      MiMapPage(uint64_t Pml4Phys, uint64_t Virt, uint64_t Phys, uint64_t Flags);
    extern uint64_t MmGetKernelPml4(void);

    uint64_t kpml4 = MmGetKernelPml4();
    for (uint64_t off = 0; off < NVME_MMIO_SIZE; off += 4096) {
        int rc = MiMapPage(kpml4,
                           NVME_MMIO_VIRT + off,
                           gNvme.Bar0Phys + off,
                           PTE_PRESENT | PTE_WRITABLE | PTE_PCD);
        if (rc != 0) {
            KdPrintf("[NVMe] MMIO map failed at off=0x%llx rc=%d\n",
                     (unsigned long long)off, rc);
            return -1;
        }
    }
    gRegs = (volatile uint8_t *)(uintptr_t)NVME_MMIO_VIRT;
    KdPrintf("[NVMe] gRegs (virt) = 0x%llx\n",
             (unsigned long long)(uintptr_t)gRegs);

    uint64_t cap = mmio_rd64(NVME_CAP);
    gNvme.DoorbellStride  = 4 << ((cap >> 32) & 0xF);
    gNvme.MaxQueueEntries = (cap & 0xFFFF) + 1;
    KdPrintf("[NVMe] CAP=0x%llx stride=%u maxq=%u\n",
             (unsigned long long)cap, gNvme.DoorbellStride, gNvme.MaxQueueEntries);

    if (nvme_reset_enable() != 0) {
        KdPrint("[NVMe] controller enable failed\n");
        return -1;
    }
    KdPrint("[NVMe] controller ready\n");

    void *idbuf = MmAllocatePages(1);
    if (!idbuf) return -1;
    if (nvme_identify(0, IDENTIFY_CTRL, idbuf) != 0) {
        KdPrint("[NVMe] identify ctrl failed\n");
        MmFreePages(idbuf, 1);
        return -1;
    }
    uint16_t vid   = *(uint16_t *)((uint8_t *)idbuf + 0);
    uint16_t ssvid = *(uint16_t *)((uint8_t *)idbuf + 2);
    KdPrintf("[NVMe] ctrl VID=%04x SSVID=%04x\n", vid, ssvid);
    MmFreePages(idbuf, 1);

    idbuf = MmAllocatePages(1);
    if (!idbuf) return -1;
    if (nvme_identify(1, IDENTIFY_NSID, idbuf) != 0) {
        KdPrint("[NVMe] identify ns1 failed\n");
        MmFreePages(idbuf, 1);
        return -1;
    }
    uint64_t nsze  = *(uint64_t *)((uint8_t *)idbuf + 0);
    uint8_t  flbas = *((uint8_t *)idbuf + 26);
    uint32_t lbaf  = flbas & 0xF;
    uint8_t  lbads = *((uint8_t *)idbuf + 128 + lbaf * 4 + 2);
    gNvme.NsSizeLba    = nsze;
    gNvme.LbaSizeBytes = 1u << (lbads & 0xFF);
    *(uint32_t *)gNvme.Nsid = 1;

    KdPrintf("[NVMe] NS1: %llu sectors x %u bytes = %llu MB\n",
             (unsigned long long)nsze,
             gNvme.LbaSizeBytes,
             (unsigned long long)((nsze * gNvme.LbaSizeBytes) / (1024 * 1024)));
    MmFreePages(idbuf, 1);

    {
        NVME_CMD setq = {0};
        setq.cdw0  = ADM_SET_FEAT;
        setq.cdw10 = FEAT_NUM_QUEUES;
        setq.cdw11 = 0;
        int rc = admin_cmd(&setq, 0, 0);
        uint32_t granted = gLastCqeCdw0;
        KdPrintf("[NVMe] SetFeatures rc=%d granted_SQ=%u granted_CQ=%u\n",
                 rc, granted & 0xFFFF, (granted >> 16) & 0xFFFF);
        if (rc != 0) return -1;
    }

    if (nvme_create_io_cq() != 0) {
        KdPrint("[NVMe] create io cq failed\n");
        return -1;
    }
    if (nvme_create_io_sq() != 0) {
        KdPrint("[NVMe] create io sq failed\n");
        return -1;
    }

    for (volatile int i = 0; i < 1000000; i++)
        __asm__ volatile ("pause");

    KdPrint("[NVMe] init ok\n");
    return 0;
}