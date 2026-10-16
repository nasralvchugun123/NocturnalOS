#include <stdio.h>
#include <nt/ntdll.h>

int putchar(int c)
{
    char tmp[2];
    tmp[0] = (char)c;
    tmp[1] = 0;
    NtWriteConsole(tmp);
    return (unsigned char)c;
}