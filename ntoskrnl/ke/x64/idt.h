
#ifndef KE_IDT_H
#define KE_IDT_H

#include <stdint.h>

void KiInitializeIdt(void);
void KiReloadIdt(void);

#endif