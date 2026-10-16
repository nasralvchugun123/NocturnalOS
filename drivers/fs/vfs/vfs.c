#include "vfs.h"
#include <rt.h>
#include <dbg/print/kdprint.h>
#include <ds/fb/fb.h>

static FAT32_FS gVfsFs;
static int      gVfsMounted = 0;

int VfsMountFAT32(fat_read_fn Read, uint64_t PartLba)
{
    int rc = Fat32Mount(Read, PartLba, &gVfsFs);
    if (rc != 0) {
        KdPrintf("[VFS] mount failed rc=%d\n", rc);
        return rc;
    }
    gVfsMounted = 1;
    KdPrint("[VFS] mounted\n");
    return 0;
}

FAT32_FS *VfsGetFs(void)
{
    return gVfsMounted ? &gVfsFs : 0;
}

int VfsIsMounted(void)
{
    return gVfsMounted;
}

static int resolve_path(const char *Path, FAT32_DIRENT *Out)
{
    if (!gVfsMounted) {
        KdPrint("[VFS] resolve: not mounted\n");
        return -1;
    }
    if (!Path || Path[0] != '\\') {
        KdPrintf("[VFS] path must start with backslash: '%s'\n", Path);
        return -2;
    }

    const char *p = Path + 1;

    if (*p == 0) {
        for (int i = 0; i < 11; i++) Out->Name[i] = ' ';
        Out->Name[0]        = ' ';
        Out->Attr           = FAT_ATTR_DIRECTORY;
        Out->FirstClusterHi = (uint16_t)(gVfsFs.RootCluster >> 16);
        Out->FirstClusterLo = (uint16_t)(gVfsFs.RootCluster & 0xFFFF);
        Out->FileSize       = 0;
        return 0;
    }

    uint32_t current_cluster = gVfsFs.RootCluster;
    char     segment[16];

    while (1) {
        int n = 0;
        while (p[n] && p[n] != '\\' && n < 15) {
            segment[n] = p[n];
            n++;
        }
        segment[n] = 0;

        if (n == 0) {
            KdPrintf("[VFS] empty segment in path '%s'\n", Path);
            return -3;
        }

        FAT32_DIRENT entry;
        if (Fat32FindInDir(&gVfsFs, current_cluster, segment, &entry) != 0) {
            KdPrintf("[VFS] not found: '%s' (in cluster %u)\n",
                     segment, current_cluster);
            return -4;
        }

        if (p[n] == 0) {
            *Out = entry;
            return 0;
        }

        if (!(entry.Attr & FAT_ATTR_DIRECTORY)) {
            KdPrintf("[VFS] '%s' is not a directory\n", segment);
            return -5;
        }
        current_cluster = ((uint32_t)entry.FirstClusterHi << 16) | entry.FirstClusterLo;
        p += n + 1;
    }
}

int VfsOpen(const char *Path, VFS_FILE *Out)
{
    if (!Out) return -1;
    Out->Used = 0;

    FAT32_DIRENT entry;
    int rc = resolve_path(Path, &entry);
    if (rc != 0) return rc;

    if (entry.Attr & FAT_ATTR_DIRECTORY) {
        KdPrintf("[VFS] VfsOpen: '%s' is a directory\n", Path);
        return -6;
    }

    Out->Used         = 1;
    Out->Fs           = &gVfsFs;
    Out->FileSize     = entry.FileSize;
    Out->Offset       = 0;
    Out->FirstCluster = ((uint32_t)entry.FirstClusterHi << 16) | entry.FirstClusterLo;

    KdPrintf("[VFS] opened '%s': size=%u clu=%u\n",
             Path, Out->FileSize, Out->FirstCluster);
    return 0;
}

int VfsListDir(const char *Path, FAT32_DIR_CB Cb, void *Ctx)
{
    uint32_t clu = 0;
    int rc = Fat32ResolvePath(&gVfsFs, Path, &clu);
    if (rc == 0) rc = Fat32IterDir(&gVfsFs, clu, Cb, Ctx);
    return rc;
}

int VfsRead(VFS_FILE *File, void *Buf, uint32_t Len)
{
    if (!File || !File->Used) return -1;
    if (File->Offset >= File->FileSize) return 0;

    int n = Fat32ReadAt(File->Fs, File->FirstCluster, File->FileSize,
                        File->Offset, (uint8_t *)Buf, Len);
    if (n > 0) File->Offset += (uint32_t)n;
    return n;
}

int VfsClose(VFS_FILE *File)
{
    if (!File) return -1;
    File->Used = 0;
    return 0;
}