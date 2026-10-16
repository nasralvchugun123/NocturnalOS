#include <dbg/print/kdprint.h>
#include <ds/fb/fb.h>
#include "shell.h"
#include "kbd.h"
#include <rt.h>
#include "rt.h"

#define LINE_MAX 256

static char  g_line[LINE_MAX];
static int   g_len = 0;

static void shell_prompt(void)
{
    FbPrint("C:\\> ");
}

static void shell_redraw_line(void)
{
}

static int starts_with(const char *s, const char *prefix)
{
    while (*prefix) {
        if (*s++ != *prefix++) return 0;
    }
    return 1;
}

static void cmd_help(void)
{
    FbPrint("Commands:\n");
    FbPrint("  help        - show help menu                  \n");
    FbPrint("  ver         - show OS version                 \n");
    FbPrint("  echo <text> - print text                      \n");
    FbPrint("  clear       - clear screen                    \n");
    FbPrint("  beep        - PC speaker beep                 \n");
    FbPrint("  halt        - halt the machine                \n");
    FbPrint("  crash       - call exception                  \n");
}

static void cmd_ver(void)
{
    FbPrint("NocturnalOS kernel / CLI mode\n");
    FbPrint("Build: " __DATE__ " " __TIME__ "\n");
    FbPrint("Arch:  x86_64                   \n");
}

static void cmd_echo(const char *args)
{
    while (*args == ' ') args++;
    FbPrint(args);
    FbPutChar('\n');
}

static void cmd_beep(void)
{
    #define PIT_CH2  0x42
    #define PIT_CMD  0x43
    #define PIT_FREQ 1193180
    uint32_t freq = 800;
    uint32_t div  = PIT_FREQ / freq;

    __asm__ volatile ("outb %0, %1" :: "a"((uint8_t)0xB6), "Nd"((uint16_t)PIT_CMD));
    __asm__ volatile ("outb %0, %1" :: "a"((uint8_t)(div & 0xFF)),  "Nd"((uint16_t)PIT_CH2));
    __asm__ volatile ("outb %0, %1" :: "a"((uint8_t)((div >> 8) & 0xFF)), "Nd"((uint16_t)PIT_CH2));

    uint8_t tmp;
    __asm__ volatile ("inb %1, %0" : "=a"(tmp) : "Nd"((uint16_t)0x61));
    __asm__ volatile ("outb %0, %1" :: "a"((uint8_t)(tmp | 3)), "Nd"((uint16_t)0x61));

    for (volatile int i = 0; i < 20000000; i++) {}

    __asm__ volatile ("inb %1, %0" : "=a"(tmp) : "Nd"((uint16_t)0x61));
    __asm__ volatile ("outb %0, %1" :: "a"((uint8_t)(tmp & ~3)), "Nd"((uint16_t)0x61));
}

static void shell_execute(const char *line)
{
    while (*line == ' ') line++;
    if (!*line) return;

    if (starts_with(line, "help"))  { cmd_help(); return; }
    if (starts_with(line, "ver"))   { cmd_ver();  return; }
    if (starts_with(line, "echo"))  { cmd_echo(line + 4); return; }
    if (starts_with(line, "clear")) { FbClear(0); return; }
    if (starts_with(line, "beep"))  { cmd_beep(); return; }
    if (starts_with(line, "halt"))  {
        FbPrint("Halting.\n");
        for (;;) __asm__ volatile ("hlt");
    }
        if (starts_with(line, "crashpf")) {
        KdPrint("[shell] trigger #PF\n");
        *(volatile uint64_t *)0xDEADBEEF000ULL = 0x1234;
        return;
    }
    if (starts_with(line, "crash")) {
        KdPrint("[shell] trigger #UD\n");
        __asm__ volatile ("ud2");
        return;
    }
    FbPrint("Unknown command: ");
    FbPrint(line);
    FbPutChar('\n');
}

void KiShellRun(void)
{
    FbPrint("NocturnalOS (intermediate mode)\n");
    FbPrint("Type 'help' for commands.\n\n");

    g_len = 0;
    shell_prompt();

    for (;;) {
        char c = KbdGetChar();
        if (!c) { __asm__ volatile ("pause"); continue; }

        if (c == '\n') {
            FbPutChar('\n');
            g_line[g_len] = 0;
            shell_execute(g_line);
            g_len = 0;
            shell_prompt();
        } else if (c == '\b') {
            if (g_len > 0) {
                g_len--;
                FbPutChar('\b'); FbPutChar(' '); FbPutChar('\b');
            }
        } else if (c >= 32 && c < 127) {
            if (g_len < LINE_MAX - 1) {
                g_line[g_len++] = c;
                FbPutChar(c);
            }
        }
    }
}