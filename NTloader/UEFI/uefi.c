
#include "uefi.h"

void __stack_chk_fail(void) {
    for (;;) __asm__ volatile ("hlt");
}