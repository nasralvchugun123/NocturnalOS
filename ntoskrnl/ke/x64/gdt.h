#ifndef KE_GDT_H
#define KE_GDT_H

#include <stdint.h>

typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  flags_limit_hi;
    uint8_t  base_hi;
} __attribute__((packed)) KGDT_ENTRY;

typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  flags_limit_hi;
    uint8_t  base_hi;
    uint32_t base_upper;
    uint32_t reserved;
} __attribute__((packed)) KGDT_TSS_ENTRY;

typedef struct {
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist[7];
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap_base;
} __attribute__((packed)) KTSS;

typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) KGDTR;

#define KGDT_ENTRIES 7

struct _KCPU;
void KiBuildGdtForCpu(struct _KCPU *Cpu);
void KiInitializeGdt(void);
void KiInitializeTss(void);
void KiSetTssRsp0(uint64_t Rsp0);

#endif