#ifndef NTOS_IMGSEC_H
#define NTOS_IMGSEC_H

#include <stdint.h>
#include <nt/ntdefs.h>
#include <nt/ntstatus.h>

#define IMG_SEC_MAX         64
#define IMG_SEC_MAX_PAGES   4096
#define IMG_SEC_MAX_MAPS    32
#define IMG_SEC_NAME_MAX    64

typedef struct _IMAGE_SECTION_ENTRY {
    int      InUse;
    char     Name[IMG_SEC_NAME_MAX];
    uint64_t Base;
    uint64_t Size;
    uint32_t PageCount;
    uint64_t PhysPages[IMG_SEC_MAX_PAGES];
    uint32_t RefCount;
    uint32_t MappedPml4[IMG_SEC_MAX_MAPS];
    uint32_t MappedCount;
} IMAGE_SECTION_ENTRY;

void     ImgSecInit(void);
IMAGE_SECTION_ENTRY *ImgSecFind(const char *Name);
IMAGE_SECTION_ENTRY *ImgSecFindByBase(uint64_t Base);

NTSTATUS ImgSecCreate(const char *Name, const void *Flattened,
                      uint64_t Size, uint64_t PreferredBase,
                      IMAGE_SECTION_ENTRY **Out);
NTSTATUS ImgSecMap(IMAGE_SECTION_ENTRY *E, uint64_t Pml4);
NTSTATUS ImgSecUnmap(IMAGE_SECTION_ENTRY *E, uint64_t Pml4);
void     ImgSecReleaseAllForPml4(uint64_t Pml4);

#endif /* NTOS_IMGSEC_H */