
#include "../../../fusion/shared.h"

#define MAX_ENTRIES 64

int cmd_ls(int argc, char **argv)
{
        NtDisplayString("[ls] enter\n");    

    const char *path = (argc >= 2) ? argv[1] : GetCwd();

    char abs[256];
    ResolvePath(path, abs, sizeof(abs));

    FUSION_DIRENT entries[MAX_ENTRIES];
    ULONG count = 0;

    NTSTATUS st = NtQueryDirectory(abs, entries, MAX_ENTRIES, &count);
    if (st < 0) {
        FString("ls: cannot open ");
        FString(abs);
        FString("\n");
        return 1;
    }

    for (ULONG i = 0; i < count; i++) {
        FUSION_DIRENT *e = &entries[i];
        if (e->Attr) {
            FString("[DIR] ");
        } else {
            FString("      ");
        }
        FString(e->Name);
        if (!e->Attr) {
            char num[16];
            uint32_t v = e->Size;
            int p = 0;
            if (v == 0) num[p++] = '0';
            while (v > 0) { num[p++] = '0' + (v % 10); v /= 10; }
            for (int a = 0, b = p - 1; a < b; a++, b--) {
                char t = num[a]; num[a] = num[b]; num[b] = t;
            }
            num[p] = 0;
            FString("    ");
            FString(num);
            FString(" bytes");
        }
        FString("\n");
    }

    return 0;
}