#ifndef NTOS_LOADER_H
#define NTOS_LOADER_H

#include <stdint.h>
#include <nt/ntdefs.h>
#include <nt/ntstatus.h>
#include "imgsec.h"

struct _EPROCESS;
typedef struct _EPROCESS EPROCESS;

#define IMG_LOAD_MAX_SIZE  0x1000000

typedef struct _IMAGE_EXPORTS {
    char     Name[64];
    uint64_t Base;
    uint64_t Size;
    uint32_t ExportCount;
    struct {
        char     Name[64];
        uint32_t Rva;
    } Exports[256];
    struct _IMAGE_EXPORTS *Next;
} IMAGE_EXPORTS;

NTSTATUS KipLoadImage(EPROCESS *Proc, const char *Path,
                      IMAGE_EXPORTS *Fallback, int ResolveSelf,
                      IMAGE_SECTION_ENTRY **OutSec,
                      IMAGE_EXPORTS **OutExports,
                      uint64_t *OutEntry);

IMAGE_EXPORTS *KipGetNtdllExports(void);
void KipResetNtdllExports(void);

#endif /* NTOS_LOADER_H */