#include <dbg/print/kdprint.h>
#include <ds/fb/fb.h>
#include "../arch/except.h"
#include "../../ob/shared.h"
#include <rt.h>
#include "../../ps/ps.h"
#include "../../../drivers/fs/vfs/vfs.h"
#include "../../../sdk/inc/nt/syscall.h"
#include "../../mm/mm.h"

typedef uint64_t (*NtSyscallFn)(uint64_t a1, uint64_t a2, uint64_t a3,
                                uint64_t a4, uint64_t a5, uint64_t a6);

static NtSyscallFn gSyscallTable[256];
extern void     ImgSecInit(void);
extern void     KiRegisterImageSectionSyscalls(void);
extern int      MiMapPage(uint64_t Pml4Phys, uint64_t Virt, uint64_t Phys, uint64_t Flags);
extern int      MiIsMapped(uint64_t Pml4, uint64_t Virt);
extern KTHREAD *KeGetCurrentThread(void);
extern void     PsTerminateCurrentThread(void);
extern uint64_t KeGetTickCount(void);
extern void     KiSystemCallEntry(void);
extern EPROCESS *PsCreateUserProcess(const char *Path, const char *CmdLine, HANDLE *HandleOut);
extern int      KbdReadLine(char *Buf, int MaxLen);
static uint64_t NtAllocateVirtualMemory(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);
static uint64_t NtFreeVirtualMemory(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);

static uint64_t NtDelayExecution(uint64_t ms, uint64_t a2, uint64_t a3,
                                 uint64_t a4, uint64_t a5, uint64_t a6)
{
    (void)a2; (void)a3; (void)a4; (void)a5; (void)a6;

    uint64_t ticks = ms / 10;
    if (ticks == 0) ticks = 1;

    uint64_t target = KeGetTickCount() + ticks;

    KiBlockCurrentThread(target);
    KiScheduleNext();
    return 0;
}

static uint64_t NtReadFile(uint64_t path_ptr, uint64_t buf_ptr, uint64_t len,
                           uint64_t a4, uint64_t a5, uint64_t a6)
{
    (void)a4; (void)a5; (void)a6;

    if (!path_ptr || !buf_ptr || len == 0) return (uint64_t)-1;

    char kpath[256];
    const char *up = (const char *)path_ptr;
    int i = 0;
    for (; i < 255; i++) {
        char c = up[i];
        kpath[i] = c;
        if (c == 0) break;
    }
    kpath[i] = 0;

    if (i == 255 && up[255] != 0) {
        KdPrint("[syscall] NtReadFile: path too long\n");
        return (uint64_t)-1;
    }

    VFS_FILE f;
    if (VfsOpen(kpath, &f) != 0) {
        KdPrintf("[syscall] NtReadFile: open '%s' failed\n", kpath);
        return (uint64_t)-1;
    }

    int n = VfsRead(&f, (void *)buf_ptr, (uint32_t)len);
    VfsClose(&f);

    KdPrintf("[syscall] NtReadFile('%s', len=%llu) -> %d\n",
             kpath, (unsigned long long)len, n);

    if (n < 0) return (uint64_t)-1;
    return (uint64_t)n;
}

static uint64_t NtTerminateProcess(uint64_t handle, uint64_t code,
                                   uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6)
{
    (void)a3; (void)a4; (void)a5; (void)a6;

    KTHREAD  *cur = KeGetCurrentThread();
    EPROCESS *self = cur ? cur->Process : 0;

    if (handle == 0 || (self && (void *)(uintptr_t)handle == (void *)self)) {
        KdPrint("[syscall] NtTerminateProcess (self)\n");
        PsTerminateCurrentThread();
        return 0;
    }

    if (!self) return (uint64_t)-1;

    EPROCESS *target = PsLookupHandle(self, (HANDLE)(uintptr_t)handle,
                                  &ObpProcessType);
    if (!target) {
        KdPrintf("[syscall] NtTerminateProcess: bad handle %llu\n",
                 (unsigned long long)handle);
        return (uint64_t)-1;
    }

    KdPrintf("[syscall] NtTerminateProcess(target='%s' pid=%u code=%llu)\n",
             target->Name, target->Pid, (unsigned long long)code);

    target->ExitCode = (int32_t)code;
    if (target && target->MainThread)
        ObpThreadTerminated(target->MainThread);
    PsProcessTerminated(target);
    return 0;
}

static uint64_t NtDisplayString(uint64_t a1, uint64_t a2, uint64_t a3,
                                uint64_t a4, uint64_t a5, uint64_t a6)
{
    (void)a2; (void)a3; (void)a4; (void)a5; (void)a6;
    const char *usr = (const char *)a1;
    if (!usr) return (uint64_t)-1;

    __asm__ volatile ("cli");

    char buf[256];
    int i = 0;
    for (; i < 255; i++) {
        char c = usr[i];
        buf[i] = c;
        if (c == 0) break;
    }
    buf[i] = 0;
    KdPrintf("%s", buf);

    __asm__ volatile ("sti");
    return 0;
}

static uint64_t NtGetTickCount(uint64_t a1, uint64_t a2, uint64_t a3,
                               uint64_t a4, uint64_t a5, uint64_t a6)
{
    (void)a1; (void)a2; (void)a3; (void)a4; (void)a5; (void)a6;
    return KeGetTickCount();
}

static uint64_t NtCreateProcess(uint64_t path_ptr, uint64_t cmdline_ptr,
                                uint64_t handle_out_ptr,
                                uint64_t a4, uint64_t a5, uint64_t a6)
{
    (void)a4; (void)a5; (void)a6;
    if (!path_ptr || !handle_out_ptr) return (uint64_t)-1;

    char kpath[256];
    const char *up = (const char *)path_ptr;
    int i = 0;
    for (; i < 255; i++) { kpath[i] = up[i]; if (!up[i]) break; }
    kpath[i] = 0;
    if (i == 255 && up[255]) return (uint64_t)-1;

    char kcmd[1024];
    int has_cmd = 0;
    if (cmdline_ptr) {
        const char *uc = (const char *)cmdline_ptr;
        int j = 0;
        for (; j < 1023; j++) { kcmd[j] = uc[j]; if (!uc[j]) break; }
        kcmd[j] = 0;
        has_cmd = (j > 0);
    }

    KdPrintf("[syscall] NtCreateProcess('%s', cmd='%s')\n",
             kpath, has_cmd ? kcmd : "(default)");

    HANDLE h = PS_INVALID_HANDLE;
    EPROCESS *proc = PsCreateUserProcess(kpath,
                                         has_cmd ? kcmd : 0,
                                         &h);
    if (!proc || h == PS_INVALID_HANDLE) {
        KdPrintf("[syscall] NtCreateProcess: failed for '%s'\n", kpath);
        return (uint64_t)-1;
    }

    *(uint64_t *)handle_out_ptr = (uint64_t)h;
    KdPrintf("[syscall] NtCreateProcess -> handle=%llu pid=%u\n",
             (unsigned long long)h, proc->Pid);
    return 0;
}


static uint64_t NtWaitForSingleObject(uint64_t handle, uint64_t timeout_ms,
                                      uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6)
{
    (void)a3; (void)a4; (void)a5; (void)a6;

    KTHREAD *cur = KeGetCurrentThread();
    if (!cur || !cur->Process) return (uint64_t)-1;

    OBJECT_HEADER *hdr = ObpLookupHandle(cur->Process,
                                     (HANDLE)(uintptr_t)handle, 0);
    if (!hdr) {
        KdPrintf("[syscall] NtWaitForSingleObject: bad handle %llu\n",
                 (unsigned long long)handle);
        return (uint64_t)-1;
    }

    uint64_t timeout_ticks;
    if (timeout_ms == 0xFFFFFFFFFFFFFFFFULL) {
        timeout_ticks = 0xFFFFFFFFFFFFFFFFULL;
    } else {
        timeout_ticks = timeout_ms / 10;
        if (timeout_ticks == 0) timeout_ticks = 1;
    }

    KdPrintf("[syscall] NtWaitForSingleObject('%s') h=%llu timeout_ticks=%llu\n",
             hdr->Type ? hdr->Type->Name : "?",
             (unsigned long long)handle,
             (unsigned long long)timeout_ticks);

    int rc = ObWaitForSingleObject(hdr, timeout_ticks);

    if (rc == 0)
        KdPrintf("[syscall] NtWaitForSingleObject -> signaled\n");
    else
        KdPrintf("[syscall] NtWaitForSingleObject -> timeout\n");

    return (uint64_t)(int64_t)rc;
}

static uint64_t NtClose(uint64_t handle, uint64_t a2, uint64_t a3,
                        uint64_t a4, uint64_t a5, uint64_t a6)
{
    (void)a2; (void)a3; (void)a4; (void)a5; (void)a6;

    KTHREAD *cur = KeGetCurrentThread();
    if (!cur || !cur->Process) return (uint64_t)-1;

    KdPrintf("[syscall] NtClose(%llu)\n", (unsigned long long)handle);
    PsCloseHandle(cur->Process, (HANDLE)(uintptr_t)handle);
    return 0;
}


static void KiWriteMsr(uint32_t msr, uint64_t value)
{
    __asm__ volatile ("wrmsr"
                      :: "c"(msr),
                         "a"((uint32_t)(value & 0xFFFFFFFF)),
                         "d"((uint32_t)(value >> 32)));
}

static uint64_t KiReadMsr(uint32_t msr)
{
    uint32_t lo, hi;
    __asm__ volatile ("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
    return ((uint64_t)hi << 32) | lo;
}

#define MSR_EFER    0xC0000080
#define MSR_STAR    0xC0000081
#define MSR_LSTAR   0xC0000082
#define MSR_FMASK   0xC0000084

void KiInitSyscallMsrsForCurrentCpu(void)
{
    uint64_t efer = KiReadMsr(MSR_EFER);
    KiWriteMsr(MSR_EFER, efer | 1);

    uint64_t star = ((uint64_t)0x10 << 48) | ((uint64_t)0x08 << 32);
    KiWriteMsr(MSR_STAR, star);

    uint64_t lstar;
    __asm__ volatile ("leaq KiSystemCallEntry(%%rip), %0" : "=r"(lstar));
    KiWriteMsr(MSR_LSTAR, lstar);
    KiWriteMsr(MSR_FMASK, 0x200);

    uint32_t eax = 1, ebx, ecx, edx;
    __asm__ volatile ("cpuid" : "+a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx));
    (void)ecx;
    (void)edx;
    uint32_t apic_id = ebx >> 24;
    uint64_t efer_rb = KiReadMsr(MSR_EFER);
    uint64_t star_rb = KiReadMsr(MSR_STAR);
    uint64_t lstar_rb = KiReadMsr(MSR_LSTAR);
    uint64_t fmask_rb = KiReadMsr(MSR_FMASK);
    KdPrintf("[syscall] CPU%u EFER=%llx STAR=%llx LSTAR=%llx FMASK=%llx SCE=%u\n",
             apic_id,
             (unsigned long long)efer_rb,
             (unsigned long long)star_rb,
             (unsigned long long)lstar_rb,
             (unsigned long long)fmask_rb,
             (unsigned)(efer_rb & 1));
}

static uint64_t NtCreateMutant(uint64_t handle_out_ptr,
                               uint64_t initial_owner,
                               uint64_t name_ptr,
                               uint64_t a4, uint64_t a5, uint64_t a6)
{
    (void)a4; (void)a5; (void)a6;
    if (!handle_out_ptr) return (uint64_t)-1;

    KTHREAD *cur = KeGetCurrentThread();
    if (!cur || !cur->Process) return (uint64_t)-1;

    char kname[OBJ_NAME_MAX] = {0};
    int named = 0;

    if (name_ptr) {
        char path[256];
        const char *uname = (const char *)name_ptr;
        int i = 0;
        for (; i < 255; i++) { path[i] = uname[i]; if (!uname[i]) break; }
        path[i] = 0;
        if (i == 255) return (uint64_t)-1;

        OBJECT_HEADER *dir = 0;
        if (ObpParsePath(path, &dir, kname, OBJ_NAME_MAX) != 0) return (uint64_t)-1;
        if (ObpLookupObjectByName(dir, kname)) return (uint64_t)-1;
        named = 1;
    }

    KMUTANT *m = (KMUTANT *)ObCreateObject(&ObpMutantType, sizeof(KMUTANT));
    if (!m) return (uint64_t)-1;

    if (initial_owner) { m->Owner = cur; m->Count = 1; }
    else               { m->Owner = 0;   m->Count = 0; }
    m->Abandoned = 0;

    OBJECT_HEADER *hdr = OBJ_HEADER(m);
    hdr->SignalState = (m->Owner == 0) ? 1 : 0;

    if (named) {
        if (ObpInsertObjectByName(ObpBaseNamedObjects, kname, hdr) != 0) {
            ObDereferenceObject(m);
            return (uint64_t)-1;
        }
    }

    HANDLE h = ObpAllocateHandle(cur->Process, hdr, 0);
    if (!h) {
        if (named) ObpDeleteObjectByName(ObpBaseNamedObjects, kname);
        ObDereferenceObject(m);
        return (uint64_t)-1;
    }
    if (!named) ObDereferenceObject(m);

    *(uint64_t *)handle_out_ptr = (uint64_t)h;
    KdPrintf("[syscall] NtCreateMutant owner=%llu name='%s' -> h=%llu\n",
             (unsigned long long)initial_owner,
             named ? kname : "(unnamed)",
             (unsigned long long)h);
    return 0;
}

static uint64_t NtOpenMutant(uint64_t handle_out_ptr, uint64_t name_ptr,
                             uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6)
{
    (void)a3; (void)a4; (void)a5; (void)a6;
    if (!handle_out_ptr || !name_ptr) return (uint64_t)-1;

    KTHREAD *cur = KeGetCurrentThread();
    if (!cur || !cur->Process) return (uint64_t)-1;

    char path[256];
    const char *uname = (const char *)name_ptr;
    int i = 0;
    for (; i < 255; i++) { path[i] = uname[i]; if (!uname[i]) break; }
    path[i] = 0;
    if (i == 255) return (uint64_t)-1;

    char kname[OBJ_NAME_MAX] = {0};
    OBJECT_HEADER *dir = 0;
    if (ObpParsePath(path, &dir, kname, OBJ_NAME_MAX) != 0) return (uint64_t)-1;

    OBJECT_HEADER *hdr = ObpLookupObjectByName(dir, kname);
    if (!hdr || hdr->Type != &ObpMutantType) return (uint64_t)-1;

    HANDLE h = ObpAllocateHandle(cur->Process, hdr, 0);
    if (!h) return (uint64_t)-1;
    *(uint64_t *)handle_out_ptr = (uint64_t)h;

    KdPrintf("[syscall] NtOpenMutant '%s' -> h=%llu\n",
             kname, (unsigned long long)h);
    return 0;
}

static uint64_t NtReleaseMutant(uint64_t handle, uint64_t prev_count_ptr,
                                uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6)
{
    (void)a3; (void)a4; (void)a5; (void)a6;

    KTHREAD *cur = KeGetCurrentThread();
    if (!cur || !cur->Process) return (uint64_t)-1;

    OBJECT_HEADER *hdr = ObpLookupHandle(cur->Process, (HANDLE)handle, &ObpMutantType);
    if (!hdr) {
        KdPrintf("[syscall] NtReleaseMutant: bad handle %llu\n",
                 (unsigned long long)handle);
        return (uint64_t)-1;
    }

    KMUTANT *m = (KMUTANT *)OBJ_BODY(hdr);

    if (m->Owner != cur) {
        KdPrintf("[syscall] NtReleaseMutant: not owned (owner='%s')\n",
                 m->Owner ? m->Owner->Name : "(null)");
        return (uint64_t)(int64_t)STATUS_MUTANT_NOT_OWNED;
    }

    if (prev_count_ptr)
        *(uint64_t *)prev_count_ptr = (uint64_t)m->Count;

    m->Count--;
    KdPrintf("[syscall] NtReleaseMutant h=%llu count now %d\n",
             (unsigned long long)handle, m->Count);

    if (m->Count == 0) {
        m->Owner = 0;
        hdr->SignalState = 1;
        ObSignalObject(hdr);
    }
    return 0;
}

static uint64_t NtCreateEvent(uint64_t handle_out_ptr,
                              uint64_t event_type,
                              uint64_t initial_state,
                              uint64_t name_ptr,
                              uint64_t a5, uint64_t a6)
{
    (void)a5; (void)a6;
    if (!handle_out_ptr) return (uint64_t)-1;

    KTHREAD *cur = KeGetCurrentThread();
    if (!cur || !cur->Process) return (uint64_t)-1;

    char kname[OBJ_NAME_MAX] = {0};
    int named = 0;

    if (name_ptr) {
        const char *uname = (const char *)name_ptr;
        char path[256];
        int i = 0;
        for (; i < 255; i++) { path[i] = uname[i]; if (!uname[i]) break; }
        path[i] = 0;
        if (i == 255) return (uint64_t)-1;

        OBJECT_HEADER *dir = 0;
        if (ObpParsePath(path, &dir, kname, OBJ_NAME_MAX) != 0) {
            KdPrintf("[syscall] NtCreateEvent: bad path '%s'\n", path);
            return (uint64_t)-1;
        }

        if (ObpLookupObjectByName(dir, kname)) {
            KdPrintf("[syscall] NtCreateEvent: '%s' already exists\n", kname);
            return (uint64_t)-1;
        }
        named = 1;
    }

    KEVENT *ev = (KEVENT *)ObCreateObject(&ObpEventType, sizeof(KEVENT));
    if (!ev) return (uint64_t)-1;

    ev->EventType = (uint32_t)event_type;

    OBJECT_HEADER *hdr = OBJ_HEADER(ev);
    hdr->SignalState = (initial_state != 0) ? 1 : 0;

    if (named) {
        if (ObpInsertObjectByName(ObpBaseNamedObjects, kname, hdr) != 0) {
            ObDereferenceObject(ev);
            return (uint64_t)-1;
        }
    }

    HANDLE h = ObpAllocateHandle(cur->Process, hdr, 0);
    if (!h) {
        if (named) ObpDeleteObjectByName(ObpBaseNamedObjects, kname);
        ObDereferenceObject(ev);
        return (uint64_t)-1;
    }
    if (!named) ObDereferenceObject(ev);

    *(uint64_t *)handle_out_ptr = (uint64_t)h;

    KdPrintf("[syscall] NtCreateEvent type=%llu initial=%llu name='%s' -> h=%llu\n",
             (unsigned long long)event_type,
             (unsigned long long)initial_state,
             named ? kname : "(unnamed)",
             (unsigned long long)h);
    return 0;
}

static uint64_t NtOpenEvent(uint64_t handle_out_ptr, uint64_t name_ptr,
                            uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6)
{
    (void)a3; (void)a4; (void)a5; (void)a6;
    if (!handle_out_ptr || !name_ptr) return (uint64_t)-1;

    KTHREAD *cur = KeGetCurrentThread();
    if (!cur || !cur->Process) return (uint64_t)-1;

    char path[256];
    const char *uname = (const char *)name_ptr;
    int i = 0;
    for (; i < 255; i++) { path[i] = uname[i]; if (!uname[i]) break; }
    path[i] = 0;
    if (i == 255) return (uint64_t)-1;

    char kname[OBJ_NAME_MAX] = {0};
    OBJECT_HEADER *dir = 0;
    if (ObpParsePath(path, &dir, kname, OBJ_NAME_MAX) != 0) {
        KdPrintf("[syscall] NtOpenEvent: bad path '%s'\n", path);
        return (uint64_t)-1;
    }

    OBJECT_HEADER *hdr = ObpLookupObjectByName(dir, kname);
    if (!hdr || hdr->Type != &ObpEventType) {
        KdPrintf("[syscall] NtOpenEvent: '%s' not found\n", kname);
        return (uint64_t)-1;
    }

    HANDLE h = ObpAllocateHandle(cur->Process, hdr, 0);
    if (!h) return (uint64_t)-1;
    *(uint64_t *)handle_out_ptr = (uint64_t)h;

    KdPrintf("[syscall] NtOpenEvent '%s' -> h=%llu\n",
             kname, (unsigned long long)h);
    return 0;
}

static uint64_t NtSetEvent(uint64_t handle, uint64_t prev_state_ptr,
                           uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6)
{
    (void)a3; (void)a4; (void)a5; (void)a6;

    KTHREAD *cur = KeGetCurrentThread();
    if (!cur || !cur->Process) return (uint64_t)-1;

    OBJECT_HEADER *hdr = ObpLookupHandle(cur->Process, (HANDLE)handle, &ObpMutantType);
    if (!hdr) return (uint64_t)-1;

    if (prev_state_ptr)
        *(uint64_t *)prev_state_ptr = (uint64_t)hdr->SignalState;

    ObSignalObject(hdr);
    KdPrintf("[syscall] NtSetEvent h=%llu\n", (unsigned long long)handle);
    return 0;
}

static uint64_t NtResetEvent(uint64_t handle, uint64_t prev_state_ptr,
                             uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6)
{
    (void)a3; (void)a4; (void)a5; (void)a6;

    KTHREAD *cur = KeGetCurrentThread();
    if (!cur || !cur->Process) return (uint64_t)-1;

    OBJECT_HEADER *hdr = ObpLookupHandle(cur->Process, (HANDLE)handle, &ObpMutantType);
    if (!hdr) return (uint64_t)-1;

    if (prev_state_ptr)
        *(uint64_t *)prev_state_ptr = (uint64_t)hdr->SignalState;

    ObResetObject(hdr);
    KdPrintf("[syscall] NtResetEvent h=%llu\n", (unsigned long long)handle);
    return 0;
}

static uint64_t NtPulseEvent(uint64_t handle, uint64_t prev_state_ptr,
                             uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6)
{
    (void)a3; (void)a4; (void)a5; (void)a6;

    KTHREAD *cur = KeGetCurrentThread();
    if (!cur || !cur->Process) return (uint64_t)-1;

    OBJECT_HEADER *hdr = ObpLookupHandle(cur->Process, (HANDLE)handle, &ObpMutantType);
    if (!hdr) return (uint64_t)-1;

    if (prev_state_ptr)
        *(uint64_t *)prev_state_ptr = (uint64_t)hdr->SignalState;

    ObSignalObject(hdr);
    ObResetObject(hdr);
    KdPrintf("[syscall] NtPulseEvent h=%llu\n", (unsigned long long)handle);
    return 0;
}

static uint64_t NtCreateSemaphore(uint64_t handle_out_ptr,
                                  uint64_t initial_count,
                                  uint64_t max_count,
                                  uint64_t name_ptr,
                                  uint64_t a5, uint64_t a6)
{
    (void)a5; (void)a6;
    if (!handle_out_ptr) return (uint64_t)-1;
    if (initial_count > max_count) return (uint64_t)-1;

    KTHREAD *cur = KeGetCurrentThread();
    if (!cur || !cur->Process) return (uint64_t)-1;

    char kname[OBJ_NAME_MAX] = {0};
    int named = 0;

    if (name_ptr) {
        char path[256];
        const char *uname = (const char *)name_ptr;
        int i = 0;
        for (; i < 255; i++) { path[i] = uname[i]; if (!uname[i]) break; }
        path[i] = 0;
        if (i == 255) return (uint64_t)-1;

        OBJECT_HEADER *dir = 0;
        if (ObpParsePath(path, &dir, kname, OBJ_NAME_MAX) != 0) return (uint64_t)-1;
        if (ObpLookupObjectByName(dir, kname)) return (uint64_t)-1;
        named = 1;
    }

    KSEMAPHORE *s = (KSEMAPHORE *)ObCreateObject(&ObpSemaphoreType,
                                                  sizeof(KSEMAPHORE));
    if (!s) return (uint64_t)-1;

    s->Limit = (int32_t)max_count;
    s->Count = (int32_t)initial_count;

    OBJECT_HEADER *hdr = OBJ_HEADER(s);
    hdr->SignalState = (initial_count > 0) ? 1 : 0;

    if (named) {
        if (ObpInsertObjectByName(ObpBaseNamedObjects, kname, hdr) != 0) {
            ObDereferenceObject(s);
            return (uint64_t)-1;
        }
    }

    HANDLE h = ObpAllocateHandle(cur->Process, hdr, 0);
    if (!h) {
        if (named) ObpDeleteObjectByName(ObpBaseNamedObjects, kname);
        ObDereferenceObject(s);
        return (uint64_t)-1;
    }
    if (!named) ObDereferenceObject(s);

    *(uint64_t *)handle_out_ptr = (uint64_t)h;
    KdPrintf("[syscall] NtCreateSemaphore initial=%llu max=%llu name='%s' -> h=%llu\n",
             (unsigned long long)initial_count,
             (unsigned long long)max_count,
             named ? kname : "(unnamed)",
             (unsigned long long)h);
    return 0;
}

static uint64_t NtOpenSemaphore(uint64_t handle_out_ptr, uint64_t name_ptr,
                                uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6)
{
    (void)a3; (void)a4; (void)a5; (void)a6;
    if (!handle_out_ptr || !name_ptr) return (uint64_t)-1;

    KTHREAD *cur = KeGetCurrentThread();
    if (!cur || !cur->Process) return (uint64_t)-1;

    char path[256];
    const char *uname = (const char *)name_ptr;
    int i = 0;
    for (; i < 255; i++) { path[i] = uname[i]; if (!uname[i]) break; }
    path[i] = 0;
    if (i == 255) return (uint64_t)-1;

    char kname[OBJ_NAME_MAX] = {0};
    OBJECT_HEADER *dir = 0;
    if (ObpParsePath(path, &dir, kname, OBJ_NAME_MAX) != 0) return (uint64_t)-1;

    OBJECT_HEADER *hdr = ObpLookupObjectByName(dir, kname);
    if (!hdr || hdr->Type != &ObpSemaphoreType) return (uint64_t)-1;

    HANDLE h = ObpAllocateHandle(cur->Process, hdr, 0);
    if (!h) return (uint64_t)-1;
    *(uint64_t *)handle_out_ptr = (uint64_t)h;

    KdPrintf("[syscall] NtOpenSemaphore '%s' -> h=%llu\n",
             kname, (unsigned long long)h);
    return 0;
}

static uint64_t NtReleaseSemaphore(uint64_t handle, uint64_t release_count,
                                   uint64_t prev_count_ptr,
                                   uint64_t a4, uint64_t a5, uint64_t a6)
{
    (void)a4; (void)a5; (void)a6;

    KTHREAD *cur = KeGetCurrentThread();
    if (!cur || !cur->Process) return (uint64_t)-1;

    OBJECT_HEADER *hdr = ObpLookupHandle(cur->Process, (HANDLE)handle, &ObpMutantType);
    if (!hdr) return (uint64_t)-1;

    KSEMAPHORE *s = (KSEMAPHORE *)OBJ_BODY(hdr);

    if (prev_count_ptr)
        *(uint64_t *)prev_count_ptr = (uint64_t)s->Count;

    int32_t new_count = s->Count + (int32_t)release_count;
    if (new_count > s->Limit) {
        KdPrintf("[syscall] NtReleaseSemaphore: overflow (%d > %d)\n",
                 new_count, s->Limit);
        return (uint64_t)-1;
    }

    s->Count = new_count;
    ObSignalObject(hdr);

    KdPrintf("[syscall] NtReleaseSemaphore h=%llu +%llu -> count=%d\n",
             (unsigned long long)handle,
             (unsigned long long)release_count,
             s->Count);
    return 0;
}


static uint64_t NtWriteConsole(uint64_t str_ptr,
                               uint64_t a2, uint64_t a3, uint64_t a4,
                               uint64_t a5, uint64_t a6)
{
    (void)a2; (void)a3; (void)a4; (void)a5; (void)a6;
    if (!str_ptr) return (uint64_t)-1;

    const char *usr = (const char *)str_ptr;
    __asm__ volatile ("cli");

    char buf[512];
    int i = 0;
    for (; i < 511; i++) {
        char c = usr[i];
        buf[i] = c;
        if (c == 0) break;
    }
    buf[i] = 0;

    KdBeginUserConsole();
    FbPrint(buf);

    __asm__ volatile ("sti");
    return 0;
}

static uint64_t NtReadConsole(uint64_t buf_ptr, uint64_t max_len,
                              uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6)
{
    (void)a3; (void)a4; (void)a5; (void)a6;
    if (!buf_ptr || max_len == 0) return (uint64_t)-1;

    char kbuf[256];
    if (max_len > 255) max_len = 255;

    int n = KbdReadLine(kbuf, (int)max_len);

    char *usr = (char *)buf_ptr;
    for (int i = 0; i < n; i++) usr[i] = kbuf[i];
    usr[n] = 0;

    return (uint64_t)n;
}

typedef struct {
    char     Name[64];
    uint32_t Attr;
    uint32_t Size;
} K_FUSION_DIRENT;

typedef struct {
    K_FUSION_DIRENT *Buf;
    uint32_t         Max;
    uint32_t         Count;
} K_LIST_CTX;

static int k_list_cb(const char *Name, uint32_t Attr, uint32_t Size, void *Ctx)
{
    K_LIST_CTX *c = (K_LIST_CTX *)Ctx;
    if (c->Count >= c->Max) return 1;

    K_FUSION_DIRENT *e = &c->Buf[c->Count++];
    int i = 0;
    while (i < 63 && Name[i]) { e->Name[i] = Name[i]; i++; }
    e->Name[i] = 0;
    e->Attr = Attr;
    e->Size = Size;
    return 0;
}


static uint64_t NtQueryDirectory(uint64_t path_ptr, uint64_t buf_ptr,
                                 uint64_t max_entries, uint64_t out_count_ptr,
                                 uint64_t a5, uint64_t a6)
{
    (void)a5; (void)a6;
    KdPrintf("[ntquery] enter path=0x%llx buf=0x%llx max=%llu\n",
             (unsigned long long)path_ptr,
             (unsigned long long)buf_ptr,
             (unsigned long long)max_entries);

    if (!path_ptr || !buf_ptr || max_entries == 0) return (uint64_t)-1;

    char kpath[256];
    const char *up = (const char *)path_ptr;
    int i = 0;
    for (; i < 255; i++) { kpath[i] = up[i]; if (!up[i]) break; }
    kpath[i] = 0;
    if (i == 255) return (uint64_t)-1;

    KdPrintf("[ntquery] path='%s'\n", kpath);

    K_LIST_CTX ctx;
    ctx.Buf   = (K_FUSION_DIRENT *)buf_ptr;
    ctx.Max   = (uint32_t)max_entries;
    ctx.Count = 0;

    int rc = VfsListDir(kpath, k_list_cb, &ctx);

    KdPrintf("[ntquery] VfsListDir rc=%d count=%u\n", rc, ctx.Count);

    if (out_count_ptr)
        *(uint64_t *)out_count_ptr = ctx.Count;

    return (rc == 0) ? 0 : (uint64_t)-1;
}

typedef uint64_t (*NtSyscallFn)(uint64_t a1, uint64_t a2, uint64_t a3,
                                uint64_t a4, uint64_t a5, uint64_t a6);

static NtSyscallFn gSyscallTable[256];

void KiRegisterSyscall(uint32_t Nr, NtSyscallFn Fn)
{
    if (Nr < 256) gSyscallTable[Nr] = Fn;
}

void KiInitSyscalls(void)
{
    KiInitSyscallMsrsForCurrentCpu();
    for (int i = 0; i < 256; i++) gSyscallTable[i] = 0;

    KiRegisterSyscall(SYS_NT_CREATE_MUTANT,           NtCreateMutant);
    KiRegisterSyscall(SYS_NT_RELEASE_MUTANT,          NtReleaseMutant);
    KiRegisterSyscall(SYS_NT_TERMINATE_PROCESS,       NtTerminateProcess);
    KiRegisterSyscall(SYS_NT_DISPLAY_STRING,          NtDisplayString);
    KiRegisterSyscall(SYS_NT_GET_TICK_COUNT,          NtGetTickCount);
    KiRegisterSyscall(SYS_NT_DELAY_EXECUTION,         NtDelayExecution);
    KiRegisterSyscall(SYS_NT_READ_FILE,               NtReadFile);
    KiRegisterSyscall(SYS_NT_CREATE_PROCESS,          NtCreateProcess);
    KiRegisterSyscall(SYS_NT_WAIT_FOR_SINGLE_OBJECT,  NtWaitForSingleObject);
    KiRegisterSyscall(SYS_NT_CLOSE,                   NtClose);
    KiRegisterSyscall(SYS_NT_CREATE_EVENT,            NtCreateEvent);
    KiRegisterSyscall(SYS_NT_SET_EVENT,               NtSetEvent);
    KiRegisterSyscall(SYS_NT_RESET_EVENT,             NtResetEvent);
    KiRegisterSyscall(SYS_NT_PULSE_EVENT,             NtPulseEvent);
    KiRegisterSyscall(SYS_NT_CREATE_SEMAPHORE,        NtCreateSemaphore);
    KiRegisterSyscall(SYS_NT_RELEASE_SEMAPHORE,       NtReleaseSemaphore);
    KiRegisterSyscall(SYS_NT_OPEN_EVENT,              NtOpenEvent);
    KiRegisterSyscall(SYS_NT_OPEN_MUTANT,             NtOpenMutant);
    KiRegisterSyscall(SYS_NT_OPEN_SEMAPHORE,          NtOpenSemaphore);
    KiRegisterSyscall(SYS_NT_WRITE_CONSOLE,           NtWriteConsole);
    KiRegisterSyscall(SYS_NT_READ_CONSOLE,            NtReadConsole);
    KiRegisterSyscall(SYS_NT_ALLOCATE_VM,             NtAllocateVirtualMemory);
    KiRegisterSyscall(SYS_NT_FREE_VM,                 NtFreeVirtualMemory);
    KiRegisterSyscall(SYS_NT_QUERY_DIRECTORY,         NtQueryDirectory);
    
    KiRegisterImageSectionSyscalls();
    KdPrint("[syscall] tables ready \n");
}

void KiSystemCall(INTERRUPT_FRAME *frame)
{
    static int gDbg = 0;
    if (gDbg < 20) {
        uint64_t cr3;
        __asm__ volatile ("mov %%cr3, %0" : "=r"(cr3));
        KdPrintf("[sysdbg] nr=%llu cr3=0x%llx user_rip=0x%llx\n",
                 (unsigned long long)frame->rax,
                 (unsigned long long)(cr3 & ~0xFFFULL),
                 (unsigned long long)frame->rip);
        gDbg++;
    }

    uint64_t saved_rip    = frame->rip;
    uint64_t saved_cs     = frame->cs;
    uint64_t saved_rflags = frame->rflags;
    uint64_t saved_rsp    = frame->rsp;
    uint64_t saved_ss     = frame->ss;
    uint64_t nr           = frame->rax;

    if (nr < 256 && gSyscallTable[nr]) {
        frame->rax = gSyscallTable[nr](frame->rdi, frame->rsi, frame->rdx,
                                       frame->r10, frame->r8,  frame->r9);
    } else {
        frame->rax = (uint64_t)-1;
    }

    frame->rip    = saved_rip;
    frame->cs     = saved_cs;
    frame->rflags = saved_rflags;
    frame->rsp    = saved_rsp;
    frame->ss     = saved_ss;
}

static uint64_t gUserHeapNext = USER_HEAP_BASE;

static uint64_t vm_alloc_region(uint64_t Size)
{
    uint64_t Pages = (Size + 0xFFF) / 0x1000;
    uint64_t Bytes = Pages * 0x1000;

    if (gUserHeapNext + Bytes > USER_HEAP_END) {
        KdPrint("[vm] heap exhausted\n");
        return 0;
    }

    uint64_t Addr = gUserHeapNext;
    gUserHeapNext += Bytes;
    return Addr;
}

static uint64_t NtAllocateVirtualMemory(uint64_t base_ptr, uint64_t zero_bits,
                                        uint64_t size_ptr, uint64_t alloc_type,
                                        uint64_t protect,
                                        uint64_t a6)
{
    (void)zero_bits; (void)alloc_type; (void)protect; (void)a6;

    if (!base_ptr || !size_ptr) return (uint64_t)-1;

    uint64_t *user_base = (uint64_t *)base_ptr;
    uint64_t *user_size = (uint64_t *)size_ptr;

    uint64_t req_size = *user_size;
    uint64_t req_base = *user_base;

    if (req_size == 0) return (uint64_t)-1;

    uint64_t va;
    if (req_base == 0) {
        va = vm_alloc_region(req_size);
        if (!va) return (uint64_t)-1;
    } else {
        va = req_base;
    }

    uint64_t pages = (req_size + 0xFFF) / 0x1000;

    KTHREAD  *cur = KeGetCurrentThread();
    EPROCESS  *p  = cur->Process;
    uint64_t pml4 = p->Pml4Phys;

    uint64_t mapped = 0;
    for (; mapped < pages; mapped++) {
        void *phys = PmmAllocatePage();
        if (!phys) {
            KdPrint("[vm] NtAllocateVM: phys alloc failed\n");
            goto rollback;
        }
        uint8_t *p8 = (uint8_t *)phys;
        for (int j = 0; j < 4096; j++) p8[j] = 0;

        if (MiMapPage(pml4, va + mapped * 0x1000,
                      (uint64_t)(uintptr_t)phys,
                      PTE_PRESENT | PTE_WRITABLE | PTE_USER) != 0) {
            PmmFreePage(phys);
            KdPrint("[vm] NtAllocateVM: MiMapPage failed\n");
            goto rollback;
        }
    }

    *user_base = va;
    *user_size = pages * 0x1000;

    KdPrintf("[vm] NtAllocateVM: base=0x%llx size=0x%llx pages=%llu\n",
             (unsigned long long)va,
             (unsigned long long)(pages * 0x1000),
             (unsigned long long)pages);
    return 0;

rollback:
    for (uint64_t k = 0; k < mapped; k++) {
        uint64_t phys = 0;
        if (MiUnmapPage(pml4, va + k * 0x1000, &phys) == 0 && phys)
            PmmFreePage((void *)(uintptr_t)phys);
    }
    return (uint64_t)-1;
}

static uint64_t NtFreeVirtualMemory(uint64_t base_ptr, uint64_t size_ptr,
                                    uint64_t free_type,
                                    uint64_t a4, uint64_t a5, uint64_t a6)
{
    (void)a4; (void)a5; (void)a6;
    if (!base_ptr) return (uint64_t)-1;

    uint64_t va = *(uint64_t *)base_ptr;
    if (!va) return 0;

    uint64_t pages = 0;
    if (size_ptr) {
        uint64_t sz = *(uint64_t *)size_ptr;
        pages = (sz + 0xFFF) / 0x1000;
    }

    KTHREAD *cur = KeGetCurrentThread();
    EPROCESS *p  = cur->Process;
    uint64_t pml4 = p->Pml4Phys;

    if (pages == 0) {
        while (1) {
            uint64_t probe = va + pages * 0x1000;
            if (pages >= 256) break;
            if (!MiIsMapped(pml4, probe)) break;
            pages++;
        }
    }

    for (uint64_t i = 0; i < pages; i++) {
        uint64_t phys = 0;
        if (MiUnmapPage(pml4, va + i * 0x1000, &phys) == 0 && phys)
            PmmFreePage((void *)(uintptr_t)phys);
    }

    KdPrintf("[vm] NtFreeVM: base=0x%llx pages=%llu\n",
             (unsigned long long)va, (unsigned long long)pages);
    return 0;
}