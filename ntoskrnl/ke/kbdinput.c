#include <dbg/print/kdprint.h>
#include <ds/fb/fb.h>
#include "kbd.h"
#include <rt.h>
#include "rt.h" 
#include "../ob/shared.h"

#define KBD_BUF_SIZE 256

static char            gBuf[KBD_BUF_SIZE];
static volatile int    gHead = 0;
static volatile int    gTail = 0;

OBJECT_HEADER *KbdConsoleEvent = 0;

void KbdInputInit(void)
{
    gHead = 0;
    gTail = 0;

    KEVENT *ev = (KEVENT *)ObCreateObject(&ObpEventType, sizeof(KEVENT));
    if (!ev) {
        KdPrint("[kbd] cannot create console event\n");
        return;
    }
    ev->EventType = SynchronizationEvent;  
    KbdConsoleEvent = OBJ_HEADER(ev);
    KdPrint("[kbd] console event created\n");
}

char KbdGetChar(void)
{
    if (!KbdConsoleEvent) return 0;

    while (gTail == gHead) {
        ObWaitForSingleObject(KbdConsoleEvent, 0xFFFFFFFFFFFFFFFFULL);
    }

    char c = gBuf[gTail];
    gTail = (gTail + 1) % KBD_BUF_SIZE;
    return c;
}

void KbdInputPush(char c)
{
    if (c == '\r') c = '\n';

    if (c == '\b') {
        if (gHead == gTail) return;                 
        int prev = (gHead - 1 + KBD_BUF_SIZE) % KBD_BUF_SIZE;
        if (gBuf[prev] == '\n') return;                 
        gHead = prev;
        FbPutChar('\b');                                  
        return;
    }

    int next = (gHead + 1) % KBD_BUF_SIZE;
    if (next == gTail) return;

    gBuf[gHead] = c;
    gHead = next;

    FbPutChar(c);

    if (c == '\n' && KbdConsoleEvent)
        ObSignalObject(KbdConsoleEvent);
}

static int has_line(void)
{
    int i = gTail;
    while (i != gHead) {
        if (gBuf[i] == '\n') return 1;
        i = (i + 1) % KBD_BUF_SIZE;
    }
    return 0;
}

int KbdReadLine(char *Buf, int MaxLen)
{
    KdPrintf("[kbd-readline] enter\n");
    if (!KbdConsoleEvent) return 0;

    while (!has_line()) {
        KdPrintf("[kbd-readline] waiting\n");
        ObWaitForSingleObject(KbdConsoleEvent, 0xFFFFFFFFFFFFFFFFULL);
    }

    KdPrintf("[kbd-readline] got line\n");

    if (!KbdConsoleEvent) return 0;
    if (MaxLen <= 0) return 0;

    while (!has_line()) {
        ObWaitForSingleObject(KbdConsoleEvent, 0xFFFFFFFFFFFFFFFFULL);
    }

    int n = 0;
    while (gTail != gHead && n < MaxLen - 1) {
        char c = gBuf[gTail];
        gTail = (gTail + 1) % KBD_BUF_SIZE;
        Buf[n++] = c;
        if (c == '\n') break;
    }
    Buf[n] = 0;
    return n;

    KdPrintf("[kbd-readline] return %d\n", n);
    return n;
}




