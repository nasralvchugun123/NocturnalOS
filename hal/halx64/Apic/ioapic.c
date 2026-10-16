#include <dbg/print/kdprint.h>
#include "Apic.h"
#include <rt.h>

#define IOAPIC_PHYS 0xFEC00000

static volatile uint32_t *gIoapic = 0;

static inline void ioapic_write_reg(uint32_t reg, uint32_t val)
{
    gIoapic[IOAPIC_IOREGSEL / 4] = reg;
    gIoapic[IOAPIC_IOWIN   / 4] = val;
}

static inline uint32_t ioapic_read_reg(uint32_t reg)
{
    gIoapic[IOAPIC_IOREGSEL / 4] = reg;
    return gIoapic[IOAPIC_IOWIN / 4];
}

void HalpIoapicInit(void)
{
    KdPrint("[Hal] IOAPIC init\n");

    gIoapic = (volatile uint32_t *)(uintptr_t)IOAPIC_PHYS;

    uint32_t ver = ioapic_read_reg(0x01);
    uint32_t max_redir = (ver >> 16) & 0xFF;

    KdPrintf("[Hal] IOAPIC ver=0x%x, %u redirection entries\n",
             ver & 0xFF, max_redir + 1);

    for (uint32_t i = 0; i <= max_redir; i++) {
        ioapic_write_reg(0x10 + 2 * i, 1u << 16);   /* masked */
        ioapic_write_reg(0x10 + 2 * i + 1, 0);
    }
}

uint32_t HalpIoapicGetVersion(void)
{
    if (!gIoapic) return 0;
    return ioapic_read_reg(0x01);
}

void HalpIoapicSetEntry(uint8_t Gsi, uint8_t Vector, uint8_t LapicId, int Masked)
{
    if (!gIoapic) return;

    uint32_t low = (uint32_t)Vector;
    if (Masked) low |= (1u << 16);

    uint32_t high = ((uint32_t)LapicId) << 24;

    ioapic_write_reg(0x10 + 2 * Gsi,     low);
    ioapic_write_reg(0x10 + 2 * Gsi + 1, high);
}