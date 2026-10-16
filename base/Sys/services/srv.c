#include <nt/ntdll.h>

static void test_heap(void)
{
    void *mem = 0;
    ULONG64 size1 = 4096;

    NtDisplayString("[ramtest] start\n");

    NTSTATUS st = NtAllocateVirtualMemory(&mem, 0, &size1, 0, 0);
    if (!NT_SUCCESS(st) || !mem) {
        NtDisplayString("[ramtest] alloc FAILED\n");
        return;
    }

    NtDisplayString("[ramtest] alloc OK\n");

    char *p = (char *)mem;
    const char *msg = "hello heap";
    int i = 0;
    while (msg[i]) { p[i] = msg[i]; i++; }
    p[i] = 0;

    int ok = 1;
    for (int j = 0; msg[j]; j++) {
        if (p[j] != msg[j]) { ok = 0; break; }
    }
    if (p[i] != 0) ok = 0;

    if (ok) {
        NtDisplayString("[ramtest] read-back OK: ");
        NtDisplayString(p);
        NtDisplayString("\n");
    } else {
        NtDisplayString("[ramtest] read-back FAILED\n");
    }

    void *mem2 = 0;
    ULONG64 size2 = 8192;
    st = NtAllocateVirtualMemory(&mem2, 0, &size2, 0, 0);
    if (NT_SUCCESS(st) && mem2 && mem2 != mem) {
        NtDisplayString("[ramtest] second alloc OK\n");
        char *q = (char *)mem2;
        q[0] = 'X'; q[1] = 0;
        if (q[0] == 'X' && p[0] == 'h')
            NtDisplayString("[ramtest] second read-back OK\n");
        NtFreeVirtualMemory(&mem2, &size2, 0);
    } else {
        NtDisplayString("[ramtest] second alloc FAILED\n");
    }

    NtFreeVirtualMemory(&mem, &size1, 0);
    NtDisplayString("[ramtest] done\n");
}

int main(int argc, char **argv)
{
    (void)argc; (void)argv;

    NtDisplayString("[services] creating named event\n");

    HANDLE hEv = 0;
    NTSTATUS st = NtCreateEvent(&hEv, NotificationEvent, 0,
                                "\\BaseNamedObjects\\FusionReady");

    HANDLE hM = 0, hS = 0;
    NtCreateMutant(&hM, 0, "\\BaseNamedObjects\\FusionMutex");
    NtCreateSemaphore(&hS, 2, 8, "\\BaseNamedObjects\\FusionSem");

    if (!NT_SUCCESS(st)) {
        NtDisplayString("[services] failed to create named event\n");
        return 1;
    }
    NtDisplayString("[services] named event created\n");

        HANDLE hCsrss = 0;
    st = NtCreateProcess("\\NocturnalOS\\system\\shell\\subsys\\csrss.exe",
                         "\\NocturnalOS\\system\\shell\\subsys\\csrss.exe",
                         &hCsrss);
    if (!NT_SUCCESS(st)) {
        NtDisplayString("[services] csrss create failed\n");
        return 1;
    }

    NtDisplayString("[services] csrss created\n");

    NtDelayExecution(3000);
    NtDisplayString("[services] signaling FusionReady\n");
    NtSetEvent(hEv, 0);

    NtWaitForSingleObject(hCsrss, 30000);

    NtDisplayString("[services] done\n");

    test_heap();
    NtDisplayString("[services] looping\n");


}
