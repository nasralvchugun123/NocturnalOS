#include "../../../fusion/shared.h"

#define FIND_MAX_PATH    512
#define FIND_MAX_DEPTH   32
#define FIND_ENTS        64

typedef struct _FIND_CTX {
    char           Path[FIND_MAX_PATH];
    uint32_t       Len;
    uint32_t       Depth;
    uint64_t       Files;
    uint64_t       Dirs;
    uint64_t       Bytes;
    FUSION_DIRENT  Ents[FIND_ENTS];
} FIND_CTX;

static FIND_CTX gFind;

static int StrEqI(const char* a, const char* b)
{
    for (; *a && *b; a++, b++) {
        char ca = *a, cb = *b;
        if (ca >= 'a' && ca <= 'z') ca -= 32;
        if (cb >= 'a' && cb <= 'z') cb -= 32;
        if (ca != cb) return 0;
    }
    return *a == 0 && *b == 0;
}

static int IsDotEntry(const char* n)
{
    if (n[0] == '.' && n[1] == 0) return 1;
    if (n[0] == '.' && n[1] == '.' && n[2] == 0) return 1;
    return 0;
}

static uint32_t PathPush(FIND_CTX* c, const char* name)
{
    uint32_t save = c->Len;
    if (c->Len > 0 && c->Path[c->Len - 1] != '\\' && c->Len + 1 < FIND_MAX_PATH)
        c->Path[c->Len++] = '\\';
    while (*name && c->Len + 1 < FIND_MAX_PATH)
        c->Path[c->Len++] = *name++;
    c->Path[c->Len] = 0;
    return save;
}

static void PathPop(FIND_CTX* c, uint32_t save)
{
    c->Len = save;
    c->Path[save] = 0;
}

static void U64ToStr(uint64_t v, char* out)
{
    if (v == 0) { out[0]='0'; out[1]=0; return; }
    char t[24]; int n = 0;
    while (v) { t[n++] = '0' + (v % 10); v /= 10; }
    int i = 0; while (n) out[i++] = t[--n];
    out[i] = 0;
}

static const char* ExtOf(const char* name)
{
    const char* dot = 0;
    for (const char* p = name; *p; p++) if (*p == '.') dot = p;
    return (dot && dot[1]) ? dot + 1 : 0;
}

static void FormatFromExt(const char* name, char* out, uint32_t cap)
{
    const char* e = ExtOf(name);
    if (!e) { out[0] = 0; return; }

    const char* tag;
    if (StrEqI(e,"EXE") || StrEqI(e,"SYS") || StrEqI(e,"DLL") || StrEqI(e,"EFI"))
        tag = "PE";
    else if (StrEqI(e,"TXT") || StrEqI(e,"INI") ||
             StrEqI(e,"LOG") || StrEqI(e,"CFG"))
        tag = "TEXT";
    else
        tag = e;

    uint32_t i = 0;
    while (tag[i] && i + 1 < cap) {
        char c = tag[i];
        if (c >= 'a' && c <= 'z') c -= 32;
        out[i++] = c;
    }
    out[i] = 0;
}

static void MagicSig(const char* path, char* out, uint32_t cap)
{
    out[0] = 0;
    uint8_t  buf[4] = {0,0,0,0};
    NTSTATUS st = NtReadFile(path, buf, 4);
    if (!NT_SUCCESS(st)) return;

    if (buf[0] == 'M' && buf[1] == 'Z') { out[0]='M'; out[1]='Z'; out[2]=0; return; }
    if (buf[0] == 0x7F && buf[1] == 'E'){ out[0]='E'; out[1]='L'; out[2]='F'; out[3]=0; return; }
    if (buf[0] == 0 && buf[1] == 0)     { out[0]='-'; out[1]=0; return; }

    static const char H[] = "0123456789ABCDEF";
    out[0] = H[(buf[0] >> 4) & 0xF];
    out[1] = H[buf[0] & 0xF];
    out[2] = H[(buf[1] >> 4) & 0xF];
    out[3] = H[buf[1] & 0xF];
    out[4] = 0;
}

static void PrintEntry(const char* full, const char* name, uint32_t size)
{
    char fmt[16], sig[8], num[24];

    FormatFromExt(name, fmt, sizeof(fmt));
    MagicSig(full, sig, sizeof(sig));
    U64ToStr((uint64_t)size, num);

    NtWriteConsole("path:\"");     NtWriteConsole(full); NtWriteConsole("\"\n");
    NtWriteConsole("filesize:\""); NtWriteConsole(num);  NtWriteConsole("\"\n");
    NtWriteConsole("file: \"");    NtWriteConsole(fmt);
    NtWriteConsole("\" \"");       NtWriteConsole(sig);  NtWriteConsole("\"\n");
}

static void FindWalk(FIND_CTX* c)
{
    if (c->Depth >= FIND_MAX_DEPTH) return;

    ULONG count = 0;
    if (!NT_SUCCESS(NtQueryDirectory(c->Path, c->Ents, FIND_ENTS, &count)))
    return;

    for (uint32_t i = 0; i < count; i++) {
        FUSION_DIRENT* e = &c->Ents[i];
        if (IsDotEntry(e->Name)) continue;

        uint32_t save = PathPush(c, e->Name);

        if (e->Attr & 0x10u /* DIRECTORY */) {
            c->Dirs++;
            c->Depth++;
            FindWalk(c);
            c->Depth--;
        } else {
            c->Files++;
            c->Bytes += e->Size;
            PrintEntry(c->Path, e->Name, e->Size);
        }

        PathPop(c, save);
    }
}

int cmd_find(int argc, char **argv)
{
    const char *arg = (argc >= 2) ? argv[1] : 0;

    FIND_CTX* c = &gFind;
    c->Len   = 0;
    c->Depth = 0;
    c->Files = 0;
    c->Dirs  = 0;
    c->Bytes = 0;
    c->Path[0] = 0;

    if (arg && *arg) {
        uint32_t i = 0;
        while (arg[i] && i + 1 < FIND_MAX_PATH) { c->Path[i] = arg[i]; i++; }
        c->Path[i] = 0;
        c->Len = i;
    } else {
        const char *cwd = GetCwd();
        if (cwd) {
            uint32_t i = 0;
            while (cwd[i] && i + 1 < FIND_MAX_PATH) { c->Path[i] = cwd[i]; i++; }
            c->Path[i] = 0;
            c->Len = i;
        }
    }

    FindWalk(c);

    char n1[24], n2[24], n3[32];
    U64ToStr(c->Files, n1);
    U64ToStr(c->Dirs,  n2);
    U64ToStr(c->Bytes, n3);
    NtWriteConsole("files:"); NtWriteConsole(n1);
    NtWriteConsole(" dirs:"); NtWriteConsole(n2);
    NtWriteConsole(" bytes:");NtWriteConsole(n3);
    NtWriteConsole("\n");
    return 0;
}