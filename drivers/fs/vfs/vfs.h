
#ifndef VFS_H
#define VFS_H

#include <stdint.h>
#include "../fat/fat32/fat32.h"


typedef struct {
    int       Used;
    FAT32_FS *Fs;
    uint32_t  FirstCluster;
    uint32_t  FileSize;
    uint32_t  Offset;
} VFS_FILE;

int  VfsMountFAT32(fat_read_fn Read, uint64_t PartLba);
FAT32_FS *VfsGetFs(void);
static FAT32_FS gVfsFs;

int VfsIsMounted(void);
int  VfsOpen(const char *Path, VFS_FILE *Out);
int  VfsRead(VFS_FILE *File, void *Buf, uint32_t Len);
int  VfsClose(VFS_FILE *File);
int  VfsListDir(const char *Path, FAT32_DIR_CB Cb, void *Ctx);

#endif
