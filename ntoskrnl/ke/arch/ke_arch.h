
#ifndef KE_ARCH_H
#define KE_ARCH_H

#include <stdint.h>

#define KGDT_NULL       0x00
#define KGDT_KCODE64    0x08
#define KGDT_KDATA64    0x10
#define KGDT_UDATA64    0x18
#define KGDT_UCODE64    0x20
#define KGDT_TSS        0x28

void KiInitializeGdt(void);
void KiInitializeTss(void);
void KiInitializeIdt(void);
void KiEnterUserMode(uint64_t EntryPoint, uint64_t UserStackTop);


#endif