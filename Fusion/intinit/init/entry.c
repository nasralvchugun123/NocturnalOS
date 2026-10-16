
#include "../../fusion/shared.h"

void FString(const char *Str)
{
    NtWriteConsole(Str);
}

static char gCwd[256] = "\\";

const char *GetCwd(void) { return gCwd; }

void SetCwd(const char *NewCwd)
{
    int i = 0;
    while (NewCwd[i] && i < 255) { gCwd[i] = NewCwd[i]; i++; }
    gCwd[i] = 0;
}

void ResolvePath(const char *In, char *Out, int MaxLen)
{
    if (In[0] == '\\') {
        int i = 0;
        while (In[i] && i < MaxLen - 1) { Out[i] = In[i]; i++; }
        Out[i] = 0;
        return;
    }

    int p = 0;
    for (int i = 0; gCwd[i] && p < MaxLen - 1; i++) Out[p++] = gCwd[i];
    if (p > 0 && Out[p - 1] != '\\' && p < MaxLen - 1) Out[p++] = '\\';
    for (int i = 0; In[i] && p < MaxLen - 1; i++) Out[p++] = In[i];
    Out[p] = 0;
}

static int tokenize(char *Line, char **Argv, int MaxArgc)
{
    int argc = 0;
    char *p = Line;
    while (*p && argc < MaxArgc - 1) {
        while (*p == ' ' || *p == '\t') p++;
        if (!*p) break;
        Argv[argc++] = p;
        while (*p && *p != ' ' && *p != '\t') p++;
        if (*p) *p++ = 0;
    }
    Argv[argc] = 0;
    return argc;
}

static void strip_eol(char *s)
{
    for (int i = 0; s[i]; i++) {
        if (s[i] == '\r' || s[i] == '\n') { s[i] = 0; return; }
    }
}

int System_initmode_cli(void)
{
    FString("\n");
    FString("                                                         /////////////////////////////////////                                                                \n");
    FString("                                                                       NocturnalOS                                                                            \n");
    FString("                                                         /////////////////////////////////////                                                                \n");
    FString("\n\n");
    
    FString("welcome to fusion fallback mode!                   \n\n");
    FString("that mean graphical mode is unavaible for some reasons\n");
    FString("or not implemented.\n");
    FString("anyway check debug trace or serial1 if you are dev \n");

    FString("\n\n");

    FString("type help for commands\n");
    
    FString("\n\n");

    char line[MAX_LINE];
    char *argv[MAX_ARGS];

    for (;;) {
        FString("> ");

        int n = NtReadConsole(line, sizeof(line));
        if (n <= 0) continue;

        strip_eol(line);
        if (line[0] == 0) continue;

        int argc = tokenize(line, argv, MAX_ARGS);
        if (argc == 0) continue;

        int rc = dispatch(argc, argv);
        if (rc == 1) break;  
    }

    FString("shell exited.\n");
    return 0;
}