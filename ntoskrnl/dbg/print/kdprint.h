#ifndef _KE_KDPRINT_H
#define _KE_KDPRINT_H

#include <nt/ntdefs.h>

VOID KdInitSerial(VOID);
VOID KdBeginUserConsole(VOID);
VOID KdPrint(const char *s);
VOID KdPrintf(const char *fmt, ...);

#endif