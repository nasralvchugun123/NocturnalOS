
#ifndef HAL_APIC_H
#define HAL_APIC_H

#include <stdint.h>

#define LAPIC_ID            0x020
#define LAPIC_TPR           0x080
#define LAPIC_EOI           0x0B0
#define LAPIC_SVR           0x0F0
#define LAPIC_LVT_TIMER     0x320
#define LAPIC_LVT_LINT0     0x350
#define LAPIC_LVT_LINT1     0x360
#define LAPIC_LVT_ERROR     0x370
#define LAPIC_TIMER_INIT    0x380
#define LAPIC_TIMER_CUR     0x390
#define LAPIC_TIMER_DIV     0x3E0

#define LAPIC_SVR_ENABLE    (1u << 8)

#define IOAPIC_IOREGSEL     0x00
#define IOAPIC_IOWIN        0x10

void     HalpLapicInit(void);
void     HalpLapicSendEoi(void);
uint32_t HalpLapicGetId(void);

void     HalpIoapicInit(void);
void     HalpIoapicSetEntry(uint8_t Gsi, uint8_t Vector, uint8_t LapicId, int Masked);
uint32_t HalpIoapicGetVersion(void);

void     HalpPicDisable(void);

#define LAPIC_ICR_LOW        0x300
#define LAPIC_ICR_HIGH       0x310

#define LAPIC_ICR_FIXED      (0u << 8)
#define LAPIC_ICR_INIT       (5u << 8)
#define LAPIC_ICR_SIPI       (6u << 8)

#define LAPIC_ICR_DEST_PHYS  (0u << 11)
#define LAPIC_ICR_ASSERT     (1u << 14)
#define LAPIC_ICR_LEVEL      (1u << 15)

void HalpLapicInitAp(void);
void HalpSendIpi(uint32_t TargetApicId, uint32_t IcrLow);

void HalpInitOProcessors(void);

#endif