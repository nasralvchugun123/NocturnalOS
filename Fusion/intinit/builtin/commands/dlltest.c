#include "../../../fusion/shared.h"

int cmd_dlltest(int argc, char **argv)
{
    (void)argc; (void)argv;

    const char *path = "\\NocturnalOS\\System\\Shell\\Tests\\testload.exe";

    FString("[dlltest] spawning ");
    FString(path);
    FString("\n");

    HANDLE hProc = 0;
    NTSTATUS rc = NtCreateProcess(path, 0, &hProc);

    if (rc != 0 || hProc == 0) {
        FString("[dlltest] NtCreateProcess FAILED\n");
        return -1;
    }

    FString("[dlltest] spawned, waiting...\n");

    NTSTATUS wr = NtWaitForSingleObject(hProc, 0xFFFFFFFFu);

    if (wr == 0) {
        FString("[dlltest] process exited\n");
    } else {
        FString("[dlltest] wait returned non-zero\n");
    }

    NtClose(hProc);
    return 0;
}