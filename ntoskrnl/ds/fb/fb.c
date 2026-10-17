#include "fb.h"
#include <RTL/rt.h>
#include <font8x8.h>
#include <font8x16.h>
#include <ke/spinlock.h>
#include <stddef.h>
#include <stdint.h>

#define FONT_FLIP_X 0
#define FONT_FLIP_Y 0

#define FB_MAX_COLS 1024u
#define FB_MAX_ROWS 512u

typedef struct {
    volatile uint32_t *base;
    uint32_t width, height, stride;
    uint32_t format;
    uint32_t cursor_x, cursor_y;
    uint32_t fg, bg;
    int      ready;
} FB_STATE;

static KSPINLOCK gFbLock = KSPINLOCK_INIT;
static FB_STATE   g_fb = {0};
static uint8_t    g_cells[FB_MAX_ROWS][FB_MAX_COLS];
static uint16_t   g_line_len[FB_MAX_ROWS];

int FbIsReady(void) { return g_fb.ready; }

static inline uint32_t fb_color(uint8_t r, uint8_t g, uint8_t b)
{
    if (g_fb.format == 1)
        return ((uint32_t)b << 16) | ((uint32_t)g << 8) | (uint32_t)r;
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

static uint32_t fb_columns(void) { return g_fb.width / FONT8X16_W; }
static uint32_t fb_rows(void)    { return g_fb.height / FONT8X16_H; }

static int fb_shadow_fits(void)
{
    return fb_columns() <= FB_MAX_COLS && fb_rows() <= FB_MAX_ROWS;
}

static void fb_reset_text_locked(void)
{
    for (uint32_t row = 0; row < FB_MAX_ROWS; ++row) {
        g_line_len[row] = 0;
        for (uint32_t col = 0; col < FB_MAX_COLS; ++col)
            g_cells[row][col] = ' ';
    }
}

int FbInit(uint64_t Base, uint32_t W, uint32_t H, uint32_t Stride, uint32_t Format)
{
    if (!Base || !W || !H || !Stride || Stride < W) return -1;
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
    g_fb.base[(uint64_t)y * g_fb.stride + x] = color;
}

static void fb_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color)
{
    for (uint32_t j = 0; j < h; ++j) {
        if (y + j >= g_fb.height) break;
        for (uint32_t i = 0; i < w && x + i < g_fb.width; ++i)
            fb_putpixel(x + i, y + j, color);
    }
}

static void fb_clear_locked(uint32_t color)
{
    uint64_t total = (uint64_t)g_fb.stride * g_fb.height;
    for (uint64_t i = 0; i < total; ++i)
        g_fb.base[i] = color;

    g_fb.cursor_x = 0;
    g_fb.cursor_y = 0;
    fb_reset_text_locked();
}

void FbClear(uint32_t color)
{
    if (!g_fb.ready) return;
    uint64_t flags = KeAcquireSpinLockIrqSave(&gFbLock);
    fb_clear_locked(color);
    KeReleaseSpinLockIrqRestore(&gFbLock, flags);
}

static void fb_draw_char_row_locked(char c, uint32_t x, uint32_t y,
                                   uint32_t pixel_row)
{
    if ((uint8_t)c < FONT8X16_FIRST || (uint8_t)c > FONT8X16_LAST)
        c = '?';

    const uint8_t *glyph = g_font8x16[(uint8_t)c - FONT8X16_FIRST];
    uint32_t src_row = FONT_FLIP_Y ? (FONT8X16_H - 1 - pixel_row) : pixel_row;
    uint8_t bits = glyph[src_row];
    for (int col = 0; col < FONT8X16_W; ++col) {
        uint8_t mask = FONT_FLIP_X ? (1 << col) : (0x80 >> col);
        uint32_t color = (bits & mask) ? g_fb.fg : g_fb.bg;
        fb_putpixel(x + (uint32_t)col, y, color);
    }
}

static void fb_draw_char_at_locked(char c, uint32_t x, uint32_t y)
{
    for (uint32_t row = 0; row < FONT8X16_H; ++row)
        fb_draw_char_row_locked(c, x, y + row, row);
}

static void fb_record_char_locked(char c)
{
    uint32_t col = g_fb.cursor_x / FONT8X16_W;
    uint32_t row = g_fb.cursor_y / FONT8X16_H;
    if (row >= FB_MAX_ROWS || col >= FB_MAX_COLS) return;

    g_cells[row][col] = (uint8_t)c;
    if (g_line_len[row] < col + 1)
        g_line_len[row] = (uint16_t)(col + 1);
}

static void fb_scroll(void)
{
    const uint32_t line = FONT8X16_H;
    uint32_t rows = fb_rows();
    uint32_t cols = fb_columns();

    if (!rows) {
        g_fb.cursor_y = 0;
        return;
    }

    if (!fb_shadow_fits() || rows > FB_MAX_ROWS || cols > FB_MAX_COLS) {
        uint32_t y = (rows - 1) * line;
        fb_fill_rect(0, y, g_fb.width, line, g_fb.bg);
        g_fb.cursor_y = y;
        return;
    }

    for (uint32_t row = 0; row + 1 < rows; ++row) {
        uint32_t old_len = g_line_len[row];
        uint32_t new_len = g_line_len[row + 1];
        uint32_t end = old_len > new_len ? old_len : new_len;

        for (uint32_t pixel_row = 0; pixel_row < FONT8X16_H; ++pixel_row) {
            for (uint32_t col = 0; col < end; ++col) {
                uint8_t old_ch = col < old_len ? g_cells[row][col] : ' ';
                uint8_t new_ch = col < new_len ? g_cells[row + 1][col] : ' ';
                if (old_ch != new_ch)
                    fb_draw_char_row_locked((char)new_ch,
                                            col * FONT8X16_W,
                                            row * FONT8X16_H + pixel_row,
                                            pixel_row);
            }
        }
    }

    uint32_t last = rows - 1;
    for (uint32_t pixel_row = 0; pixel_row < FONT8X16_H; ++pixel_row) {
        for (uint32_t col = 0; col < g_line_len[last]; ++col)
            fb_draw_char_row_locked(' ', col * FONT8X16_W,
                                    last * FONT8X16_H + pixel_row,
                                    pixel_row);
    }

    for (uint32_t row = 0; row + 1 < rows; ++row) {
        g_line_len[row] = g_line_len[row + 1];
        for (uint32_t col = 0; col < cols; ++col)
            g_cells[row][col] = g_cells[row + 1][col];
    }
    g_line_len[last] = 0;
    for (uint32_t col = 0; col < cols; ++col)
        g_cells[last][col] = ' ';

    if (g_fb.cursor_y >= line)
        g_fb.cursor_y -= line;
    else
        g_fb.cursor_y = 0;
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
    fb_record_char_locked(c);
    fb_draw_char_at_locked(c, g_fb.cursor_x, g_fb.cursor_y);

    g_fb.cursor_x += FONT8X16_W;
    if (g_fb.cursor_x + FONT8X16_W > g_fb.width)
        fb_newline();
}

void FbPutChar(char c)
{
    if (!g_fb.ready) return;

    uint64_t flags = KeAcquireSpinLockIrqSave(&gFbLock);

    if (c == '\n') { fb_newline(); goto out; }
    if (c == '\r') {
        g_fb.cursor_x = 0;
        goto out;
    }
    if (c == '\b') {
        if (g_fb.cursor_x >= FONT8X16_W) {
            g_fb.cursor_x -= FONT8X16_W;
            fb_fill_rect(g_fb.cursor_x, g_fb.cursor_y,
                         FONT8X16_W, FONT8X16_H, g_fb.bg);
            uint32_t col = g_fb.cursor_x / FONT8X16_W;
            uint32_t row = g_fb.cursor_y / FONT8X16_H;
            if (row < FB_MAX_ROWS && col < FB_MAX_COLS) {
                g_cells[row][col] = ' ';
                while (g_line_len[row] &&
                       g_cells[row][g_line_len[row] - 1] == ' ')
                    --g_line_len[row];
            }
        }
        goto out;
    }
    if (c == '\t') {
        for (int i = 0; i < 4; ++i) fb_draw_char_locked(' ');
        goto out;
    }

    fb_draw_char_locked(c);

out:
    KeReleaseSpinLockIrqRestore(&gFbLock, flags);
}

void FbPrint(const char *s)
{
    if (!s) return;
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
    FbClear(color);
}

void FbSetColors(uint32_t Fg, uint32_t Bg)
{
    if (!g_fb.ready) return;
    uint64_t flags = KeAcquireSpinLockIrqSave(&gFbLock);
    g_fb.fg = Fg;
    g_fb.bg = Bg;
    KeReleaseSpinLockIrqRestore(&gFbLock, flags);
}
