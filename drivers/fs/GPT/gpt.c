
#include <dbg/print/kdprint.h>
#include <ds/fb/fb.h>
#include "gpt.h"
#include <rt.h>
#include "../../../ntoskrnl/mm/mm.h"


typedef int (*read_sectors_fn)(uint64_t, uint32_t, void *);

const uint8_t GUID_TYPE_EFI_SYSTEM[16] = {
    0x28, 0x73, 0x2A, 0xC1, 0x1F, 0xF8, 0xD2, 0x11,
    0xBA, 0x4B, 0x00, 0xA0, 0xC9, 0x3E, 0xC9, 0x3B
};
const uint8_t GUID_TYPE_MS_BASIC_DATA[16] = {
    0xA2, 0xA0, 0xD0, 0xEB, 0xE5, 0xB9, 0x33, 0x44,
    0x87, 0xC0, 0x68, 0xB6, 0xB7, 0x26, 0x99, 0xC7
};

// fst 92b
typedef struct {
    uint8_t  Signature[8];       
    uint32_t Revision;
    uint32_t HeaderSize;
    uint32_t HeaderCrc32;
    uint32_t Reserved;
    uint64_t CurrentLba;
    uint64_t BackupLba;
    uint64_t FirstUsableLba;
    uint64_t LastUsableLba;
    uint8_t  DiskGuid[16];
    uint64_t PartitionEntryLba;
    uint32_t NumberOfPartitionEntries;
    uint32_t SizeOfPartitionEntry;
    uint32_t PartitionEntryArrayCrc32;
} __attribute__((packed)) GPT_HEADER;

static int guid_is_zero(const uint8_t *g)
{
    for (int i = 0; i < 16; i++) if (g[i]) return 0;
    return 1;
}

static int guid_equal(const uint8_t *a, const uint8_t *b)
{
    for (int i = 0; i < 16; i++) if (a[i] != b[i]) return 0;
    return 1;
}

int GptParse(void *ReadSectors, GPT_INFO *Out)
{
    read_sectors_fn rd = (read_sectors_fn)ReadSectors;

    for (uint64_t i = 0; i < sizeof(GPT_INFO); i++) ((uint8_t *)Out)[i] = 0;

    uint8_t *hdr_buf = (uint8_t *)MmAllocatePages(1);
    if (!hdr_buf) return -1;
    if (rd(1, 1, hdr_buf) != 0) {
        KdPrint("[GPT] read LBA1 failed\n");
        MmFreePages(hdr_buf, 1);
        return -2;
    }

    GPT_HEADER *h = (GPT_HEADER *)hdr_buf;

    const char sig[8] = {'E','F','I',' ','P','A','R','T'};
    for (int i = 0; i < 8; i++) {
        if (h->Signature[i] != (uint8_t)sig[i]) {
            KdPrintf("[GPT] bad signature: %02x %02x %02x %02x %02x %02x %02x %02x\n",
                     h->Signature[0], h->Signature[1], h->Signature[2], h->Signature[3],
                     h->Signature[4], h->Signature[5], h->Signature[6], h->Signature[7]);
            MmFreePages(hdr_buf, 1);
            return -3;
        }
    }

    Out->DiskLba1       = 1;
    Out->FirstUsableLba = h->FirstUsableLba;
    Out->LastUsableLba  = h->LastUsableLba;
    Out->EntryCount     = h->NumberOfPartitionEntries;
    Out->EntrySize      = h->SizeOfPartitionEntry;

    KdPrintf("[GPT] header OK: entries=%u entry_size=%u first=%llu last=%llu\n",
             h->NumberOfPartitionEntries, h->SizeOfPartitionEntry,
             (unsigned long long)h->FirstUsableLba,
             (unsigned long long)h->LastUsableLba);

    if (Out->EntryCount > 128 || Out->EntrySize < 128) {
        KdPrint("[GPT] unsupported entry count/size\n");
        MmFreePages(hdr_buf, 1);
        return -4;
    }

    uint64_t table_size = (uint64_t)Out->EntryCount * Out->EntrySize;
    uint32_t sectors_needed = (uint32_t)((table_size + 511) / 512);
    uint32_t pages_needed   = (uint32_t)((table_size + 4095) / 4096);
    if (pages_needed > 32) pages_needed = 32;

    uint8_t *table = (uint8_t *)MmAllocatePages(pages_needed);
    if (!table) { MmFreePages(hdr_buf, 1); return -5; }

    uint64_t read_lba = h->PartitionEntryLba;
    for (uint32_t off = 0; off < sectors_needed; off += 8) {
        uint32_t chunk = 8;
        if (off + chunk > sectors_needed) chunk = sectors_needed - off;
        if (rd(read_lba + off, chunk, table + off * 512) != 0) {
            KdPrintf("[GPT] read fail at lba=%llu count=%u\n",
                     (unsigned long long)(read_lba + off), chunk);
            MmFreePages(table, pages_needed);
            MmFreePages(hdr_buf, 1);
            return -6;
        }
    }
    MmFreePages(hdr_buf, 1);

    KdPrintf("[GPT] PartitionEntryLba=%llu\n",
        (unsigned long long)h->PartitionEntryLba);

        
    for (uint32_t i = 0; i < Out->EntryCount; i++) {
        GPT_ENTRY *e = (GPT_ENTRY *)(table + i * Out->EntrySize);
        if (guid_is_zero(e->TypeGuid)) continue;
        if (e->FirstLba == 0 || e->LastLba == 0) continue;

        if (Out->PartitionCount >= GPT_MAX_PARTITIONS) break;
        Out->Partitions[Out->PartitionCount++] = *e;
    }

    KdPrintf("[GPT] %d non-empty partitions\n", Out->PartitionCount);

    for (int i = 0; i < Out->PartitionCount; i++) {
        GPT_ENTRY *e = &Out->Partitions[i];
        char guid_str[40];
        char name_str[40];
        GptFormatGuid(e->TypeGuid, guid_str);
        GptFormatName(e->NameUtf16, name_str, sizeof(name_str));

        const char *kind = "other";
        if (guid_equal(e->TypeGuid, GUID_TYPE_EFI_SYSTEM))      kind = "EFI System";
        else if (guid_equal(e->TypeGuid, GUID_TYPE_MS_BASIC_DATA)) kind = "MS Basic Data";

        KdPrintf("[GPT]   [%d] %s  LBA %llu..%llu  name='%s'\n",
                 i, kind,
                 (unsigned long long)e->FirstLba,
                 (unsigned long long)e->LastLba,
                 name_str);
    }

    MmFreePages(table, pages_needed);
    return 0;
}

int GptFindPartition(GPT_INFO *Info, const uint8_t *TypeGuid, GPT_ENTRY **Out)
{
    for (int i = 0; i < Info->PartitionCount; i++) {
        if (guid_equal(Info->Partitions[i].TypeGuid, TypeGuid)) {
            *Out = &Info->Partitions[i];
            return 0;
        }
    }
    return -1;
}

void GptFormatGuid(const uint8_t *G, char *Out)
{
    const char hex[] = "0123456789ABCDEF";
    int p = 0;
    int le_map[4] = {3, 2, 1, 0};
    for (int i = 0; i < 4; i++) Out[p++] = hex[(G[le_map[i]] >> 4) & 0xF], Out[p++] = hex[G[le_map[i]] & 0xF];
    Out[p++] = '-';
    for (int i = 0; i < 2; i++) Out[p++] = hex[(G[5 - i] >> 4) & 0xF], Out[p++] = hex[G[5 - i] & 0xF];
    Out[p++] = '-';
    for (int i = 0; i < 2; i++) Out[p++] = hex[(G[7 - i] >> 4) & 0xF], Out[p++] = hex[G[7 - i] & 0xF];
    Out[p++] = '-';
    for (int i = 8; i < 10; i++) Out[p++] = hex[(G[i] >> 4) & 0xF], Out[p++] = hex[G[i] & 0xF];
    Out[p++] = '-';
    for (int i = 10; i < 16; i++) Out[p++] = hex[(G[i] >> 4) & 0xF], Out[p++] = hex[G[i] & 0xF];
    Out[p] = 0;
}

void GptFormatName(const uint16_t *N, char *Out, int OutSize)
{
    int i = 0;
    while (i < OutSize - 1 && N[i] && i < 36) {
        uint16_t c = N[i];
        Out[i] = (c < 128) ? (char)c : '?';
        i++;
    }
    Out[i] = 0;
}