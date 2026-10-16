
#include "shared.h"
#include <rt.h>
#include <dbg/print/kdprint.h>
#include <ds/fb/fb.h>

static void obp_strncpy(char *dst, const char *src, size_t n)
{
    size_t i = 0;
    while (i + 1 < n && src[i]) { dst[i] = src[i]; i++; }
    dst[i] = 0;
}

static int obp_streq_ci(const char *a, const char *b)
{
    while (*a && *b) {
        char ca = *a, cb = *b;
        if (ca >= 'A' && ca <= 'Z') ca += 32;
        if (cb >= 'A' && cb <= 'Z') cb += 32;
        if (ca != cb) return 0;
        a++; b++;
    }
    return *a == 0 && *b == 0;
}

int ObpInsertObjectByName(OBJECT_HEADER *Dir, const char *Name,
                          OBJECT_HEADER *Obj)
{
    if (!Dir || !Name || !Obj) return -1;
    if (Dir->Type != &ObpDirectoryType) return -1;

    OBJECT_DIRECTORY *d = (OBJECT_DIRECTORY *)OBJ_BODY(Dir);

    for (uint32_t i = 0; i < OBJ_DIR_MAX; i++) {
        if (d->Entries[i].Object &&
            obp_streq_ci(d->Entries[i].Name, Name))
            return -2;
    }

    for (uint32_t i = 0; i < OBJ_DIR_MAX; i++) {
        if (!d->Entries[i].Object) {
            obp_strncpy(d->Entries[i].Name, Name, OBJ_NAME_MAX);
            d->Entries[i].Object = Obj;
            d->EntryCount++;
            ObReferenceObject(OBJ_BODY(Obj));
            KdPrintf("[Ob] inserted '\\BaseNamedObjects\\%s'\n", Name);
            return 0;
        }
    }
    return -3;
}

OBJECT_HEADER *ObpLookupObjectByName(OBJECT_HEADER *Dir, const char *Name)
{
    if (!Dir || !Name) return 0;
    if (Dir->Type != &ObpDirectoryType) return 0;

    OBJECT_DIRECTORY *d = (OBJECT_DIRECTORY *)OBJ_BODY(Dir);

    for (uint32_t i = 0; i < OBJ_DIR_MAX; i++) {
        if (d->Entries[i].Object &&
            obp_streq_ci(d->Entries[i].Name, Name))
            return d->Entries[i].Object;
    }
    return 0;
}

void ObpDeleteObjectByName(OBJECT_HEADER *Dir, const char *Name)
{
    if (!Dir || !Name) return;
    if (Dir->Type != &ObpDirectoryType) return;

    OBJECT_DIRECTORY *d = (OBJECT_DIRECTORY *)OBJ_BODY(Dir);

    for (uint32_t i = 0; i < OBJ_DIR_MAX; i++) {
        if (d->Entries[i].Object &&
            obp_streq_ci(d->Entries[i].Name, Name)) {
            KdPrintf("[Ob] removed '\\BaseNamedObjects\\%s'\n", Name);
            d->Entries[i].Object = 0;
            d->Entries[i].Name[0] = 0;
            if (d->EntryCount > 0) d->EntryCount--;
            return;
        }
    }
}

int ObpParsePath(const char *Path, OBJECT_HEADER **OutDir,
                 char *OutName, size_t NameSize)
{
    if (!Path || !OutDir || !OutName || NameSize == 0) return -1;

    const char *p = Path;
    while (*p == '\\') p++;

    const char *bno = "BaseNamedObjects";
    int i = 0;
    while (bno[i]) {
        char c = p[i];
        if (c >= 'A' && c <= 'Z') c += 32;
        char b = bno[i];
        if (b >= 'A' && b <= 'Z') b += 32;
        if (c != b) return -2;
        i++;
    }
    p += i;

    if (*p != '\\') return -3;
    p++;
    if (*p == 0) return -4;

    obp_strncpy(OutName, p, NameSize);

    *OutDir = ObpBaseNamedObjects;
    return 0;
}