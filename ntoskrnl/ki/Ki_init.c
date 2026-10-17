#include "../ntoskrnl.h"
#include "boot_info.h"
#include <rt.h>
#include "../ke/ke.h"
#include "../ke/kbd.h"
#include "../ke/shell.h"
#include "../mm/mm.h"
#include "../ps/ps.h"
#include "../../drivers/fs/GPT/gpt.h"
#include "../../drivers/fs/fat/fat32/fat32.h"
#include "../../drivers/fs/vfs/vfs.h"
#include <dbg/print/kdprint.h>
#include <ds/fb/fb.h>
#include <nt/ntdll.h>
#include <nt/atomic.h>

static BOOT_INFO *gBootInfo    = 0;
static volatile int64_t gCurrentTick = 0;

void KeIncrementTick(void)
{
    (void)AtomicIncrement64(&gCurrentTick);
}

uint64_t KeGetTickCount(void)
{
    return (uint64_t)AtomicLoadAcquire64(&gCurrentTick);
}

extern void      KiInitSyscalls(void);
extern void      KiEnterUserMode(uint64_t EntryPoint, uint64_t UserStackTop, uint64_t Params);
extern void      KiSetTssRsp0(uint64_t Rsp0);
extern uint64_t  MmCreateAddressSpace(void);
extern void      MmSwitchAddressSpace(uint64_t Pml4Phys);
extern void     *MmAllocatePages(uint32_t PageCount);

extern void      ImgSecInit(void);
extern void      KiRegisterImageSectionSyscalls(void);

extern int       NvmeInit(void);
extern int       NvmeReadSectors(uint64_t Lba, uint32_t Count, void *Buf);

static void KiDumpLba0(const uint8_t *buf)
{
    KdPrint("[Ki] LBA0[0..15]: ");
    for (int i = 0; i < 16; i++) KdPrintf("%02x ", buf[i]);
    KdPrint("\n");
    KdPrintf("[Ki] LBA0[510..511]: %02x %02x (expect 55 AA)\n",
             buf[510], buf[511]);
}

static void KiBringUpStorage(void)
{
    KdPrint("[Ki] storage bring-up\n");

    KdPrint("[Ki] testing NVMe\n");
    if (NvmeInit() != 0) {
        KdPrint("[Ki] NVMe not found, storage skipped\n");
        return;
    }

    {
        uint8_t *buf = (uint8_t *)MmAllocatePages(1);
        if (!buf) {
            KdPrint("[Ki] no memory for LBA0 buf\n");
            return;
        }
        if (NvmeReadSectors(0, 1, buf) == 0)
            KiDumpLba0(buf);
        else
            KdPrint("[Ki] NvmeReadSectors(0) failed\n");
    }

    KdPrint("[Ki] parsing GPT\n");
    static GPT_INFO gpt;
    if (GptParse((void *)NvmeReadSectors, &gpt) != 0) {
        KdPrint("[Ki] GptParse failed\n");
        return;
    }

    GPT_ENTRY *part = 0;
    if (GptFindPartition(&gpt, GUID_TYPE_MS_BASIC_DATA, &part) != 0 || !part) {
        KdPrint("[Ki] no MS Basic Data partition\n");
        return;
    }
    KdPrintf("[Ki] DATA partition: LBA %llu..%llu\n",
             (unsigned long long)part->FirstLba,
             (unsigned long long)part->LastLba);

    static FAT32_FS fat;
    if (Fat32Mount((fat_read_fn)NvmeReadSectors, part->FirstLba, &fat) == 0) {
        KdPrint("[FAT32] root listing:\n");
        Fat32ListRoot(&fat);
    } else {
        KdPrint("[FAT32] mount failed\n");
    }

    KdPrint("[VFS] mounting on DATA partition\n");
    if (VfsMountFAT32((fat_read_fn)NvmeReadSectors, part->FirstLba) != 0) {
        KdPrint("[VFS] mount failed\n");
        return;
    }
    KdPrint("[VFS] mounted OK\n");

    KdPrint("[Ki] storage ready\n");
}

void KiSystemStartup(BOOT_INFO *BootInfo)
{
    __asm__ volatile ("cli" ::: "memory");
    KdInitSerial();
    KdPrint("\n\n\n\n\n");
    KCpuInitBsp(); 
    
    if (!BootInfo || BootInfo->Magic != BOOT_INFO_MAGIC) {
        KdPrint("[Ki] FATAL: bad BOOT_INFO\n");
        for (;;) __asm__ volatile ("hlt");
    }
    gBootInfo = BootInfo;

    KdPrint("[Ki] installing early GDT/TSS/IDT\n");
    KiInitializeGdt();
    KiInitializeTss();
    KiInitializeIdt();

    KdPrint("                                                                <system>                                                          \n");
    KdPrint("################################################################################################################################# \n");
    KdPrintf("[Ki] BOOT_INFO magic=0x%x version=%u size=%u\n",
             (unsigned)BootInfo->Magic,
             (unsigned)BootInfo->Version,
             (unsigned)BootInfo->Size);
    KdPrintf("[Ki] KernelBase=0x%llx Framebuffer=0x%llx %ux%u stride=%u fmt=%u\n",
             (unsigned long long)BootInfo->KernelBase,
             (unsigned long long)BootInfo->FramebufferBase,
             BootInfo->FramebufferWidth,
             BootInfo->FramebufferHeight,
             BootInfo->FramebufferPixelsPerScanLine,
             BootInfo->FramebufferFormat);
    KdPrintf("[Ki] MemoryMap size=%llu stride=%llu map=0x%llx\n",
             (unsigned long long)BootInfo->MemoryMapSize,
             (unsigned long long)BootInfo->MemoryMapDescriptorSize,
             (unsigned long long)BootInfo->MemoryMap);

    if (BootInfo->FramebufferBase) {
        if (FbInit(BootInfo->FramebufferBase,
                   BootInfo->FramebufferWidth,
                   BootInfo->FramebufferHeight,
                   BootInfo->FramebufferPixelsPerScanLine,
                   BootInfo->FramebufferFormat) == 0) {
            KdPrint("[Ki] framebuffer console up\n");
        } else {
            KdPrint("[Ki] fb init FAILED\n");
        }
    } else {
        KdPrint("[Ki] NO FRAMEBUFFER\n");
    }

    extern void HalpInitOProcessors(void);
    extern void HalpMadtInit(uint64_t RsdpPhys);
    
    KdPrint("[Ki] phases\n");
    HalInitSystem();                  KdPrint("Halsystems   = ok\n");
    HalpMadtInit(BootInfo->AcpiRsdp); KdPrint("madt parse   = ok\n");
    MmInitSystem(BootInfo);           KdPrint("sys meminit  = ok\n");
    KeInitSystem();                   KdPrint("sys keinit   = ok\n");
    ObInitSystem();                   KdPrint("sys objinit  = ok\n");
    PsInitSystem();                   KdPrint("sys psinit   = ok\n");
    HalpInitOProcessors();        KdPrint("smp bringup  = ok\n");
    IoInitSystem();                   KdPrint("sys ioinit   = ok\n");
    KiInitSyscalls();                 KdPrint("syscall init = ok\n");
    KbdInit();                        KdPrint("Kinput  init = ok\n");
    KbdInitConsole();                 KdPrint("init console = ok\n");
    ImgSecInit();                     KdPrint("imgsec  init = ok\n");
    KiRegisterImageSectionSyscalls(); KdPrint("imgsyscalls  = ok\n");

    KdPrint("[Ki] phases done\n");


    KdPrint("[Ki] enabling interrupts (sti)\n");
    __asm__ volatile ("sti");

    KeCurrentCpu()->NoPreempt++;

    KiBringUpStorage();

    KdPrint("[Ki] launching user processes\n");
    PsCreateUserProcess("\\NocturnalOS\\system\\shell\\subsys\\services.exe", 0, 0);
    KdPrint("[Ki] services.exe spawned\n");

    KeCurrentCpu()->NoPreempt--;

    KTHREAD *boot = KeGetCurrentThread();
    if (boot) boot->State = ThreadStateBlocked;

    KiScheduleNext();

    for (;;) {
        __asm__ volatile ("sti");
        __asm__ volatile ("hlt");
    }
}