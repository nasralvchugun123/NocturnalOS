#ifndef FUSION_SHARED_H
#define FUSION_SHARED_H

#include <stdint.h>
#include <nt/ntdll.h>

typedef enum {
    MODE_UNKNOWN = 0,
    MODE_CLI,
    MODE_GUI,
} FUSION_MODE;

#define MAX_ARGS 16
#define MAX_LINE 256

typedef struct {
    char     Name[64];
    uint32_t Attr;
    uint32_t Size;
} FUSION_DIRENT;

void FString(const char *Str);

int  System_initmode_cli(void);

typedef int (*CMDFN)(int argc, char **argv);

int  dispatch(int argc, char **argv);
const char *GetCwd(void);
void        SetCwd(const char *NewCwd);
void        ResolvePath(const char *In, char *Out, int MaxLen);
int  cmd_help(int argc, char **argv);
int  cmd_echo(int argc, char **argv);
int  cmd_ver (int argc, char **argv);
int  cmd_exit(int argc, char **argv);
int  cmd_ls  (int argc, char **argv);
int  cmd_cd  (int argc, char **argv);
int  cmd_cat (int argc, char **argv);
int  cmd_show(int argc, char **argv);
int  cmd_find(int argc, char **argv);
int  cmd_dlltest(int argc, char **argv);

#endif