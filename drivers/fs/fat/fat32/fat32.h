
#ifndef FAT32_H
#define FAT32_H

#include <stdint.h>

typedef int (*fat_read_fn)(uint64_t Lba, uint32_t Count, void *Buf);

typedef struct {
    uint8_t  Name[11];
    uint8_t  Attr;
    uint8_t  NtRes;
    uint8_t  CrtTimeTenth;
    uint16_t CrtTime;
    uint16_t CrtDate;
    uint16_t LastAccDate;
    uint16_t FirstClusterHi;
    uint16_t WrtTime;
    uint16_t WrtDate;
    uint16_t FirstClusterLo;
    uint32_t FileSize;
} __attribute__((packed)) FAT32_DIRENT;

#define FAT_ATTR_READ_ONLY  0x01
#define FAT_ATTR_HIDDEN     0x02
#define FAT_ATTR_SYSTEM     0x04
#define FAT_ATTR_VOLUME_ID  0x08
#define FAT_ATTR_DIRECTORY  0x10
#define FAT_ATTR_ARCHIVE    0x20
#define FAT_ATTR_LFN        0x0F

typedef struct {
    fat_read_fn Read;
    uint64_t    PartLba;
    uint16_t    BytesPerSector;
    uint8_t     SectorsPerCluster;
    uint16_t    ReservedSectors;
    uint8_t     NumFats;
    uint32_t    FatSizeSectors;
    uint32_t    RootCluster;
    uint32_t    FatStartLba;
    uint32_t    DataStartLba;
    uint32_t    ClusterSize;
    uint32_t    TotalClusters;
} FAT32_FS;

int Fat32Mount(fat_read_fn Read, uint64_t PartLba, FAT32_FS *Out);

int Fat32ReadCluster(FAT32_FS *Fs, uint32_t Cluster, void *Buf);
uint32_t Fat32NextCluster(FAT32_FS *Fs, uint32_t Cluster);

int Fat32ListDir(FAT32_FS *Fs, uint32_t DirCluster);
int Fat32ListRoot(FAT32_FS *Fs);

int Fat32FindInDir(FAT32_FS *Fs, uint32_t DirCluster,
const char *Name, FAT32_DIRENT *Out);
int Fat32FindInRoot(FAT32_FS *Fs, const char *Name, FAT32_DIRENT *Out);
int Fat32ReadFile(FAT32_FS *Fs, FAT32_DIRENT *Entry, void *Buf, uint32_t BufSize);
int Fat32ReadAt(FAT32_FS *Fs, uint32_t FirstCluster, uint32_t FileSize,
 uint32_t Offset, uint8_t *Buf, uint32_t Len);
 typedef int (*FAT32_DIR_CB)(const char *Name, uint32_t Attr,
                            uint32_t Size, void *Ctx);

int Fat32IterDir(FAT32_FS *Fs, uint32_t DirCluster,
                 FAT32_DIR_CB Cb, void *Ctx);

int Fat32ResolvePath(FAT32_FS *Fs, const char *Path, uint32_t *OutCluster);

#endif