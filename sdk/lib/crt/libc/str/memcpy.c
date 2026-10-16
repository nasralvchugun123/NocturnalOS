#include <string.h>
#include <RTL/rt.h>
void *memcpy(void *d, const void *s, size_t n){ RtlCopyMemory(d,s,n); return d; }