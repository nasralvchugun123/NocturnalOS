#include <nt/ntdll.h>

int main(int argc, char **argv)
{
    (void)argc; (void)argv;

    NtDisplayString("[csrss] alive\n");

    HANDLE hFusion = 0;
    NTSTATUS st = NtCreateProcess(
        "\\NocturnalOS\\system\\shell\\fusion\\fusion.exe",
        "\\NocturnalOS\\system\\shell\\fusion\\fusion.exe",
        &hFusion);

    if (!NT_SUCCESS(st)) {
        NtDisplayString("[csrss] fusion create failed\n");
    } else {
        NtClose(hFusion);
        NtDisplayString("[csrss] fusion started\n");
    }
    return 0;
}