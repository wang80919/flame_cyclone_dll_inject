#include "pch.h"
#include <windows.h>
#include <tchar.h>
#include <stdlib.h>
#include "CWinHookApi.h"
#include "CXInputHook.h"
#include "Win32Utils/CFileMapping.h"
#include "Win32Utils/CVersionUtils.h"

#define XUSER_MAX_COUNT                 4

#pragma data_seg( ".mydata")
HHOOK        g_hHook = NULL;
#pragma data_seg()
#pragma comment(linker, "/section:.mydata,rws")

CFileMapping* g_pFileMapping = nullptr;
CXInputHook* g_pCXInputHook = nullptr;
bool g_fLoad = false;

Hook_Dll_API BOOL HookIsEnable()
{
    if (!g_pFileMapping)
    {
        return TRUE;
    }

    PSHARE_DATA_PROTO lpData = (PSHARE_DATA_PROTO)g_pFileMapping->GetDataPtr();
    return lpData->fHook;
}

Hook_Dll_API BOOL HookEnable(BOOL fEnable)
{
    if (!g_pFileMapping)
    {
        return TRUE;
    }

    PSHARE_DATA_PROTO lpData = (PSHARE_DATA_PROTO)g_pFileMapping->GetDataPtr();
    return lpData->fHook = fEnable;
}

Hook_Dll_API BOOL HookIsEnableXinput(DWORD dwUserIndex)
{
    if (!g_pFileMapping)
    {
        return TRUE;
    }

    PSHARE_DATA_PROTO lpData = (PSHARE_DATA_PROTO)g_pFileMapping->GetDataPtr();
    return lpData->fEnable[dwUserIndex];
}

Hook_Dll_API BOOL HookEnableXinput(DWORD dwUserIndex, BOOL fEnable)
{
    if (!g_pFileMapping)
    {
        return TRUE;
    }

    PSHARE_DATA_PROTO lpData = (PSHARE_DATA_PROTO)g_pFileMapping->GetDataPtr();
    BOOL fLastState = FALSE;
    if (lpData)
    {
        fLastState = lpData->fEnable[dwUserIndex];
        lpData->fEnable[dwUserIndex] = fEnable;
    }
    return fLastState;
}

Hook_Dll_API BOOL HookIsMergeXinput()
{
    if (!g_pFileMapping)
    {
        return TRUE;
    }

    PSHARE_DATA_PROTO lpData = (PSHARE_DATA_PROTO)g_pFileMapping->GetDataPtr();
    return lpData->fMerge;
}

Hook_Dll_API BOOL HookMergeXinput(BOOL fEnable)
{
    if (!g_pFileMapping)
    {
        return TRUE;
    }

    PSHARE_DATA_PROTO lpData = (PSHARE_DATA_PROTO)g_pFileMapping->GetDataPtr();
    BOOL fLastState = FALSE;
    if (lpData)
    {
        fLastState = lpData->fMerge;
        lpData->fMerge = fEnable;
    }
    return fLastState;
}

Hook_Dll_API DWORD HookGetXinputState(DWORD dwUserIndex, XINPUT_STATE* pState)
{
    if (!g_pFileMapping)
    {
        return ERROR_SUCCESS;
    }

    PSHARE_DATA_PROTO lpData = (PSHARE_DATA_PROTO)g_pFileMapping->GetDataPtr();
    if (lpData)
    {
        *pState = lpData->XinputState[dwUserIndex];
    }
    return ERROR_SUCCESS;
}

Hook_Dll_API VOID HookSetXinputState(DWORD dwUserIndex, XINPUT_STATE* pState)
{
    if (!g_pFileMapping)
    {
        return;
    }

    PSHARE_DATA_PROTO lpData = (PSHARE_DATA_PROTO)g_pFileMapping->GetDataPtr();
    if (lpData)
    {
        lpData->XinputState[dwUserIndex] = *pState;
    }
}

Hook_Dll_API PSHARE_DATA_PROTO HookGetShareData()
{
    if (!g_pFileMapping)
    {
        return NULL;
    }

    PSHARE_DATA_PROTO lpData = (PSHARE_DATA_PROTO)g_pFileMapping->GetDataPtr();
    return lpData;
}

Hook_Dll_API HHOOK HookLoad(DWORD dwThreadId)
{
    if (g_hHook)
    {
        return g_hHook;
    }

    g_hHook = ::SetWindowsHookEx(WH_GETMESSAGE, [](int nCode, WPARAM wParam, LPARAM lParam) {

        /*if (!g_fLoad)
        {
            g_fLoad = true;
            HookInitialize();
        }*/

        if (nCode < HC_ACTION)
        {
            return ::CallNextHookEx(g_hHook, nCode, wParam, lParam);
        }

        return ::CallNextHookEx(g_hHook, nCode, wParam, lParam);
        }, g_hModule, dwThreadId
    );

    // 安装 Hook 成功后投递一个消息, 因为消息钩子需要收到消息才能触发, 
    // 这样做可以让被 Hook 的进程立即加载钩子 DLL
    if (g_hHook)
    {
        ::PostMessage(HWND_BROADCAST, WM_NULL, 0, 0);
    }

    return g_hHook;
}

Hook_Dll_API BOOL HookUnload()
{
    BOOL fResult = FALSE;
    if (NULL == g_hHook)
    {
        return TRUE;
    }

    fResult = ::UnhookWindowsHookEx(g_hHook);
    g_hHook = NULL;

    // 安装 Hook 成功后投递一个消息, 因为消息钩子需要收到消息才能触发, 
    // 这样做可以让被 Hook 的进程立即卸载钩子 DLL
    if (fResult)
    {
        DWORD_PTR dwResult = 0;
        ::PostMessage(HWND_BROADCAST, WM_NULL, 0, 0);
        /*::SendMessageTimeout(
            HWND_BROADCAST, 
            WM_NULL, 
            0, 
            0, SMTO_ABORTIFHUNG | SMTO_NOTIMEOUTIFNOTHUNG, 
            1000, 
            &dwResult
        );*/
    }

    return fResult;
}

Hook_Dll_API BOOL HookInitialize()
{
    TCHAR szCurPath[MAX_PATH] = { 0 };
    ::GetModuleFileName(NULL, szCurPath, _countof(szCurPath));

    CVersionUtils::VERSION_INFO info = CVersionUtils::GetFileVersion(szCurPath);

    // 过滤掉微软出品
    if (_tstring::npos != info.strCompanyName.find(_T("Microsoft Corporation")))
    {
        return TRUE;
    }

    if (_tstring::npos != info.strLegalCopyright.find(_T("Microsoft Corporation")))
    {
        return TRUE;
    }

    g_pFileMapping = new (std::nothrow)CFileMapping;
    if (g_pFileMapping)
    {
        g_pFileMapping->Initialize(SHARE_MEM_NAME, 4096);
    }

    g_pCXInputHook = new (std::nothrow)CXInputHook;
    if (g_pCXInputHook)
    {
        g_pCXInputHook->Initialize();
        return TRUE;
    }

    return TRUE;
}

Hook_Dll_API VOID HookUninitialize(BOOL fDestroyShareData/* = FALSE*/)
{
    if (g_pCXInputHook)
    {
        g_pCXInputHook->Uninitialize();
        delete g_pCXInputHook;
        g_pCXInputHook = nullptr;
    }

    if (fDestroyShareData && g_pFileMapping)
    {
        g_pFileMapping->Uninitialize();
        delete g_pFileMapping;
        g_pFileMapping = nullptr;
    }
}