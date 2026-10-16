#include <nt/ntdefs.h>

extern BOOLEAN DllMain(HANDLE hinstDLL, ULONG fdwReason, PVOID lpReserved);

#define DLL_PROCESS_ATTACH 1
#define DLL_PROCESS_DETACH 0
#define DLL_THREAD_ATTACH  2
#define DLL_THREAD_DETACH  3

BOOLEAN _DllMainCRTStartup(HANDLE hinstDLL, ULONG fdwReason, PVOID lpReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        // CRT init goes here 
    }
    BOOLEAN ok = DllMain(hinstDLL, fdwReason, lpReserved);
    if (fdwReason == DLL_PROCESS_DETACH) {
        // CRT fini goes here 
    }
    return ok;
}