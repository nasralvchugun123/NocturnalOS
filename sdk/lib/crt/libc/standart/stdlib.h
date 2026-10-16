#ifndef _STDLIB_H
#define _STDLIB_H

#include <stddef.h>

void  *malloc(size_t n);
void   free(void *p);
void  *realloc(void *p, size_t n);
void  *calloc(size_t n, size_t sz);

void   exit(int status) __attribute__((noreturn));
void   abort(void) __attribute__((noreturn));

int    atoi(const char *s);
long   atol(const char *s);

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

#endif