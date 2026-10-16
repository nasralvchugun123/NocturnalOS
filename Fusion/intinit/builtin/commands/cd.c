#include "../../../fusion/shared.h"

int cmd_cd(int argc, char **argv)
{
    if (argc < 2) {
        FString(GetCwd());
        FString("\n");
        return 0;
    }

    if (argv[1][0] == '.' && argv[1][1] == 0) return 0;

    if (argv[1][0] == '.' && argv[1][1] == '.' && argv[1][2] == 0) {
        const char *cwd = GetCwd();
        int len = 0;
        while (cwd[len]) len++;
        while (len > 1 && cwd[len - 1] != '\\') len--;
        if (len > 1) len--;
        char buf[256];
        for (int i = 0; i < len; i++) buf[i] = cwd[i];
        buf[len] = 0;
        if (len == 0) { buf[0] = '\\'; buf[1] = 0; }
        SetCwd(buf);
        return 0;
    }

    char abs[256];
    ResolvePath(argv[1], abs, sizeof(abs));

    FUSION_DIRENT tmp[1];
    ULONG n = 0;
    NTSTATUS st = NtQueryDirectory(abs, tmp, 1, &n);
    if (!NT_SUCCESS(st)) {
        FString("cd: no such directory: ");
        FString(abs);
        FString("\n");
        return 1;
    }

    SetCwd(abs);
    return 0;
}