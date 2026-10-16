#ifndef _RTL_STRING_H
#define _RTL_STRING_H

#include <nt/ntdefs.h>

SIZE_T RtlStringLength(const CHAR *s);
int    RtlStringCompare(const CHAR *a, const CHAR *b);
CHAR  *RtlStringCopy(CHAR *dst, const CHAR *src);
CHAR  *RtlStringConcat(CHAR *dst, const CHAR *src);

VOID   RtlInitUnicodeString(PUNICODE_STRING us, const WCHAR *src);
VOID   RtlInitAnsiString(PANSI_STRING as, const CHAR *src);
VOID   RtlFreeUnicodeString(PUNICODE_STRING us);

SIZE_T strlen(const CHAR *s);
int    strcmp(const CHAR *a, const CHAR *b);
int    strncmp(const CHAR *a, const CHAR *b, SIZE_T n);
CHAR  *strcpy(CHAR *dst, const CHAR *src);
CHAR  *strncpy(CHAR *dst, const CHAR *src, SIZE_T n);
CHAR  *strchr(const CHAR *s, int c);
CHAR  *strrchr(const CHAR *s, int c);

#endif