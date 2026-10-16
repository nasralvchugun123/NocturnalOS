#ifndef _RTL_MEM_H
#define _RTL_MEM_H

#include <nt/ntdefs.h>

VOID  RtlCopyMemory(PVOID dst, const VOID *src, SIZE_T n);
VOID  RtlMoveMemory(PVOID dst, const VOID *src, SIZE_T n);
VOID  RtlFillMemory(PVOID dst, SIZE_T n, UINT8 v);
VOID  RtlZeroMemory(PVOID dst, SIZE_T n);
int   RtlCompareMemory(const VOID *a, const VOID *b, SIZE_T n);

VOID *memcpy(VOID *dst, const VOID *src, SIZE_T n);
VOID *memmove(VOID *dst, const VOID *src, SIZE_T n);
VOID *memset(VOID *dst, int c, SIZE_T n);
int   memcmp(const VOID *a, const VOID *b, SIZE_T n);

#endif