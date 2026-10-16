#include "fb.h"
#include <RTL/rt.h>
#include <font8x8.h>
#include <font8x16.h>
#include <ke/spinlock.h>

#define FONT_FLIP_X 0
#define FONT_FLIP_Y 0

typedef struct {
    volatile uint32_t *base;
    uint32_t width, height, stride;
    uint32_t format;
    uint32_t cursor_x, cursor_y;
    uint32_t fg, bg;
    int      ready;
} FB_STATE;

static KSPINLOCK gFbLock = KSPINLOCK_INIT;
static FB_STATE   g_fb    = {0};

int FbIsReady(void) { return g_fb.ready; }

static inline uint32_t fb_color(uint8_t r, uint8_t g, uint8_t b)
{
    if (g_fb.format == 1)
        return ((uint32_t)b << 16) | ((uint32_t)g << 8) | (uint32_t)r;
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

int FbInit(uint64_t Base, uint32_t W, uint32_t H, uint32_t Stride, uint32_t Format)
{
    if (!Base || !W || !H || !Stride) return -1;
    g_fb.base     = (volatile uint32_t *)(uintptr_t)Base;
    g_fb.width    = W;
    g_fb.height   = H;
    g_fb.stride   = Stride;
    g_fb.format   = Format;
    g_fb.cursor_x = 0;
    g_fb.cursor_y = 0;
    g_fb.fg       = fb_color(0xC0, 0xC0, 0xC0);
    g_fb.bg       = fb_color(0x00, 0x00, 0x00);
    g_fb.ready    = 1;
    FbClear(g_fb.bg);
    return 0;
}

static inline void fb_putpixel(uint32_t x, uint32_t y, uint32_t color)
{
    if (x >= g_fb.width || y >= g_fb.height) return;
    g_fb.base[y * g_fb.stride + x] = color;
}

static void fb_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color)
{
    for (uint32_t j = 0; j < h; j++)
        for (uint32_t i = 0; i < w; i++)
            fb_putpixel(x + i, y + j, color);
}

void FbClear(uint32_t color)
{
    if (!g_fb.ready) return;
    for (uint32_t y = 0; y < g_fb.height; y++)
        for (uint32_t x = 0; x < g_fb.width; x++)
            g_fb.base[y * g_fb.stride + x] = color;
    g_fb.cursor_x = 0;
    g_fb.cursor_y = 0;
}

static inline void fb_fast_copy(uint32_t *dst, const uint32_t *src, size_t count)
{
    size_t bytes = count * sizeof(uint32_t);
    __asm__ volatile ("rep movsb"
        : "+D"(dst), "+S"(src), "+c"(bytes)
        : : "memory");
}

static void fb_scroll(void)
{
    uint32_t line  = FONT8X16_H;
    uint32_t shift = line * g_fb.stride;
    uint32_t total = g_fb.stride * g_fb.height;

    if (shift >= total) {
        for (uint32_t i = 0; i < total; i++) g_fb.base[i] = g_fb.bg;
        g_fb.cursor_y = 0;
        return;
    }

    uint32_t copy = total - shift;
    uint32_t *base = (uint32_t *)g_fb.base;

    fb_fast_copy(base, base + shift, copy);
    for (uint32_t i = copy; i < total; i++) base[i] = g_fb.bg;

    g_fb.cursor_y -= line;
}

static void fb_newline(void)
{
    g_fb.cursor_x = 0;
    g_fb.cursor_y += FONT8X16_H;
    while (g_fb.cursor_y + FONT8X16_H > g_fb.height)
        fb_scroll();
}

static void fb_draw_char_locked(char c)
{
    if ((uint8_t)c < FONT8X16_FIRST || (uint8_t)c > FONT8X16_LAST)
        c = '?';

    const uint8_t *glyph = g_font8x16[(uint8_t)c - FONT8X16_FIRST];

    for (int row = 0; row < FONT8X16_H; row++) {
        int src_row = FONT_FLIP_Y ? (FONT8X16_H - 1 - row) : row;
        uint8_t bits = glyph[src_row];
        for (int col = 0; col < FONT8X16_W; col++) {
            uint8_t mask = FONT_FLIP_X ? (1 << col) : (0x80 >> col);
            uint32_t color = (bits & mask) ? g_fb.fg : g_fb.bg;
            fb_putpixel(g_fb.cursor_x + col, g_fb.cursor_y + row, color);
        }
    }
    g_fb.cursor_x += FONT8X16_W;
    if (g_fb.cursor_x + FONT8X16_W > g_fb.width)
        fb_newline();
}

void FbPutChar(char c)
{
    if (!g_fb.ready) return;

    uint64_t f = KeAcquireSpinLockIrqSave(&gFbLock);

    if (c == '\n') { fb_newline(); goto out; }
    if (c == '\r') { g_fb.cursor_x = 0; goto out; }
    if (c == '\b') {
        if (g_fb.cursor_x >= FONT8X16_W) {
            g_fb.cursor_x -= FONT8X16_W;
            fb_fill_rect(g_fb.cursor_x, g_fb.cursor_y,
                         FONT8X16_W, FONT8X16_H, g_fb.bg);
        }
        goto out;
    }
    if (c == '\t') {
        for (int i = 0; i < 4; i++) fb_draw_char_locked(' ');
        goto out;
    }

    fb_draw_char_locked(c);

out:
    KeReleaseSpinLockIrqRestore(&gFbLock, f);
}

void FbPrint(const char *s)
{
    while (*s) FbPutChar(*s++);
}

void FbPrintf(const char *fmt, ...)
{
    char buf[512];
    __builtin_va_list ap;
    __builtin_va_start(ap, fmt);
    RtlVsprintf(buf, sizeof(buf), fmt, ap);
    __builtin_va_end(ap);
    FbPrint(buf);
}

void FbFillScreen(uint32_t color)
{
    if (!g_fb.ready) return;
    uint64_t f = KeAcquireSpinLockIrqSave(&gFbLock);
    uint32_t total = g_fb.stride * g_fb.height;
    for (uint32_t i = 0; i < total; i++) g_fb.base[i] = color;
    g_fb.cursor_x = 0;
    g_fb.cursor_y = 0;
    KeReleaseSpinLockIrqRestore(&gFbLock, f);
}

void FbSetColors(uint32_t Fg, uint32_t Bg)
{
    uint64_t f = KeAcquireSpinLockIrqSave(&gFbLock);
    g_fb.fg = Fg;
    g_fb.bg = Bg;
    KeReleaseSpinLockIrqRestore(&gFbLock, f);
}