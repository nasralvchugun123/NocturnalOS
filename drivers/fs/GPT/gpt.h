
#ifndef GPT_H
#define GPT_H

#include <stdint.h>

typedef struct {
    uint8_t  TypeGuid[16];
    uint8_t  UniqueGuid[16];
    uint64_t FirstLba;
    uint64_t LastLba;
    uint64_t Attributes;
    uint16_t NameUtf16[36];
} __attribute__((packed)) GPT_ENTRY;

#define GPT_MAX_PARTITIONS 16

typedef struct {
    uint64_t  DiskLba1;           
    uint64_t  FirstUsableLba;
    uint64_t  LastUsableLba;
    uint32_t  EntryCount;
    uint32_t  EntrySize;

    int       PartitionCount;
    GPT_ENTRY Partitions[GPT_MAX_PARTITIONS];
} GPT_INFO;

extern const uint8_t GUID_TYPE_EFI_SYSTEM[16];      /* C12A7328-F81F-11D2-BA4B-00A0C93EC93B */
extern const uint8_t GUID_TYPE_MS_BASIC_DATA[16];   /* EBD0A0A2-B9E5-4433-87C0-68B6B72699C7 */

int GptParse(void *ReadSectors, GPT_INFO *Out);

int GptFindPartition(GPT_INFO *Info, const uint8_t *TypeGuid, GPT_ENTRY **Out);
void GptFormatGuid(const uint8_t *Guid, char *Out /* >=37 chars */);
void GptFormatName(const uint16_t *NameUtf16, char *Out, int OutSize);

#endif