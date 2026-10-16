#ifndef _KE_FB_H
#define _KE_FB_H

#include <stdint.h>
#include <nt/ntdefs.h>

int  FbInit(ULONG64 Base, UINT32 Width, UINT32 Height,
            UINT32 Stride, UINT32 Format);
VOID FbClear(UINT32 color);
VOID FbPutChar(char c);
VOID FbPrint(const char *s);
VOID FbPrintf(const char *fmt, ...);
int  FbIsReady(VOID);

VOID     FbFillScreen(UINT32 color);
VOID     FbSetColors(UINT32 Fg, UINT32 Bg);


#endif