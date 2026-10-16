#include <stdio.h>
#include <nt/ntdll.h>

int puts(const char *s)
{
    NtWriteConsole(s);
    NtWriteConsole("\n");
    return 0;
}