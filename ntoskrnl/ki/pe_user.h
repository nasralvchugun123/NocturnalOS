
#ifndef USER_PE_H
#define USER_PE_H

#include <stdint.h>

typedef struct {
    uint64_t EntryPoint;
    uint64_t ImageBase;
    uint64_t ImageSize;
    int      SectionsLoaded;
} USER_PE_RESULT;

int UserLoadPE(const uint8_t *FileBuf, uint32_t FileSize,
               uint64_t TargetBase, uint64_t Pml4Phys, USER_PE_RESULT *Out);

#endif