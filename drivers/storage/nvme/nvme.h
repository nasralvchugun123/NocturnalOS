
#ifndef NVME_H
#define NVME_H

#include <stdint.h>

typedef struct {
    uint32_t PciBus, PciDev, PciFn;
    uint64_t Bar0Phys;
    uint32_t DoorbellStride;
    uint32_t MaxQueueEntries;

    void    *AdminSqVirt;
    void    *AdminCqVirt;
    uint64_t AdminSqPhys;
    uint64_t AdminCqPhys;
    uint16_t AdminSqTail;
    uint16_t AdminCqHead;
    uint8_t  AdminCqPhase;

    void    *IoSqVirt;
    void    *IoCqVirt;
    uint64_t IoSqPhys;
    uint64_t IoCqPhys;
    uint16_t IoSqTail;
    uint16_t IoCqHead;
    uint8_t  IoCqPhase;

    uint32_t NsCount;
    uint64_t NsSizeLba;      
    uint32_t LbaSizeBytes;   
    uint8_t  Nsid[4];       
} NVME_DEVICE;

int  NvmeInit(void);
int  NvmeReadSectors(uint64_t Lba, uint32_t Count, void *Buf);
int  NvmeWriteSectors(uint64_t Lba, uint32_t Count, const void *Buf);
void NvmeGetInfo(NVME_DEVICE *Out);

#endif