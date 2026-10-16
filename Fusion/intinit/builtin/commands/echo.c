
#include "../../../fusion/shared.h"

int cmd_echo(int argc, char **argv)
{
    for (int i = 1; i < argc; i++) {
        if (i > 1) FString(" ");
        FString(argv[i]);
    }
    FString("\n");
    return 0;
}