#include <stdlib.h>
#include <nt/ntdll.h>

void exit(int status)
{
    NtTerminateProcess(0, (LONG)status);
    for (;;) __asm__ volatile ("hlt");
}