#include <RTL/rt.h>

SIZE_T RtlStringLength(const CHAR *s)
{
    const CHAR *p = s;
    while (*p) p++;
    return (SIZE_T)(p - s);
}

int RtlStringCompare(const CHAR *a, const CHAR *b)
{
    while (*a && *a == *b) { a++; b++; }
    return (int)(UINT8)*a - (int)(UINT8)*b;
}

int RtlStringCompareNoCase(const CHAR *a, const CHAR *b)
{
    while (*a && *b) {
        CHAR ca = *a, cb = *b;
        if (ca >= 'A' && ca <= 'Z') ca += 32;
        if (cb >= 'A' && cb <= 'Z') cb += 32;
        if (ca != cb) return (int)(UINT8)ca - (int)(UINT8)cb;
        a++; b++;
    }
    return (int)(UINT8)*a - (int)(UINT8)*b;
}

CHAR *RtlStringCopy(CHAR *dst, const CHAR *src)
{
    CHAR *r = dst;
    while ((*dst++ = *src++)) {}
    return r;
}

CHAR *RtlStringConcat(CHAR *dst, const CHAR *src)
{
    CHAR *r = dst;
    while (*dst) dst++;
    while ((*dst++ = *src++)) {}
    return r;
}

SIZE_T strlen(const CHAR *s) { return RtlStringLength(s); }
int    strcmp(const CHAR *a, const CHAR *b) { return RtlStringCompare(a, b); }

int strncmp(const CHAR *a, const CHAR *b, SIZE_T n)
{
    while (n && *a && *a == *b) { a++; b++; n--; }
    if (n == 0) return 0;
    return (int)(UINT8)*a - (int)(UINT8)*b;
}

CHAR *strcpy(CHAR *dst, const CHAR *src) { return RtlStringCopy(dst, src); }

CHAR *strncpy(CHAR *dst, const CHAR *src, SIZE_T n)
{
    CHAR *r = dst;
    while (n && (*dst = *src)) { dst++; src++; n--; }
    while (n--) *dst++ = 0;
    return r;
}

CHAR *strchr(const CHAR *s, int c)
{
    while (*s) {
        if (*s == (CHAR)c) return (CHAR *)s;
        s++;
    }
    return (c == 0) ? (CHAR *)s : 0;
}