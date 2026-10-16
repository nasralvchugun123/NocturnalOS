
#include "freez.h"

void KeFreezeCPU(void)
{
    while (1) {
        __asm__ volatile("hlt");
    }
}
