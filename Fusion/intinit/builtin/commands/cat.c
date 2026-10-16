
#include "../../../fusion/shared.h"

#define CAT_BUF 4096

int cmd_cat(int argc, char **argv)
{
    if (argc < 2) { FString("usage: cat <file>\n"); return 1; }

    char abs[256];
    ResolvePath(argv[1], abs, sizeof(abs));

    static char buf[4096];
    NTSTATUS st = NtReadFile(abs, buf, sizeof(buf) - 1);
    if (st < 0) {
        FString("cat: cannot read ");
        FString(abs);
        FString("\n");
        return 1;
    }

    int n = (int)st;
    if (n <= 0) { FString("cat: empty\n"); return 0; }

    for (int i = 0; i < n; i++) {
        char c = buf[i];
        if (c == '\n' || c == '\t' || (c >= 32 && c < 127)) {
        } else {
            buf[i] = '.';
        }
    }
    buf[n] = 0;
    FString(buf);
    if (buf[n - 1] != '\n') FString("\n");
    return 0;
}

int cmd_show(int argc, char **argv) { return cmd_cat(argc, argv); }