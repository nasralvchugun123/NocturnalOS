
#ifndef HAL_IRQ_H
#define HAL_IRQ_H

#include <stdint.h>

void HalEnableSystemInterrupt(uint8_t Irq);
void HalDisableSystemInterrupt(uint8_t Irq);
void HalEndOfInterrupt(uint8_t Irq);

#endif