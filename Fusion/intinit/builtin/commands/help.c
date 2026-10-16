
#include "../../../fusion/shared.h"

int cmd_help(int argc, char **argv)
{
    (void)argc; (void)argv;
    FString("Commands:\n");
    FString("  help            - this text\n");
    FString("  echo <t>        - print text\n");
    FString("  ver             - version\n");
    FString("  ls <path>       - list directory\n");
    FString("  dlltest         - call dll subsystem test \n");
    FString("  cd <path>       - change directory\n");
    FString("  cat <file>      - show file\n");
    FString("  find <file>     - trying to find file\n");
    FString("  exit            - exit shell\n");
    return 0;
}