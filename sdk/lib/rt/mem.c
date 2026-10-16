#include <RTL/rt.h>

VOID RtlCopyMemory(PVOID dst, const VOID *src, SIZE_T n)
{
    UINT8 *d = (UINT8 *)dst;
    const UINT8 *s = (const UINT8 *)src;
    while (n--) *d++ = *s++;
}

VOID RtlMoveMemory(PVOID dst, const VOID *src, SIZE_T n)
{
    UINT8 *d = (UINT8 *)dst;
    const UINT8 *s = (const UINT8 *)src;
    if (d < s) {
        while (n--) *d++ = *s++;
    } else {
        d += n; s += n;
        while (n--) *--d = *--s;
    }
}

VOID RtlFillMemory(PVOID dst, SIZE_T n, UINT8 v)
{
    UINT8 *d = (UINT8 *)dst;
    while (n--) *d++ = v;
}

VOID RtlZeroMemory(PVOID dst, SIZE_T n)
{
    RtlFillMemory(dst, n, 0);
}

int RtlCompareMemory(const VOID *a, const VOID *b, SIZE_T n)
{
    const UINT8 *x = (const UINT8 *)a;
    const UINT8 *y = (const UINT8 *)b;
    while (n--) {
        if (*x != *y) return (int)*x - (int)*y;
        x++; y++;
    }
    return 0;
}

VOID *memcpy(VOID *dst, const VOID *src, SIZE_T n)
{
    RtlCopyMemory(dst, src, n);
    return dst;
}
VOID *memmove(VOID *dst, const VOID *src, SIZE_T n)
{
    RtlMoveMemory(dst, src, n);
    return dst;
}
VOID *memset(VOID *dst, int c, SIZE_T n)
{
    RtlFillMemory(dst, n, (UINT8)c);
    return dst;
}
int memcmp(const VOID *a, const VOID *b, SIZE_T n)
{
    return RtlCompareMemory(a, b, n);
}