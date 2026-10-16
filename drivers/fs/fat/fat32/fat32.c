#include <dbg/print/kdprint.h>
#include <ds/fb/fb.h>
#include "fat32.h"
#include <rt.h>

#define FAT32_MAX_CLUSTER   4096
#define FAT32_EOC_MIN       0x0FFFFFF8
#define FAT32_BAD           0x0FFFFFF7
#define FAT32_EOC           0x0FFFFFFF

static uint8_t  gSectorBuf[512]  __attribute__((aligned(4096)));
static uint8_t  gClusterBuf[FAT32_MAX_CLUSTER] __attribute__((aligned(4096)));

static uint16_t rd16(const uint8_t *p) { return (uint16_t)p[0] | ((uint16_t)p[1] << 8); }
static uint32_t rd32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8)
         | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void format_83(const uint8_t *Name, char *Out)
{
    int p = 0;
    for (int i = 0; i < 8; i++) {
        if (Name[i] == ' ') break;
        Out[p++] = (char)Name[i];
    }
    if (Name[8] != ' ') {
        Out[p++] = '.';
        for (int i = 8; i < 11; i++) {
            if (Name[i] == ' ') break;
            Out[p++] = (char)Name[i];
        }
    }
    Out[p] = 0;
}

static void make_83(const char *In, uint8_t *Out)
{
    for (int i = 0; i < 11; i++) Out[i] = ' ';
    int p = 0;
    int i = 0;
    while (In[i] && In[i] != '.' && p < 8) {
        char c = In[i];
        if (c >= 'a' && c <= 'z') c = (char)(c - 32);
        Out[p++] = (uint8_t)c;
        i++;
    }
    if (In[i] == '.') {
        i++;
        p = 8;
        while (In[i] && p < 11) {
            char c = In[i];
            if (c >= 'a' && c <= 'z') c = (char)(c - 32);
            Out[p++] = (uint8_t)c;
            i++;
        }
    }
}

int Fat32Mount(fat_read_fn Read, uint64_t PartLba, FAT32_FS *Out)
{
    for (uint32_t i = 0; i < sizeof(FAT32_FS); i++) ((uint8_t *)Out)[i] = 0;

    if (Read(PartLba, 1, gSectorBuf) != 0) {
        KdPrint("[FAT32] read boot sector failed\n");
        return -1;
    }

    if (gSectorBuf[510] != 0x55 || gSectorBuf[511] != 0xAA) {
        KdPrintf("[FAT32] bad boot sig: %02x %02x\n",
                 gSectorBuf[510], gSectorBuf[511]);
        return -2;
    }

    uint16_t bps        = rd16(gSectorBuf + 0x0B);
    uint8_t  spc        = gSectorBuf[0x0D];
    uint16_t rsvd       = rd16(gSectorBuf + 0x0E);
    uint8_t  nfats      = gSectorBuf[0x10];
    uint16_t rootent    = rd16(gSectorBuf + 0x11);
    uint16_t fatsz16    = rd16(gSectorBuf + 0x16);
    uint32_t fatsz32    = rd32(gSectorBuf + 0x24);
    uint32_t rootclu    = rd32(gSectorBuf + 0x2C);
    uint32_t totsec32   = rd32(gSectorBuf + 0x20);

    if (bps != 512) {
        KdPrintf("[FAT32] unsupported BytesPerSector=%u\n", bps);
        return -3;
    }
    if (spc == 0 || rsvd == 0 || nfats == 0) {
        KdPrint("[FAT32] invalid BPB (spc/rsvd/nfats)\n");
        return -4;
    }
    if (rootent != 0 || fatsz16 != 0) {
        KdPrint("[FAT32] not FAT32 (FAT12/16 signature)\n");
        return -5;
    }
    if (fatsz32 == 0) {
        KdPrint("[FAT32] FatSize32 == 0\n");
        return -6;
    }

    uint32_t cluster_size = (uint32_t)bps * spc;
    if (cluster_size > FAT32_MAX_CLUSTER) {
        KdPrintf("[FAT32] cluster size %u > %u, unsupported\n",
                 cluster_size, FAT32_MAX_CLUSTER);
        return -7;
    }

    Out->Read              = Read;
    Out->PartLba           = PartLba;
    Out->BytesPerSector    = bps;
    Out->SectorsPerCluster = spc;
    Out->ReservedSectors   = rsvd;
    Out->NumFats           = nfats;
    Out->FatSizeSectors    = fatsz32;
    Out->RootCluster       = rootclu;
    Out->FatStartLba       = (uint32_t)PartLba + rsvd;
    Out->DataStartLba      = Out->FatStartLba + (uint32_t)nfats * fatsz32;
    Out->ClusterSize       = cluster_size;
    Out->TotalClusters     = (totsec32 - (rsvd + (uint32_t)nfats * fatsz32)) / spc;

    KdPrintf("[FAT32] mounted: bps=%u spc=%u rsvd=%u nfats=%u fat32sz=%u rootclu=%u\n",
             bps, spc, rsvd, nfats, fatsz32, rootclu);
    KdPrintf("[FAT32] fat_start=%u data_start=%u cluster_size=%u clusters=%u\n",
             Out->FatStartLba, Out->DataStartLba,
             Out->ClusterSize, Out->TotalClusters);

    return 0;
}

int Fat32ReadCluster(FAT32_FS *Fs, uint32_t Cluster, void *Buf)
{
    if (Cluster < 2) {
        KdPrintf("[FAT32] read cluster: invalid cluster %u\n", Cluster);
        return -1;
    }
    uint64_t lba = (uint64_t)Fs->DataStartLba
                 + (uint64_t)(Cluster - 2) * Fs->SectorsPerCluster;
    uint8_t *p = (uint8_t *)Buf;
    for (uint8_t s = 0; s < Fs->SectorsPerCluster; s++) {
        if (Fs->Read(lba + s, 1, p + s * 512) != 0) {
            KdPrintf("[FAT32] read cluster %u sector %u failed\n", Cluster, s);
            return -2;
        }
    }
    return 0;
}

uint32_t Fat32NextCluster(FAT32_FS *Fs, uint32_t Cluster)
{
    uint32_t fat_offset = Cluster * 4;
    uint32_t fat_sector = Fs->FatStartLba + (fat_offset / Fs->BytesPerSector);
    uint32_t entry_off  = fat_offset % Fs->BytesPerSector;

    if (Fs->Read(fat_sector, 1, gSectorBuf) != 0) {
        KdPrintf("[FAT32] read FAT sector %u failed\n", fat_sector);
        return 0;
    }
    uint32_t val = rd32(gSectorBuf + entry_off) & 0x0FFFFFFF;

    if (val >= FAT32_EOC_MIN) return FAT32_EOC;
    if (val == FAT32_BAD)     return 0;
    if (val < 2)              return 0;
    return val;
}

int Fat32ListDir(FAT32_FS *Fs, uint32_t DirCluster)
{
    uint32_t cluster = DirCluster;
    int total = 0;

    KdPrintf("[FAT32] listing cluster %u:\n", DirCluster);

    while (cluster >= 2 && cluster < FAT32_EOC_MIN) {
        if (Fat32ReadCluster(Fs, cluster, gClusterBuf) != 0) {
            KdPrintf("[FAT32] read cluster %u failed\n", cluster);
            return -1;
        }
        uint32_t entries = Fs->ClusterSize / 32;

        for (uint32_t i = 0; i < entries; i++) {
            FAT32_DIRENT *e = (FAT32_DIRENT *)(gClusterBuf + i * 32);

            if (e->Name[0] == 0x00) {
                KdPrintf("[FAT32] %d entries\n", total);
                return 0;
            }
            if (e->Name[0] == '.' && (e->Name[1] == ' ' ||
                          (e->Name[1] == '.' && e->Name[2] == ' ')))
            continue;

            if (e->Name[0] == 0xE5) continue;
            if (e->Attr == FAT_ATTR_LFN) continue;
            if (e->Attr & FAT_ATTR_VOLUME_ID) continue;

            char name[16];
            format_83(e->Name, name);
            uint32_t first = ((uint32_t)e->FirstClusterHi << 16) | e->FirstClusterLo;
            const char *kind = (e->Attr & FAT_ATTR_DIRECTORY) ? "DIR" : "FILE";

            KdPrintf("[FAT32]   %s  %s  size=%u  clu=%u\n",
                     name, kind, e->FileSize, first);
            total++;
        }
        cluster = Fat32NextCluster(Fs, cluster);
    }
    KdPrintf("[FAT32] %d entries\n", total);
    return 0;
}

int Fat32ListRoot(FAT32_FS *Fs)
{
    return Fat32ListDir(Fs, Fs->RootCluster);
}


#define LFN_MAX_ENTRIES  20
#define LFN_MAX_CHARS    (LFN_MAX_ENTRIES * 13)   /* 260 as MAX_PATH */

static const int gLfnCharOffsets[13] = {
    1, 3, 5, 7, 9,         
    14, 16, 18, 20, 22, 24, 
    28, 30                 
};

static void lfn_collect(const uint8_t *raw, uint16_t *out, int pos)
{
    for (int k = 0; k < 13; k++) {
        int o = gLfnCharOffsets[k];
        uint16_t ch = (uint16_t)raw[o] | ((uint16_t)raw[o + 1] << 8);
        out[pos + k] = ch;
    }
}

static int lfn_matches_ascii(const uint16_t *lfn, const char *ascii)
{
    int i = 0;
    for (; lfn[i] && ascii[i]; i++) {
        uint16_t w = lfn[i];
        if (w > 0x7F) return 0;               
        char wc = (char)w;
        char ac = ascii[i];
        if (wc >= 'a' && wc <= 'z') wc -= 32;
        if (ac >= 'a' && ac <= 'z') ac -= 32;
        if (wc != ac) return 0;
    }
    return lfn[i] == 0 && ascii[i] == 0;
}

int Fat32FindInDir(FAT32_FS *Fs, uint32_t DirCluster,
                   const char *Name, FAT32_DIRENT *Out)
{
    if (!Fs || !Name || !Out) return -1;

    uint8_t want[11];
    make_83(Name, want);

    uint32_t cluster = DirCluster;
    uint32_t guard   = 0;

    uint16_t lfn_buf[LFN_MAX_CHARS + 1];
    int      lfn_ok  = 0;     
    int      lfn_end = 0;   

    while (cluster >= 2 && cluster < FAT32_EOC_MIN) {
        if (Fat32ReadCluster(Fs, cluster, gClusterBuf) != 0) return -1;
        uint32_t entries = Fs->ClusterSize / 32;

        for (uint32_t i = 0; i < entries; i++) {
            FAT32_DIRENT *e   = (FAT32_DIRENT *)(gClusterBuf + i * 32);
            uint8_t      *raw = (uint8_t *)e;

            if (e->Name[0] == 0x00) return -1;
            if (e->Name[0] == 0xE5) { lfn_ok = 0; lfn_end = 0; continue; }

            if (e->Attr == FAT_ATTR_LFN) {
                uint8_t seq = e->Name[0];
                int     num = seq & 0x3F;
                if (num < 1 || num > LFN_MAX_ENTRIES) {
                    lfn_ok = 0; lfn_end = 0; continue;
                }
                int pos = (num - 1) * 13;
                lfn_collect(raw, lfn_buf, pos);

                if (seq & 0x40) lfn_end = pos + 13;
                lfn_ok = 1;
                continue;
            }
            if (e->Attr & FAT_ATTR_VOLUME_ID) {
                lfn_ok = 0; lfn_end = 0; continue;
            }

            int match = 0;

            if (lfn_ok && lfn_end > 0) {
                lfn_buf[lfn_end] = 0;
                if (lfn_matches_ascii(lfn_buf, Name)) match = 1;
            }

            if (!match) {
                int eq = 1;
                for (int j = 0; j < 11; j++) {
                    if (e->Name[j] != want[j]) { eq = 0; break; }
                }
                if (eq) match = 1;
            }

            lfn_ok  = 0;
            lfn_end = 0;

            if (match) {
                *Out = *e;
                return 0;
            }
        }

        cluster = Fat32NextCluster(Fs, cluster);
            lfn_ok  = 0;
            lfn_end = 0;
        if (++guard > 100000) {
                 KdPrint("[FAT32] FindInDir loop guard\n");
        return -1;
}
    }
    return -1;
}

int Fat32FindInRoot(FAT32_FS *Fs, const char *Name, FAT32_DIRENT *Out)
{
    return Fat32FindInDir(Fs, Fs->RootCluster, Name, Out);
}

int Fat32ReadAt(FAT32_FS *Fs, uint32_t FirstCluster, uint32_t FileSize,
                uint32_t Offset, uint8_t *Buf, uint32_t Len)
{
    if (Offset >= FileSize) return 0;
    if (Offset + Len > FileSize) Len = FileSize - Offset;

    uint32_t cluster    = FirstCluster;
    uint32_t skip       = Offset / Fs->ClusterSize;
    uint32_t in_cluster = Offset % Fs->ClusterSize;

    for (uint32_t i = 0; i < skip; i++) {
        cluster = Fat32NextCluster(Fs, cluster);
        if (cluster == 0 || cluster == FAT32_EOC) {
            KdPrintf("[FAT32] ReadAt: chain ended at skip %u/%u\n", i, skip);
            return -1;
        }
    }

    uint32_t written = 0;
    uint32_t guard   = 0;

    while (written < Len) {
        if (Fat32ReadCluster(Fs, cluster, gClusterBuf) != 0) return -1;

        uint32_t avail = Fs->ClusterSize - in_cluster;
        uint32_t chunk = (Len - written < avail) ? (Len - written) : avail;

        for (uint32_t i = 0; i < chunk; i++)
            Buf[written + i] = gClusterBuf[in_cluster + i];

        written   += chunk;
        in_cluster = 0;

        if (written < Len) {
            cluster = Fat32NextCluster(Fs, cluster);
            if (cluster == 0 || cluster == FAT32_EOC) break;
        }
        if (++guard > 100000) {
            KdPrint("[FAT32] ReadAt loop guard\n");
            return -1;
        }
    }
    return (int)written;
}

int Fat32ReadFile(FAT32_FS *Fs, FAT32_DIRENT *Entry, void *Buf, uint32_t BufSize)
{
    if (Entry->Attr & FAT_ATTR_DIRECTORY) {
        KdPrint("[FAT32] read file: it's a directory\n");
        return -1;
    }
    if (Entry->FileSize == 0) return 0;
    if (Entry->FileSize > BufSize) {
        KdPrintf("[FAT32] buffer too small: need %u, have %u\n",
                 Entry->FileSize, BufSize);
        return -2;
    }

    uint32_t first = ((uint32_t)Entry->FirstClusterHi << 16) | Entry->FirstClusterLo;
    if (first < 2) {
        KdPrint("[FAT32] file has no cluster\n");
        return -3;
    }

    return Fat32ReadAt(Fs, first, Entry->FileSize, 0, (uint8_t *)Buf, Entry->FileSize);
}

int Fat32IterDir(FAT32_FS *Fs, uint32_t DirCluster,
                 FAT32_DIR_CB Cb, void *Ctx)
{
    uint32_t cluster = DirCluster;
    uint32_t guard = 0;

    while (cluster >= 2 && cluster < FAT32_EOC_MIN) {
        if (Fat32ReadCluster(Fs, cluster, gClusterBuf) != 0) return -1;
        uint32_t entries = Fs->ClusterSize / 32;

        for (uint32_t i = 0; i < entries; i++) {
            FAT32_DIRENT *e = (FAT32_DIRENT *)(gClusterBuf + i * 32);

            if (e->Name[0] == 0x00) return 0;   
            if (e->Name[0] == 0xE5) continue;   
            if (e->Attr == FAT_ATTR_LFN) continue;
            if (e->Attr & FAT_ATTR_VOLUME_ID) continue;

            char name[16];
            format_83(e->Name, name);
            uint32_t first = ((uint32_t)e->FirstClusterHi << 16) | e->FirstClusterLo;
            uint32_t attr  = (e->Attr & FAT_ATTR_DIRECTORY) ? 1 : 0;

            if (Cb(name, attr, e->FileSize, Ctx) != 0)
                return 0;   
        }
        cluster = Fat32NextCluster(Fs, cluster);
        if (++guard > 100000) return -1;
    }
    return 0;
}

int Fat32ResolvePath(FAT32_FS *Fs, const char *Path, uint32_t *OutCluster)
{
    if (!Path || !OutCluster) return -1;

    uint32_t cur = Fs->RootCluster;

    while (*Path == '\\') Path++;

    if (*Path == 0) { *OutCluster = cur; return 0; }

    char comp[32];
    while (*Path) {
        int i = 0;
        while (*Path && *Path != '\\' && i < 31) comp[i++] = *Path++;
        comp[i] = 0;
        while (*Path == '\\') Path++;

        FAT32_DIRENT e;
        if (Fat32FindInDir(Fs, cur, comp, &e) != 0) return -2;
        if (!(e.Attr & FAT_ATTR_DIRECTORY)) return -3;

        cur = ((uint32_t)e.FirstClusterHi << 16) | e.FirstClusterLo;
        if (cur < 2) return -4;
    }

    *OutCluster = cur;
    return 0;
}
