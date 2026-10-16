#include <stdio.h>
#include <nt/ntdll.h>

extern int vsnprintf(char *buf, size_t n, const char *fmt, __builtin_va_list ap);

int printf(const char *fmt, ...)
{
    char buf[1024];
    __builtin_va_list ap;
    __builtin_va_start(ap, fmt);
    int r = vsnprintf(buf, sizeof(buf), fmt, ap);
    __builtin_va_end(ap);

    NtWriteConsole(buf);
    return r;
}

int sprintf(char *buf, const char *fmt, ...)
{
    __builtin_va_list ap;
    __builtin_va_start(ap, fmt);
    int r = vsnprintf(buf, (size_t)-1, fmt, ap);
    __builtin_va_end(ap);
    return r;
}

int snprintf(char *buf, size_t n, const char *fmt, ...)
{
    __builtin_va_list ap;
    __builtin_va_start(ap, fmt);
    int r = vsnprintf(buf, n, fmt, ap);
    __builtin_va_end(ap);
    return r;
}