
#include <stdint.h>

static const char gMapLow[128] = {
    0,   27, '1', '2', '3', '4', '5', '6',
    '7', '8', '9', '0', '-', '=', 8,   9,
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',
    'o', 'p', '[', ']', 13,  0,   'a', 's',
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
    '\'','`', 0,  '\\','z', 'x', 'c', 'v',
    'b', 'n', 'm', ',', '.', '/', 0,   '*',
0,   ' ', 0,   0,   0,   0,   0,   0,       

};

static const char gMapHigh[128] = {
    0,   27, '!', '@', '#', '$', '%', '^',
    '&', '*', '(', ')', '_', '+', 8,   9,
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I',
    'O', 'P', '{', '}', 13,  0,   'A', 'S',
    'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',
    '"', '~', 0,  '|', 'Z', 'X', 'C', 'V',
    'B', 'N', 'M', '<', '>', '?', 0,   '*',
    0,   ' ', 0,   0,   0,   0,   0,   0,
};

void KbdMapInit(void) { }

int KbdMapTranslate(uint8_t Sc, int *OutChar, uint8_t *Mods)
{
    if (Sc > 0x7F) return -1;

    uint8_t mods = *Mods;
    int shift = (mods & 0x03) != 0;
    int caps  = (mods & 0x40) != 0;

    int use_high = shift;
    if (caps) {
        if ((Sc >= 0x10 && Sc <= 0x19) ||   
            (Sc >= 0x1E && Sc <= 0x26) ||   
            (Sc >= 0x2C && Sc <= 0x32))    
            use_high = !use_high;
    }

    char c = use_high ? gMapHigh[Sc] : gMapLow[Sc];
    if (c == 0) return -1;

    *OutChar = (int)(uint8_t)c;
    return 0;
}