#include <RTL/rt.h>
#include <stdint.h>

typedef struct { char *buf; SIZE_T cap; SIZE_T pos; } OUT;

static void oc(OUT *o, char c)
{ if (o->buf && o->pos + 1 < o->cap) o->buf[o->pos] = c; o->pos++; }

static void os(OUT *o, const char *s)
{ if (!s) s = "(null)"; while (*s) oc(o, *s++); }

static void ou(OUT *o, uint64_t v, int base, int up, int w, char pad)
{
    char t[32]; int i = 0;
    const char *d = up ? "0123456789ABCDEF" : "0123456789abcdef";
    if (!v) t[i++] = '0';
    else while (v) { t[i++] = d[v % base]; v /= base; }
    for (int p = i; p < w; p++) oc(o, pad);
    while (i--) oc(o, t[i]);
}

static void oi(OUT *o, int64_t v, int w, char pad)
{ if (v < 0) { oc(o, '-'); ou(o, (uint64_t)(-v), 10, 0, w ? w-1 : 0, pad); }
  else ou(o, (uint64_t)v, 10, 0, w, pad); }

static int isd(char c) { return c >= '0' && c <= '9'; }

int RtlVsprintf(char *buf, SIZE_T n, const char *fmt, __builtin_va_list ap)
{
    OUT o = { buf, n, 0 };
    while (*fmt) {
        if (*fmt != '%') { oc(&o, *fmt++); continue; }
        fmt++;
        char pad = ' '; int w = 0;
        if (*fmt == '0') { pad = '0'; fmt++; }
        while (isd(*fmt)) { w = w*10 + (*fmt-'0'); fmt++; }
        int l = 0, ll = 0;
        if (*fmt == 'l') { l = 1; fmt++; if (*fmt == 'l') { ll = 1; fmt++; } }
        else if (*fmt == 'z') { l = 1; fmt++; }
        switch (*fmt) {
        case 's': os(&o, __builtin_va_arg(ap, const char *)); break;
        case 'c': oc(&o, (char)__builtin_va_arg(ap, int)); break;
        case 'd': case 'i': {
            int64_t v = ll ? __builtin_va_arg(ap, long long)
                      : l  ? __builtin_va_arg(ap, long)
                           : __builtin_va_arg(ap, int);
            oi(&o, v, w, pad); break;
        }
        case 'u': {
            uint64_t v = ll ? __builtin_va_arg(ap, unsigned long long)
                       : l  ? __builtin_va_arg(ap, unsigned long)
                            : __builtin_va_arg(ap, unsigned int);
            ou(&o, v, 10, 0, w, pad); break;
        }
        case 'x': case 'X': {
            uint64_t v = ll ? __builtin_va_arg(ap, unsigned long long)
                       : l  ? __builtin_va_arg(ap, unsigned long)
                            : __builtin_va_arg(ap, unsigned int);
            ou(&o, v, 16, (*fmt == 'X'), w, pad); break;
        }
        case 'p': {
            uint64_t v = (uint64_t)(uintptr_t)__builtin_va_arg(ap, void *);
            os(&o, "0x"); ou(&o, v, 16, 0, 16, '0'); break;
        }
        case '%': oc(&o, '%'); break;
        default:  oc(&o, '%'); if (*fmt) oc(&o, *fmt); break;
        }
        if (*fmt) fmt++;
    }
    if (buf && n) buf[o.pos < n ? o.pos : n-1] = 0;
    return (int)o.pos;
}

int vsnprintf(char *buf, SIZE_T n, const char *fmt, __builtin_va_list ap)
{ return RtlVsprintf(buf, n, fmt, ap); }

int RtlSprintf(char *buf, SIZE_T n, const char *fmt, ...)
{
    __builtin_va_list ap; __builtin_va_start(ap, fmt);
    int r = RtlVsprintf(buf, n, fmt, ap);
    __builtin_va_end(ap); return r;
}

int snprintf(char *buf, SIZE_T n, const char *fmt, ...)
{
    __builtin_va_list ap; __builtin_va_start(ap, fmt);
    int r = RtlVsprintf(buf, n, fmt, ap);
    __builtin_va_end(ap); return r;
}