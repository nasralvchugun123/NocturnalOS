#include "ps.h"
#include <rt.h>
#include "../mm/mm.h"
#include "../../drivers/fs/vfs/vfs.h"
#include "../dll/loader.h"
#include "../dll/imgsec.h"
#include <nt/ntdefs.h>
#include <nt/pe.h>
#include <dbg/print/kdprint.h>
#include <ke/kcpu.h>
#include <nt/atomic.h>

#define USER_PE_BUF_PAGES 16
#define USER_PE_BUF_BYTES (USER_PE_BUF_PAGES * 4096)
#define NTDLL_PATH        "\\NocturnalOS\\System\\Core\\Sys64\\ntdll.dll"

static volatile uint32_t gNextThreadId = 1;
static volatile uint32_t gNextPid      = 1;

extern void KiEnterUserMode(uint64_t EntryPoint, uint64_t UserStackTop, uint64_t Params);
extern uint32_t KiGetNextCpuForThread(void);
extern void KiInsertReadyThreadOnCpu(uint32_t CpuIndex, KTHREAD *t);

static struct {
    OBJECT_HEADER Hdr;
    EPROCESS      Body;
} gSystemContainer;

static int gSystemProcessInit = 0;

EPROCESS *PsGetSystemProcess(void)
{
    if (!gSystemProcessInit) {
        for (uint64_t i = 0; i < sizeof(gSystemContainer); i++)
            ((uint8_t *)&gSystemContainer)[i] = 0;

        gSystemContainer.Hdr.Type        = &ObpProcessType;
        gSystemContainer.Hdr.RefCount    = 1;
        gSystemContainer.Hdr.HandleCount = 0;
        gSystemContainer.Hdr.SignalState = 0;
        gSystemContainer.Hdr.PageCount   = 0;
        gSystemContainer.Hdr.WaiterHead  = 0;

        gSystemContainer.Body.Pid    = 0;
        gSystemContainer.Body.Name[0] = 'S';
        gSystemContainer.Body.Name[1] = 'y';
        gSystemContainer.Body.Name[2] = 's';
        gSystemContainer.Body.Name[3] = 't';
        gSystemContainer.Body.Name[4] = 'e';
        gSystemContainer.Body.Name[5] = 'm';
        gSystemContainer.Body.Name[6] = 0;

        gSystemProcessInit = 1;
        KdPrint("[ps] system process initialized\n");
    }
    return &gSystemContainer.Body;
}

HANDLE PsCreateHandle(EPROCESS *Owner, OBJECT_TYPE *Type, void *Object)
{
    if (!Owner || !Object || !Type) return PS_INVALID_HANDLE;

    OBJECT_HEADER *hdr = OBJ_HEADER(Object);
    if (hdr->Type != Type) {
        KdPrintf("[ps] PsCreateHandle: type mismatch (owner='%s')\n", Owner->Name);
        return PS_INVALID_HANDLE;
    }

    return ObpAllocateHandle(Owner, hdr, 0);
}

void *PsLookupHandle(EPROCESS *Owner, HANDLE H, OBJECT_TYPE *Expected)
{
    OBJECT_HEADER *hdr = ObpLookupHandle(Owner, H, Expected);
    if (!hdr) return 0;
    return OBJ_BODY(hdr);
}

void PsCloseHandle(EPROCESS *Owner, HANDLE H)
{
    ObpFreeHandle(Owner, H);
}

void PsProcessTerminated(EPROCESS *Proc)
{
    if (!Proc) return;

    if (Proc->Pml4Phys) {
        ImgSecReleaseAllForPml4(Proc->Pml4Phys);
        MmFreeAddressSpace(Proc->Pml4Phys);
        Proc->Pml4Phys = 0;
    }

    ObSignalObject(OBJ_HEADER(Proc));
}

static void KiThreadStartup(void)
{
    KTHREAD *t = KeGetCurrentThread();
    if (!t) for (;;) __asm__ volatile ("hlt");

    KdPrint("\n\n\n");
    KdPrintf("[ps] thread '%s' (id=%u) started\n", t->Name, t->Id);

    __asm__ volatile ("sti");

    if (t->EntryPoint)
        t->EntryPoint(t->Argument);

    PsTerminateCurrentThread();
    for (;;) {
        __asm__ volatile ("sti");
        __asm__ volatile ("hlt");
    }
}

static void KiInitThreadStack(KTHREAD *t)
{
    uint64_t top = t->KernelStackTop;
    uint64_t *sp = (uint64_t *)(top - 80);

    sp[0] = 0;
    sp[1] = 0;
    sp[2] = 0;
    sp[3] = 0;
    sp[4] = 0;
    sp[5] = 0;
    sp[6] = 0;
    sp[7] = 0;
    sp[8] = (uint64_t)(uintptr_t)KiThreadStartup;

    t->Rsp = (uint64_t)(uintptr_t)sp;
}

KTHREAD *PsCreateThread(void (*EntryPoint)(void *), void *Argument, const char *Name, EPROCESS *Process)
{
    KTHREAD *t = (KTHREAD *)MmAllocatePages(1);
    if (!t) {
        KdPrint("[ps] PsCreateThread: KTHREAD alloc failed\n");
        return 0;
    }

    for (uint64_t i = 0; i < sizeof(KTHREAD); i++) ((uint8_t *)t)[i] = 0;

    t->Id         = AtomicFetchAdd32(&gNextThreadId, 1);
    t->Priority   = 8;
    t->EntryPoint = EntryPoint;
    t->Argument   = Argument;

    KTHREAD *cur = KeGetCurrentThread();
    t->LastCpuIndex = KeCurrentCpu()->CpuId;
    t->Process = Process ? Process : (cur ? cur->Process : PsGetSystemProcess());

    if (Name) {
        int i;
        for (i = 0; Name[i] && i < 31; i++) t->Name[i] = Name[i];
        t->Name[i] = 0;
    }

        if (EntryPoint) {
        void *stack = MmAllocatePages(4);
        if (!stack) {
            KdPrint("[ps] PsCreateThread: stack alloc failed\n");
            MmFreePages(t, 1);
            return 0;
        }
        t->KernelStackBase = (uint64_t)(uintptr_t)stack;
        t->KernelStackTop  = ((uint64_t)(uintptr_t)stack + 4 * 4096) & ~0xFULL;
        t->State           = ThreadStateReady;
        KiInitThreadStack(t);
        
        int isIdle = 0;
        if (Name && Name[0]=='i' && Name[1]=='d' && Name[2]=='l' && Name[3]=='e' && Name[4]==0) {
            isIdle = 1;
        }

        if (!isIdle) {
            t->LastCpuIndex = KiGetNextCpuForThread(); 
            KiInsertReadyThreadOnCpu(t->LastCpuIndex, t);
        } else {
            t->LastCpuIndex = KeCurrentCpu()->CpuId;
        }
    } else {
        t->State = ThreadStateReady;
    }

    KdPrintf("[ps] created thread '%s' id=%u base=0x%llx top=0x%llx rsp=0x%llx\n",
             t->Name, t->Id,
             (unsigned long long)t->KernelStackBase,
             (unsigned long long)t->KernelStackTop,
             (unsigned long long)t->Rsp);
    return t;
}

void PsTerminateCurrentThread(void)
{
    KTHREAD *t = KeGetCurrentThread();
    if (!t) for (;;) __asm__ volatile ("hlt");

    t->State = ThreadStateTerminated;
    KdPrintf("[ps] thread '%s' id=%u terminated\n", t->Name, t->Id);

    ObpThreadTerminated(t);

    if (t->Process && t->Process->MainThread == t) {
        PsProcessTerminated(t->Process);
    }

    KiScheduleNext();

    for (;;) __asm__ volatile ("hlt");
}

static uint64_t PsBuildProcessParameters(uint64_t Pml4,
                                         const char *ImagePath,
                                         const char *CmdLine)
{
    void *phys = PmmAllocatePage();
    if (!phys) {
        KdPrint("[ps] PsBuildProcessParameters: alloc failed\n");
        return 0;
    }

    uint8_t *p = (uint8_t *)phys;
    for (int i = 0; i < 4096; i++) p[i] = 0;

    RTL_USER_PROCESS_PARAMETERS *up = (RTL_USER_PROCESS_PARAMETERS *)p;
    uint8_t *str = p + sizeof(*up);

    uint32_t iplen = 0;
    while (ImagePath && ImagePath[iplen] && iplen < 200) iplen++;
    for (uint32_t i = 0; i < iplen; i++) str[i] = (uint8_t)ImagePath[i];
    str[iplen] = 0;
    up->ImagePathName.Length        = (USHORT)(iplen * 2);
    up->ImagePathName.MaximumLength = (USHORT)((iplen + 1) * 2);
    up->ImagePathName.Buffer        = (PWSTR)(USER_PARAMS_ADDR + (uint64_t)(str - p));
    str += iplen + 1;

    const char *cl = (CmdLine && CmdLine[0]) ? CmdLine : ImagePath;
    uint32_t cllen = 0;
    while (cl && cl[cllen] && cllen < 1500) cllen++;
    for (uint32_t i = 0; i < cllen; i++) str[i] = (uint8_t)cl[i];
    str[cllen] = 0;
    up->CommandLine.Length        = (USHORT)(cllen * 2);
    up->CommandLine.MaximumLength = (USHORT)((cllen + 1) * 2);
    up->CommandLine.Buffer        = (PWSTR)(USER_PARAMS_ADDR + (uint64_t)(str - p));
    str += cllen + 1;

    up->Length                  = (uint32_t)(str - p);
    up->Flags                   = 0;
    up->Environment             = 0;
    up->CurrentDirectoryHandle  = 0;

    if (MiMapPage(Pml4, USER_PARAMS_ADDR,
                  (uint64_t)(uintptr_t)phys,
                  PTE_PRESENT | PTE_WRITABLE | PTE_USER) != 0) {
        KdPrint("[ps] PsBuildProcessParameters: MiMapPage failed\n");
        PmmFreePage(phys);
        return 0;
    }

    KdPrintf("[ps] params built: va=0x%llx len=%u cl='%s'\n",
             (unsigned long long)USER_PARAMS_ADDR,
             up->Length, cl);
    return USER_PARAMS_ADDR;
}

static uint64_t PsBuildPeb(uint64_t Pml4, uint64_t ImageBase, uint64_t Params)
{
    void *phys = PmmAllocatePage();
    if (!phys) return 0;
    uint8_t *p = (uint8_t *)phys;
    for (int i = 0; i < 4096; i++) p[i] = 0;

    PEB *peb = (PEB *)p;
    peb->ImageBaseAddress  = (PVOID)ImageBase;
    peb->ProcessParameters = (PVOID)Params;

    PEB_LDR_DATA *ldr = (PEB_LDR_DATA *)(p + sizeof(PEB));
    ldr->Length = sizeof(PEB_LDR_DATA);
    ldr->Initialized = 1;
    ldr->SsHandle = 0;
    
    uint64_t ldr_va = USER_PEB_ADDR + sizeof(PEB);
    uint64_t l1_va  = ldr_va + (uint64_t)((uint8_t *)&ldr->InLoadOrderModuleList -
                                          (uint8_t *)ldr);
    uint64_t l2_va  = ldr_va + (uint64_t)((uint8_t *)&ldr->InMemoryOrderModuleList -
                                          (uint8_t *)ldr);
    uint64_t l3_va  = ldr_va + (uint64_t)((uint8_t *)&ldr->InInitializationOrderModuleList -
                                          (uint8_t *)ldr);

    ldr->InLoadOrderModuleList.Flink             = (LIST_ENTRY *)(uintptr_t)l1_va;
    ldr->InLoadOrderModuleList.Blink             = (LIST_ENTRY *)(uintptr_t)l1_va;
    ldr->InMemoryOrderModuleList.Flink           = (LIST_ENTRY *)(uintptr_t)l2_va;
    ldr->InMemoryOrderModuleList.Blink           = (LIST_ENTRY *)(uintptr_t)l2_va;
    ldr->InInitializationOrderModuleList.Flink   = (LIST_ENTRY *)(uintptr_t)l3_va;
    ldr->InInitializationOrderModuleList.Blink   = (LIST_ENTRY *)(uintptr_t)l3_va;

    peb->Ldr = (PPEB_LDR_DATA)(uintptr_t)ldr_va;

    if (MiMapPage(Pml4, USER_PEB_ADDR, (uint64_t)(uintptr_t)phys,
                  PTE_PRESENT | PTE_WRITABLE | PTE_USER) != 0) {
        PmmFreePage(phys);
        return 0;
    }

    KdPrintf("[ps] PEB at 0x%llx (Ldr=0x%llx, lists=%llx/%llx/%llx)\n",
             (unsigned long long)USER_PEB_ADDR,
             (unsigned long long)ldr_va,
             (unsigned long long)l1_va,
             (unsigned long long)l2_va,
             (unsigned long long)l3_va);
    return USER_PEB_ADDR;
}

static uint64_t PsBuildTeb(uint64_t Pml4, uint64_t PebVa, uint64_t StackTop)
{
    void *phys = PmmAllocatePage();
    if (!phys) return 0;
    uint8_t *p = (uint8_t *)phys;
    for (int i = 0; i < 4096; i++) p[i] = 0;

    TEB *teb = (TEB *)p;

    /*NT_TIB Windows x86_64 layout (не,     уже точно)*/
    teb->NtTib.ExceptionList        = (PVOID)0xFFFFFFFFFFFFFFFFULL;
    teb->NtTib.StackBase            = (PVOID)StackTop;
    teb->NtTib.StackLimit           = (PVOID)(StackTop - 0x1000);
    teb->NtTib.SubSystemTib         = 0;
    teb->NtTib.FiberData            = 0;
    teb->NtTib.ArbitraryUserPointer = 0;
    teb->NtTib.Self                 = (PVOID)USER_TEB_ADDR;

    teb->ProcessEnvironmentBlock    = (PVOID)PebVa;

    if (MiMapPage(Pml4, USER_TEB_ADDR, (uint64_t)(uintptr_t)phys,
                  PTE_PRESENT | PTE_WRITABLE | PTE_USER) != 0) {
        PmmFreePage(phys);
        return 0;
    }

        KdPrintf("[ps] TEB dump @0x8000900000:\n");
    uint64_t *q = (uint64_t *)p;
    for (int i = 0; i < 16; i++)
        KdPrintf("       +0x%02x: 0x%016llx\n",
        i * 8, (unsigned long long)q[i]);

    KdPrintf("[ps] TEB at 0x%llx (Self=0x%llx, StackBase=0x%llx)\n",
             (unsigned long long)USER_TEB_ADDR,
             (unsigned long long)USER_TEB_ADDR,
             (unsigned long long)StackTop);
    return USER_TEB_ADDR;
}

static void KiUserThreadStartup(void *arg)
{
    
    EPROCESS *proc = (EPROCESS *)arg;

    KdPrintf("[ps] user thread '%s' entering entry=0x%llx peb=0x%llx teb=0x%llx\n",
             proc->Name,
             (unsigned long long)proc->UserEntryPoint,
             (unsigned long long)proc->Peb,
             (unsigned long long)proc->Teb);

    __asm__ volatile ("cli");
    
    uint64_t teb = proc->Teb;
    uint32_t lo  = (uint32_t)(teb & 0xFFFFFFFF);
    uint32_t hi  = (uint32_t)(teb >> 32);
    __asm__ volatile ("wrmsr"
                      :: "c"(0xC0000102u), "a"(lo), "d"(hi)
                      : "memory");

    KCPU *c = KeCurrentCpu();
    KdPrintf("[dbg] KCPU=0x%llx Tss.rsp0=0x%llx Kcpu.Rsp0=0x%llx\n",
             (unsigned long long)c,
             (unsigned long long)c->Tss.rsp0,
             (unsigned long long)c->Rsp0);
    

    KdPrintf("[ps] user RSP=0x%llx entry=0x%llx\n",
             (unsigned long long)proc->UserStackTop,
             (unsigned long long)proc->UserEntryPoint);

    KiEnterUserMode(proc->UserEntryPoint,
                    proc->UserStackTop,
                    proc->Peb);

    for (;;) __asm__ volatile ("hlt");
}

EPROCESS *PsCreateUserProcess(const char *Path, const char *CmdLine, HANDLE *HandleOut)
{
    if (HandleOut) *HandleOut = PS_INVALID_HANDLE;
    if (!Path) {
        KdPrint("[ps] PsCreateUserProcess: Path is NULL\n");
        return 0;
    }

    EPROCESS *proc = (EPROCESS *)ObCreateObject(&ObpProcessType, sizeof(EPROCESS));
    if (!proc) {
        KdPrint("[ps] PsCreateUserProcess: ObCreateObject failed\n");
        return 0;
    }

    proc->ExitCode          = 0;
    proc->Pid               = AtomicFetchAdd32(&gNextPid, 1);
    proc->Pml4Phys          = 0;
    proc->MainThread        = 0;
    proc->UserEntryPoint    = 0;
    proc->UserStackTop      = 0;
    proc->ProcessParameters = 0;
    proc->Peb               = 0;
    proc->Teb               = 0;

    {
        const char *base = Path;
        for (const char *p = Path; *p; p++) if (*p == '\\') base = p + 1;
        int i = 0;
        while (base[i] && i < 31) { proc->Name[i] = base[i]; i++; }
        proc->Name[i] = 0;
    }

    uint64_t pml4 = MmCreateAddressSpace();
    if (!pml4) {
        KdPrintf("[ps] '%s': MmCreateAddressSpace failed\n", proc->Name);
        ObDereferenceObject(proc);
        return 0;
    }
    proc->Pml4Phys = pml4;

    IMAGE_EXPORTS *ntdllExp = KipGetNtdllExports();
    if (!ntdllExp) {
        IMAGE_SECTION_ENTRY *sec = 0;
        NTSTATUS st = KipLoadImage(proc, NTDLL_PATH, 0, 1,
                                   &sec, &ntdllExp, 0);
        if (!NT_SUCCESS(st) || !ntdllExp) {
            KdPrintf("[ps] '%s': ntdll bootstrap failed\n", proc->Name);
            MmFreeAddressSpace(pml4);
            proc->Pml4Phys = 0;
            ObDereferenceObject(proc);
            return 0;
        }
    } else {
        IMAGE_SECTION_ENTRY *sec = ImgSecFindByBase(ntdllExp->Base);
        if (sec) ImgSecMap(sec, pml4);
    }

    IMAGE_SECTION_ENTRY *exeSec = 0;
    IMAGE_EXPORTS       *exeExp = 0;
    uint64_t             entry  = 0;
    NTSTATUS st = KipLoadImage(proc, Path, ntdllExp, 0,
                               &exeSec, &exeExp, &entry);
    if (!NT_SUCCESS(st)) {
        KdPrintf("[ps] '%s': KipLoadImage failed\n", Path);
        MmFreeAddressSpace(pml4);
        proc->Pml4Phys = 0;
        ObDereferenceObject(proc);
        return 0;
    }
    proc->UserEntryPoint = entry;

    {
        void *stack_top_phys = PmmAllocatePage();
        if (!stack_top_phys) {
            KdPrintf("[ps] '%s': user stack alloc failed\n", proc->Name);
            MmFreeAddressSpace(pml4);
            proc->Pml4Phys = 0;
            ObDereferenceObject(proc);
            return 0;
        }
        for (int i = 0; i < 4096; i++) ((uint8_t *)stack_top_phys)[i] = 0;

        if (MiMapPage(pml4, USER_STACK_TOP - 4096,
                      (uint64_t)(uintptr_t)stack_top_phys,
                      PTE_PRESENT | PTE_WRITABLE | PTE_USER) != 0) {
            KdPrintf("[ps] '%s': MiMapPage stack top failed\n", proc->Name);
            PmmFreePage(stack_top_phys);
            MmFreeAddressSpace(pml4);
            proc->Pml4Phys = 0;
            ObDereferenceObject(proc);
            return 0;
        }
    }
    proc->UserStackTop = USER_STACK_TOP - 0x108;

    proc->ProcessParameters = PsBuildProcessParameters(pml4, Path, CmdLine);
    if (!proc->ProcessParameters) {
        KdPrintf("[ps] '%s': params build failed\n", proc->Name);
        MmFreeAddressSpace(pml4);
        proc->Pml4Phys = 0;
        ObDereferenceObject(proc);
        return 0;
    }

    proc->Peb = PsBuildPeb(pml4, exeSec->Base, proc->ProcessParameters);
    if (!proc->Peb) {
        KdPrintf("[ps] '%s': PEB build failed\n", proc->Name);
        MmFreeAddressSpace(pml4);
        proc->Pml4Phys = 0;
        ObDereferenceObject(proc);
        return 0;
    }

    proc->Teb = PsBuildTeb(pml4, proc->Peb, proc->UserStackTop);
    if (!proc->Teb) {
        KdPrintf("[ps] '%s': TEB build failed\n", proc->Name);
        MmFreeAddressSpace(pml4);
        proc->Pml4Phys = 0;
        ObDereferenceObject(proc);
        return 0;
    }

    __asm__ volatile ("cli");

    KTHREAD *t = PsCreateThread(KiUserThreadStartup, proc, proc->Name, proc);
    if (!t) {
        __asm__ volatile ("sti");
        KdPrintf("[ps] '%s': PsCreateThread failed\n", proc->Name);
        MmFreeAddressSpace(pml4);
        proc->Pml4Phys = 0;
        ObDereferenceObject(proc);
        return 0;
    }

    proc->MainThread = t;
    
    HANDLE selfH = PsCreateHandle(proc, &ObpProcessType, proc);
    (void)selfH;

    KTHREAD  *cur    = KeGetCurrentThread();
    EPROCESS *parent = cur ? cur->Process : 0;
    HANDLE    hChild = PS_INVALID_HANDLE;
    if (parent) hChild = PsCreateHandle(parent, &ObpProcessType, proc);

    __asm__ volatile ("sti");

    if (HandleOut) *HandleOut = hChild;

    KdPrintf("[ps] user process '%s' pid=%u thread=%u as=0x%llx entry=0x%llx peb=0x%llx\n",
             proc->Name, proc->Pid, t->Id,
             (unsigned long long)pml4,
             (unsigned long long)proc->UserEntryPoint,
             (unsigned long long)proc->Peb);

    return proc;
}