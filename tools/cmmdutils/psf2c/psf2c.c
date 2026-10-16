
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s <font.psfu>\n", argv[0]);
        return 1;
    }

    FILE *f = fopen(argv[1], "rb");
    if (!f) { perror("fopen"); return 1; }

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    uint8_t *data = malloc((size_t)sz);
    if (!data) { perror("malloc"); return 1; }
    if (fread(data, 1, (size_t)sz, f) != (size_t)sz) {
        perror("fread"); return 1;
    }
    fclose(f);

    if (sz < 32) { fprintf(stderr, "too small\n"); return 1; }

    uint32_t magic   = *(uint32_t *)(data + 0);
    uint32_t hdrsize = *(uint32_t *)(data + 8);
    uint32_t nglyphs = *(uint32_t *)(data + 16);
    uint32_t charsz  = *(uint32_t *)(data + 20);
    uint32_t height  = *(uint32_t *)(data + 24);
    uint32_t width   = *(uint32_t *)(data + 28);

    if (magic != 0x864AB572) {
        fprintf(stderr, "not PSF2 (magic=%08x)\n", magic);
        return 1;
    }
    if (width != 8 || height != 16) {
        fprintf(stderr, "expected 8x16, got %ux%u\n", width, height);
        return 1;
    }
    if (nglyphs < 127) {
        fprintf(stderr, "only %u glyphs\n", nglyphs);
        return 1;
    }

    int first = 32, last = 126;

    printf("/* Auto-generated. Do not edit. */\n");
    printf("#ifndef FONT8X16_H\n#define FONT8X16_H\n\n");
    printf("#include <stdint.h>\n\n");
    printf("#define FONT8X16_FIRST  %d\n", first);
    printf("#define FONT8X16_LAST   %d\n", last);
    printf("#define FONT8X16_W      8\n");
    printf("#define FONT8X16_H      16\n\n");
    printf("static const uint8_t g_font8x16[%d][16] = {\n",
           last - first + 1);

    for (int code = first; code <= last; code++) {
        uint8_t *g = data + hdrsize + (uint32_t)code * charsz;
        printf("    {");
        for (int i = 0; i < 16; i++)
            printf(" 0x%02X,", g[i]);
        char ch = (code >= 32 && code < 127) ? (char)code : '?';
        printf(" }, /* %3d '%c' */\n", code, ch);
    }

    printf("};\n\n#endif\n");
    free(data);
    return 0;
}