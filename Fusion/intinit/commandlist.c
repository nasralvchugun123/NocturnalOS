#include "../fusion/shared.h"

typedef struct {
    const char *Name;
    CMDFN       Fn;
} CMD;

static const CMD gCmds[] = {
    { "help", cmd_help },
    { "echo", cmd_echo },
    { "ver",  cmd_ver  },
    { "exit", cmd_exit },
    { "ls",   cmd_ls   },
    { "cd",   cmd_cd   },
    { "cat",  cmd_cat  },
    { "show", cmd_show },
    { "find", cmd_find },
    { "dlltest", cmd_dlltest },

    { 0, 0 }
};

static int str_eq(const char *a, const char *b)
{
    while (*a && *b) { if (*a != *b) return 0; a++; b++; }
    return *a == 0 && *b == 0;
}

int dispatch(int argc, char **argv)
{
    for (const CMD *c = gCmds; c->Name; c++) {
        if (str_eq(argv[0], c->Name))
            return c->Fn(argc, argv);
    }

    FString("unknown command: ");
    FString(argv[0]);
    FString("\n");
    return 0;
}