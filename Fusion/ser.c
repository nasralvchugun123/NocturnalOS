
#include "fusion/shared.h"

static FUSION_MODE detect_mode(void)
{
    return MODE_CLI;
}

int main(int argc, char **argv)
{
    (void)argc; (void)argv;

    FUSION_MODE mode = detect_mode();

    if (mode == MODE_CLI)
        return System_initmode_cli();

    FString("[fusion] unsupported mode\n");
    return 1;
}