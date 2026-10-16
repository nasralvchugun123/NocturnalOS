
#ifndef KE_KBD_H
#define KE_KBD_H

#include <stdint.h>
#include "../ob/shared.h"

void KbdInit(void);        
void KbdInitConsole(void); 

void KbdHandleScancode(uint8_t Sc);

int  KbdReadLine(char *Buf, int MaxLen);

extern OBJECT_HEADER *KbdConsoleEvent;

void KbdSetDebug(int On);

char KbdGetChar(void);

#endif