#include <nt/ntdefs.h>

int g_test_value = 0xCAFEBABE;
const char g_greeting[] = "hello from testdll";

__declspec(dllexport) int TestAdd(int a, int b)
{
    return a + b;
}

__declspec(dllexport) int TestGetValue(void)
{
    return g_test_value;
}

__declspec(dllexport) const char *TestHello(void)
{
    return g_greeting;
}

int __stdcall DllMain(void *hinst, unsigned long reason, void *reserved)
{
    (void)hinst; (void)reason; (void)reserved;
    return 1;
}