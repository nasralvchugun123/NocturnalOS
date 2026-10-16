#include <nt/ntdll.h>
#include <RTL/rt.h>

extern int main(int argc, char **argv, char **envp);

#define MAX_ARGS 64

static int parse_cmdline(char *cmd, char **argv, int max)
{
    int argc = 0;
    char *p = cmd;
    while (*p && argc < max - 1) {
        while (*p == ' ' || *p == '\t') p++;
        if (!*p) break;
        if (*p == '"') {
            p++;
            argv[argc++] = p;
            while (*p && *p != '"') p++;
            if (*p) *p++ = 0;
        } else {
            argv[argc++] = p;
            while (*p && *p != ' ' && *p != '\t') p++;
            if (*p) *p++ = 0;
        }
    }
    argv[argc] = 0;
    return argc;
}

__attribute__((noreturn))
void _start(RTL_USER_PROCESS_PARAMETERS *params)
{
    int argc = 0;
    char *argv[MAX_ARGS];

    if (params && params->CommandLine.Buffer && params->CommandLine.Length) {
        argc = parse_cmdline((char *)params->CommandLine.Buffer, argv, MAX_ARGS);
    }
    if (argc == 0) {
        argv[0] = (char *)"<unknown>";
        argv[1] = 0;
        argc = 1;
    }

    int rc = main(argc, argv, (char **)params->Environment);
    NtTerminateProcess(0, (LONG)rc);

    for (;;) __asm__ volatile ("hlt");
}