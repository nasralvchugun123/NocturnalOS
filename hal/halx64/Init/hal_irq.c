
#include "hal_irq.h"
#include "../Apic/Apic.h"

extern uint32_t HalpLapicGetId(void);

#define GSI_KEYBOARD  1

void HalEnableSystemInterrupt(uint8_t Irq)
{
    uint8_t vec = (uint8_t)(0x20 + Irq);
    HalpIoapicSetEntry(Irq, vec, (uint8_t)HalpLapicGetId(), 0 /* unmasked */);
}

void HalDisableSystemInterrupt(uint8_t Irq)
{
    uint8_t vec = (uint8_t)(0x20 + Irq);
    HalpIoapicSetEntry(Irq, vec, (uint8_t)HalpLapicGetId(), 1 /* masked */);
}

void HalEndOfInterrupt(uint8_t Irq)
{
    (void)Irq;
    HalpLapicSendEoi();
}