#include <nt/ntdll.h>

int _DllMainCRTStartup(void *h, unsigned long r, void *x)
{ (void)h; (void)r; (void)x; return 1; }

#define DLLEXPORT __declspec(dllexport)

DLLEXPORT int Add(int a, int b) { return a + b; }
DLLEXPORT int Sub(int a, int b) { return a - b; }
DLLEXPORT int Mul(int a, int b) { return a * b; }

DLLEXPORT const char *GetName(void) { return "DLLTEST"; }

DLLEXPORT void SayHello(void)
{
    NtWriteConsole("[dlltest] hello from DLL\n");
}