#ifndef _RTL_FORMAT_H
#define _RTL_FORMAT_H

#include <nt/ntdefs.h>

int RtlVsprintf(char *buf, SIZE_T n, const char *fmt, __builtin_va_list ap);
int RtlSprintf(char *buf, SIZE_T n, const char *fmt, ...);

int vsnprintf(char *buf, SIZE_T n, const char *fmt, __builtin_va_list ap);
int snprintf(char *buf, SIZE_T n, const char *fmt, ...);

#endif