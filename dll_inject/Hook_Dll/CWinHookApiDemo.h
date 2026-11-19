#pragma once

#if 0
#include <windows.h>
#include <Xinput.h>

#ifdef Hook_Dll_EXPORTS
#define Hook_Dll_API __declspec(dllexport)
#else
#define Hook_Dll_API __declspec(dllimport)
#endif

extern "C"
{
    Hook_Dll_API BOOL LoadCbtHook();
    Hook_Dll_API VOID UnloadCbtHook();

    Hook_Dll_API BOOL LoadKeyboardHook();
    Hook_Dll_API VOID UnloadKeyboardHook();

    Hook_Dll_API BOOL LoadMsgHook();
    Hook_Dll_API VOID UnloadMsgHook();

    Hook_Dll_API BOOL LoadGetMsgHook();
    Hook_Dll_API VOID UnloadGetMsgHook();
}

#endif