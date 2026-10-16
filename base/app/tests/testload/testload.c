#include <nt/ntdefs.h>
#include <nt/ntdll.h>
#include <nt/ntstatus.h>

typedef int  (*AddFn)(int, int);
typedef int  (*GetValFn)(void);
typedef const char *(*HelloFn)(void);

static void make_unicode(UNICODE_STRING *us, WCHAR *buf, int cap,
                         const char *ascii)
{
    int i = 0;
    while (ascii[i] && i + 1 < cap) { buf[i] = (WCHAR)ascii[i]; i++; }
    buf[i] = 0;
    us->Length        = (USHORT)(i * 2);
    us->MaximumLength = (USHORT)((i + 1) * 2);
    us->Buffer        = buf;
}

int main(void)
{
    NtDisplayString("[testload] begin\n");

    WCHAR pathw[128];
    UNICODE_STRING us;
    make_unicode(&us, pathw, 128,
                 "\\NocturnalOS\\System\\Shell\\Tests\\testdll.dll");

    void *base = 0;
    NTSTATUS st = LdrLoadDll(0, 0, &us, &base);
    if (st != 0 || !base) {
        NtDisplayString("[testload] LdrLoadDll FAILED\n");
        return 1;
    }
    NtDisplayString("[testload] dll loaded\n");

    char name_add[] = "TestAdd";
    ANSI_STRING as_add = { 7, 8, name_add };
    AddFn add = 0;
    st = LdrGetProcedureAddress(base, &as_add, 0, (void **)&add);
    if (st != 0 || !add) {
        NtDisplayString("[testload] TestAdd not found\n");
        return 2;
    }
    int r = add(2, 3);
    NtDisplayString(r == 5 ? "[testload] TestAdd(2,3)=5 OK\n"
                           : "[testload] TestAdd WRONG\n");

    char name_gv[] = "TestGetValue";
    ANSI_STRING as_gv = { 12, 13, name_gv };
    GetValFn gv = 0;
    if (LdrGetProcedureAddress(base, &as_gv, 0, (void **)&gv) == 0 && gv) {
        int v = gv();
        NtDisplayString(v == 0xCAFEBABE ? "[testload] TestGetValue OK\n"
                                        : "[testload] TestGetValue WRONG\n");
    }

    char name_hi[] = "TestHello";
    ANSI_STRING as_hi = { 9, 10, name_hi };
    HelloFn hi = 0;
    if (LdrGetProcedureAddress(base, &as_hi, 0, (void **)&hi) == 0 && hi) {
        NtDisplayString("[testload] TestHello -> ");
        NtDisplayString(hi());
        NtDisplayString("\n");
    }

    NtDisplayString("[testload] done\n");
    return 0;
}