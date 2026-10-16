#include "imgsec.h"
#include "loader.h"
#include "../mm/mm.h"
#include "../ps/ps.h"
#include <rt.h>
#include <dbg/print/kdprint.h>
#include <nt/syscall.h>

extern KTHREAD *KeGetCurrentThread(void);

static uint64_t NtCreateImageSection(uint64_t name_ptr, uint64_t buf_ptr,
                                     uint64_t size, uint64_t pref_base,
                                     uint64_t out_base_ptr,
                                     uint64_t a6)
{
    (void)a6;
    if (!name_ptr || !buf_ptr || !out_base_ptr) return (uint64_t)-1;

    KTHREAD *cur = KeGetCurrentThread();
    if (!cur || !cur->Process) return (uint64_t)-1;

    char kname[IMG_SEC_NAME_MAX];
    const char *un = (const char *)name_ptr;
    int i = 0;
    for (; i < IMG_SEC_NAME_MAX - 1; i++) { kname[i] = un[i]; if (!un[i]) break; }
    kname[i] = 0;

    IMAGE_SECTION_ENTRY *sec = 0;
    NTSTATUS st = ImgSecCreate(kname, (const void *)buf_ptr, size,
                               pref_base, &sec);
    if (!NT_SUCCESS(st)) return (uint64_t)(int64_t)st;

    st = ImgSecMap(sec, cur->Process->Pml4Phys);
    if (!NT_SUCCESS(st)) return (uint64_t)(int64_t)st;

    *(uint64_t *)out_base_ptr = sec->Base;
    return 0;
}

static uint64_t NtMapImageSection(uint64_t name_ptr, uint64_t out_base_ptr,
                                  uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6)
{
    (void)a3;(void)a4;(void)a5;(void)a6;
    if (!name_ptr || !out_base_ptr) return (uint64_t)-1;

    KTHREAD *cur = KeGetCurrentThread();
    if (!cur || !cur->Process) return (uint64_t)-1;

    char kname[IMG_SEC_NAME_MAX];
    const char *un = (const char *)name_ptr;
    int i = 0;
    for (; i < IMG_SEC_NAME_MAX - 1; i++) { kname[i] = un[i]; if (!un[i]) break; }
    kname[i] = 0;

    IMAGE_SECTION_ENTRY *sec = ImgSecFind(kname);
    if (!sec) return (uint64_t)(int64_t)STATUS_NOT_FOUND;

    NTSTATUS st = ImgSecMap(sec, cur->Process->Pml4Phys);
    if (!NT_SUCCESS(st)) return (uint64_t)(int64_t)st;

    *(uint64_t *)out_base_ptr = sec->Base;
    return 0;
}

static uint64_t NtUnmapImageSection(uint64_t name_ptr, uint64_t a2,
                                    uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6)
{
    (void)a2;(void)a3;(void)a4;(void)a5;(void)a6;
    if (!name_ptr) return (uint64_t)-1;

    KTHREAD *cur = KeGetCurrentThread();
    if (!cur || !cur->Process) return (uint64_t)-1;

    char kname[IMG_SEC_NAME_MAX];
    const char *un = (const char *)name_ptr;
    int i = 0;
    for (; i < IMG_SEC_NAME_MAX - 1; i++) { kname[i] = un[i]; if (!un[i]) break; }
    kname[i] = 0;

    IMAGE_SECTION_ENTRY *sec = ImgSecFind(kname);
    if (!sec) return (uint64_t)(int64_t)STATUS_NOT_FOUND;
    return (uint64_t)(int64_t)ImgSecUnmap(sec, cur->Process->Pml4Phys);
}

static uint64_t NtQueryImageSection(uint64_t name_ptr, uint64_t out_base_ptr,
                                    uint64_t out_size_ptr,
                                    uint64_t a4, uint64_t a5, uint64_t a6)
{
    (void)a4;(void)a5;(void)a6;
    if (!name_ptr) return (uint64_t)-1;

    char kname[IMG_SEC_NAME_MAX];
    const char *un = (const char *)name_ptr;
    int i = 0;
    for (; i < IMG_SEC_NAME_MAX - 1; i++) { kname[i] = un[i]; if (!un[i]) break; }
    kname[i] = 0;

    IMAGE_SECTION_ENTRY *sec = ImgSecFind(kname);
    if (!sec) return (uint64_t)(int64_t)STATUS_NOT_FOUND;

    if (out_base_ptr) *(uint64_t *)out_base_ptr = sec->Base;
    if (out_size_ptr) *(uint64_t *)out_size_ptr = sec->Size;
    return 0;
}

static uint64_t NtProtectVirtualMemory(uint64_t base_ptr, uint64_t size_ptr,
                                       uint64_t new_protect, uint64_t old_protect_ptr,
                                       uint64_t a5, uint64_t a6)
{
    (void)new_protect;(void)old_protect_ptr;(void)a5;(void)a6;
    if (!base_ptr) return (uint64_t)-1;

    KTHREAD *cur = KeGetCurrentThread();
    if (!cur || !cur->Process) return (uint64_t)-1;

    uint64_t va = *(uint64_t *)base_ptr;
    uint64_t sz = size_ptr ? *(uint64_t *)size_ptr : 0;
    uint32_t pages = (uint32_t)((sz + 0xFFF) / 0x1000);
    if (pages == 0) pages = 1;

    for (uint32_t i = 0; i < pages; i++) {
        uint64_t phys = 0;
        if (MiUnmapPage(cur->Process->Pml4Phys, va + i * 0x1000, &phys) != 0)
            return (uint64_t)-1;
        if (MiMapPage(cur->Process->Pml4Phys, va + i * 0x1000, phys,
                      PTE_PRESENT | PTE_WRITABLE | PTE_USER) != 0)
            return (uint64_t)-1;
    }
    return 0;
}

static uint64_t NtCurrentPeb(uint64_t out_ptr, uint64_t a2, uint64_t a3,
                             uint64_t a4, uint64_t a5, uint64_t a6)
{
    (void)a2;(void)a3;(void)a4;(void)a5;(void)a6;
    if (!out_ptr) return (uint64_t)-1;
    KTHREAD *cur = KeGetCurrentThread();
    if (!cur || !cur->Process) return (uint64_t)-1;
    *(uint64_t *)out_ptr = cur->Process->Peb;
    return 0;
}

static uint64_t NtGetModuleHandle(uint64_t name_ptr, uint64_t out_base_ptr,
                                  uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6)
{
    (void)a3;(void)a4;(void)a5;(void)a6;
    if (!name_ptr || !out_base_ptr) return (uint64_t)-1;

    char kname[IMG_SEC_NAME_MAX];
    const char *un = (const char *)name_ptr;
    int i = 0;
    for (; i < IMG_SEC_NAME_MAX - 1; i++) { kname[i] = un[i]; if (!un[i]) break; }
    kname[i] = 0;

    IMAGE_SECTION_ENTRY *sec = ImgSecFind(kname);
    if (!sec) return (uint64_t)(int64_t)STATUS_NOT_FOUND;
    *(uint64_t *)out_base_ptr = sec->Base;
    return 0;
}

void KiRegisterImageSectionSyscalls(void);

typedef uint64_t (*NtSyscallFn)(uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t);
extern void KiRegisterSyscall(uint32_t Nr, NtSyscallFn Fn);

void KiRegisterImageSectionSyscalls(void)
{
    KiRegisterSyscall(SYS_NT_CREATE_IMAGE_SECTION, NtCreateImageSection);
    KiRegisterSyscall(SYS_NT_MAP_IMAGE_SECTION,    NtMapImageSection);
    KiRegisterSyscall(SYS_NT_UNMAP_IMAGE_SECTION,  NtUnmapImageSection);
    KiRegisterSyscall(SYS_NT_QUERY_IMAGE_SECTION,  NtQueryImageSection);
    KiRegisterSyscall(SYS_NT_PROTECT_VM,           NtProtectVirtualMemory);
    KiRegisterSyscall(SYS_NT_CURRENT_PEB,          NtCurrentPeb);
    KiRegisterSyscall(SYS_NT_GET_MODULE_HANDLE,    NtGetModuleHandle);
}