#include <nt/ntdll.h>

typedef int  (*Fn_II)(int, int);
typedef const char *(*Fn_V)(void);
typedef void (*Fn_VV)(void);

#define OK(s)   NtWriteConsole("[ringtest] OK   " s "\n")
#define FAIL(s) NtWriteConsole("[ringtest] FAIL " s "\n")

int main(int argc, char **argv)
{
    (void)argc; (void)argv;

    NtWriteConsole("[ringtest] start\n");

    HANDLE h = 0;
    NTSTATUS st = NtLoadLibrary("DLLTEST", &h);
    if (!NT_SUCCESS(st) || !h) {
        FAIL("NtLoadLibrary(DLLTEST)");
        return 1;
    }
    OK("NtLoadLibrary(DLLTEST)");

    void *addr = 0;
    st = NtGetProcAddress(h, "Add", &addr);
    if (!NT_SUCCESS(st) || !addr) {
        FAIL("NtGetProcAddress(Add)");
        return 2;
    }
    Fn_II Add = (Fn_II)addr;
    if (Add(2, 3) == 5) OK("Add(2,3)=5");
    else                FAIL("Add(2,3) wrong");

    addr = 0;
    st = NtGetProcAddress(h, "Mul", &addr);
    if (!NT_SUCCESS(st) || !addr) {
        FAIL("NtGetProcAddress(Mul)");
        return 3;
    }
    Fn_II Mul = (Fn_II)addr;
    if (Mul(6, 7) == 42) OK("Mul(6,7)=42");
    else                 FAIL("Mul(6,7) wrong");

    addr = 0;
    st = NtGetProcAddress(h, "GetName", &addr);
    if (!NT_SUCCESS(st) || !addr) {
        FAIL("NtGetProcAddress(GetName)");
        return 4;
    }
    Fn_V GetName = (Fn_V)addr;
    const char *nm = GetName();
    NtWriteConsole("[ringtest] GetName -> ");
    NtWriteConsole(nm ? nm : "(null)");
    NtWriteConsole("\n");

    addr = 0;
    st = NtGetProcAddress(h, "SayHello", &addr);
    if (!NT_SUCCESS(st) || !addr) {
        FAIL("NtGetProcAddress(SayHello)");
        return 5;
    }
    Fn_VV SayHello = (Fn_VV)addr;
    SayHello();

    NtWriteConsole("[ringtest] done\n");
    return 0;
}