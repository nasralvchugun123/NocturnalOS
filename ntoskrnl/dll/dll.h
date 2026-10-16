#ifndef NTOS_DLL_H
#define NTOS_DLL_H
#include "imgsec.h"
#include "loader.h"

#ifndef NTOS_DLL_H
#define NTOS_DLL_H

#include <stdint.h>
#include <nt/ntdefs.h>
#include <nt/ntstatus.h>

#define DLL_MAX_LOADED   95
#define DLL_NAME_MAX     64
#define DLL_PATH_MAX     256
#define DLL_EXPORT_MAX   256
#define DLL_PAGES_MAX    512
#define DLL_DEPS_MAX     16

typedef struct _DLL_ENTRY {
    char     Name[DLL_NAME_MAX];
    char     Path[DLL_PATH_MAX];
    uint64_t Base;
    uint64_t Size;
    uint64_t PhysPages[DLL_PAGES_MAX];
    uint32_t PageCount;
    uint32_t RefCount;

    struct _DLL_ENTRY *Deps[DLL_DEPS_MAX];
    uint32_t DepCount;

    struct { char Name[64]; uint32_t Rva; } Exports[DLL_EXPORT_MAX];
    uint32_t ExportCount;
} DLL_ENTRY;

int        DllInitSystem(void);
DLL_ENTRY *DllFindByName(const char *Name);
DLL_ENTRY *DllLoad(const char *Name, const char *Path);
NTSTATUS   DllEnsureMapped(DLL_ENTRY *e, uint64_t Pml4Phys);
void       DllReleaseForProcess(uint64_t Pml4Phys);
void      *DllGetProcAddress(DLL_ENTRY *e, const char *Func);

#endif