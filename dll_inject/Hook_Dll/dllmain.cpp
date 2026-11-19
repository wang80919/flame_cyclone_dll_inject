#include "pch.h"
#include "MinHook/include/MinHook.h"
#include <tchar.h>
#include "CWinHookApi.h"

HMODULE g_hModule = NULL;

BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    UNREFERENCED_PARAMETER(lpReserved);

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        g_hModule = hModule;
        if (!HookInitialize())
        {
            return FALSE;
        }
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        HookUninitialize(TRUE);
        break;
    }
    return TRUE;
}
