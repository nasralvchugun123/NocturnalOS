#ifndef _STDIO_H
#define _STDIO_H

#include <stddef.h>

int  printf(const char *fmt, ...);
int  sprintf(char *buf, const char *fmt, ...);
int  snprintf(char *buf, size_t n, const char *fmt, ...);
int  vsnprintf(char *buf, size_t n, const char *fmt, __builtin_va_list ap);

int  puts(const char *s);
int  putchar(int c);
int  fputs(const char *s, void *stream);
int  fputc(int c, void *stream);

#define stdout ((void*)1)
#define stderr ((void*)2)
#define stdin  ((void*)0)

#endif