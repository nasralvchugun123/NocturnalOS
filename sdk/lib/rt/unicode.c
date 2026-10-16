#include <RTL/rt.h>

VOID RtlInitUnicodeString(PUNICODE_STRING us, const WCHAR *src)
{
    if (!us) return;
    if (!src) {
        us->Length = 0;
        us->MaximumLength = 0;
        us->Buffer = NULL;
        return;
    }
    SIZE_T bytes = RtlStringLength((const CHAR *)src) * 2;
    
    us->Buffer = (PWSTR)src;
    us->Length = (USHORT)bytes;
    us->MaximumLength = (USHORT)(bytes + 2);
}

VOID RtlInitAnsiString(PANSI_STRING as, const CHAR *src)
{
    if (!as) return;
    if (!src) {
        as->Length = 0;
        as->MaximumLength = 0;
        as->Buffer = NULL;
        return;
    }
    SIZE_T len = RtlStringLength(src);
    as->Buffer = (PCHAR)src;
    as->Length = (USHORT)len;
    as->MaximumLength = (USHORT)(len + 1);
}

VOID RtlFreeUnicodeString(PUNICODE_STRING us)
{
    (void)us;
}