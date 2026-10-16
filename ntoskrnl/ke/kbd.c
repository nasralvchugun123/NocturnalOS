#include <dbg/print/kdprint.h>
#include <ds/fb/fb.h>
#include "kbd.h"
#include <rt.h>
#include "../ob/shared.h"

#define MOD_SHIFT_L   0x01
#define MOD_SHIFT_R   0x02
#define MOD_CTRL_L    0x04
#define MOD_CTRL_R    0x08
#define MOD_ALT_L     0x10
#define MOD_ALT_R     0x20
#define MOD_CAPS      0x40

static uint8_t gMods = 0;
static int     gKbdDebug = 0;

void KbdSetDebug(int On) { gKbdDebug = On; }

extern int  KbdMapTranslate(uint8_t Sc, int *OutChar, uint8_t *Mods);
extern void KbdMapInit(void);
extern void KbdInputPush(char c);
extern void KbdInputInit(void);

void KbdInitConsole(void) { KbdInputInit(); }

void KbdInit(void)
{
    KdPrint("[kbd] KbdInit\n");
    KbdMapInit();
    gMods = 0;
    KdPrint("[kbd] ready\n");
}


void KbdHandleScancode(uint8_t sc)
{
    static int extended = 0;

    if (sc == 0xE0) { extended = 1; return; }

    int release = (sc & 0x80) != 0;
    uint8_t code = sc & 0x7F;

    if (!extended) {
        switch (code) {
        case 0x2A:
            if (release) gMods &= ~MOD_SHIFT_L; else gMods |= MOD_SHIFT_L;
            extended = 0; return;
        case 0x36:
            if (release) gMods &= ~MOD_SHIFT_R; else gMods |= MOD_SHIFT_R;
            extended = 0; return;
        case 0x1D:
            if (release) gMods &= ~MOD_CTRL_L; else gMods |= MOD_CTRL_L;
            extended = 0; return;
        case 0x38:
            if (release) gMods &= ~MOD_ALT_L; else gMods |= MOD_ALT_L;
            extended = 0; return;
        case 0x3A:
            if (!release) gMods ^= MOD_CAPS;
            extended = 0; return;
        }
    } else {
        switch (code) {
        case 0x1D:
            if (release) gMods &= ~MOD_CTRL_R; else gMods |= MOD_CTRL_R;
            extended = 0; return;
        case 0x38:
            if (release) gMods &= ~MOD_ALT_R; else gMods |= MOD_ALT_R;
            extended = 0; return;
        }
    }

    extended = 0;
    if (release) return;

    int ch = 0;
    if (KbdMapTranslate(code, &ch, &gMods) != 0 || ch == 0)
        return;

    if (ch == 0x1B) return;   /* Esc */
    if (ch > 0 && ch < 32 && ch != '\b' && ch != '\n' && ch != '\r' && ch != '\t')
        return;

    if (gKbdDebug)
        KdPrintf("[kbd] sc=%02x -> '%c' mods=%02x\n", code, ch, gMods);

    KbdInputPush((char)ch);
}