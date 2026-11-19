#include "pch.h"
#include "CXInputHook.h"
#include "MinHook/include/MinHook.h"
#include <tchar.h>
#include <Xinput.h>
#include <time.h>
#include "CWinHookApi.h"
#include <windows.h>
#include <stdlib.h>
#include <process.h>

#if defined _M_X64
#ifndef _DEBUG
#pragma comment(lib, "MinHook/lib/libMinHook-x64-v143-mt.lib")
#else
#pragma comment(lib, "MinHook/lib/libMinHook-x64-v143-mtd.lib")
#endif

#elif defined _M_IX86
#ifndef _DEBUG
#pragma comment(lib, "MinHook/lib/libMinHook-x86-v143-mt.lib")
#else
#pragma comment(lib, "MinHook/lib/libMinHook-x86-v143-mtd.lib")
#endif

#endif

_Fn_XInputGetState      Ori_XInput_9_1_0_GetState = nullptr;
_Fn_XInputGetState      Ori_XInput_1_1_GetState = nullptr;
_Fn_XInputGetState      Ori_XInput_1_2_GetState = nullptr;
_Fn_XInputGetState      Ori_XInput_1_3_GetState = nullptr;
_Fn_XInputGetState      Ori_XInput_1_4_GetState = nullptr;
_Fn_XInputGetState      Ori_XInput_1_4_GetStateEx = nullptr;

_Fn_XInputGetKeystroke      Ori_XInput_9_1_0_XInputGetKeystroke = nullptr;
_Fn_XInputGetKeystroke      Ori_XInput_1_1_XInputGetKeystroke = nullptr;
_Fn_XInputGetKeystroke      Ori_XInput_1_2_XInputGetKeystroke = nullptr;
_Fn_XInputGetKeystroke      Ori_XInput_1_3_XInputGetKeystroke = nullptr;
_Fn_XInputGetKeystroke      Ori_XInput_1_4_XInputGetKeystroke = nullptr;

DWORD WINAPI Detour_XInput_9_1_0_GetState(DWORD dwUserIndex, XINPUT_STATE* pState)
{
    if (HookIsEnable())
    {
        return HookGetXinputState(dwUserIndex, pState);
    }

    return Ori_XInput_9_1_0_GetState(dwUserIndex, pState);
}

DWORD WINAPI Detour_XInput_1_1_GetState(DWORD dwUserIndex, XINPUT_STATE* pState)
{
    if (HookIsEnable())
    {
        return HookGetXinputState(dwUserIndex, pState);
    }

    return Ori_XInput_1_1_GetState(dwUserIndex, pState);
}

DWORD WINAPI Detour_XInput_1_2_GetState(DWORD dwUserIndex, XINPUT_STATE* pState)
{
    if (HookIsEnable())
    {
        return HookGetXinputState(dwUserIndex, pState);
    }

    return Ori_XInput_1_2_GetState(dwUserIndex, pState);
}

DWORD WINAPI Detour_XInput_1_3_GetState(DWORD dwUserIndex, XINPUT_STATE* pState)
{
    if (HookIsEnable())
    {
        return HookGetXinputState(dwUserIndex, pState);
    }

    return Ori_XInput_1_3_GetState(dwUserIndex, pState);
}

DWORD WINAPI Detour_XInput_1_4_GetState(DWORD dwUserIndex, XINPUT_STATE* pState)
{
    if (HookIsEnable())
    {
        return HookGetXinputState(dwUserIndex, pState);
    }

    return Ori_XInput_1_4_GetState(dwUserIndex, pState);
}

DWORD WINAPI Detour_XInput_1_4_GetStateEx(DWORD dwUserIndex, XINPUT_STATE* pState)
{
    if (HookIsEnable())
    {
        return HookGetXinputState(dwUserIndex, pState);
    }

    return Ori_XInput_1_4_GetStateEx(dwUserIndex, pState);
}

DWORD WINAPI Detour_XInput_1_3_XInputGetKeystroke(DWORD dwUserIndex, DWORD dwReserved, PXINPUT_KEYSTROKE pKeystroke)
{
    if (HookIsEnable())
    {
        return ERROR_SUCCESS;
    }

    return Ori_XInput_1_3_XInputGetKeystroke(dwUserIndex, dwReserved, pKeystroke);
}

DWORD WINAPI Detour_XInput_1_4_XInputGetKeystroke(DWORD dwUserIndex, DWORD dwReserved, PXINPUT_KEYSTROKE pKeystroke)
{
    if (HookIsEnable())
    {
        return ERROR_SUCCESS;
    }

    return Ori_XInput_1_4_XInputGetKeystroke(dwUserIndex, dwReserved, pKeystroke);
}

CXInputHook CXInputHook::m_Instance;

CXInputHook::CXInputHook()
    :
    m_XInput_9_1_0_GetState(nullptr),
    m_XInput_1_1_GetState(nullptr),
    m_XInput_1_2_GetState(nullptr),
    m_XInput_1_3_GetState(nullptr),
    m_XInput_1_4_GetState(nullptr),
    m_XInput_1_4_GetStateEx(nullptr),
    m_hAbortEvent(false),
    m_hExitEvent(false),
    m_hThread(NULL),
    m_fInit(false)
{

}

CXInputHook::~CXInputHook()
{

}

bool CXInputHook::_HookXInput_9_1_0_GetState()
{
    MH_STATUS nCode = MH_STATUS::MH_OK;
    bool result = false;

    do
    {
        HMODULE hModule = ::GetModuleHandle(_T("XInput9_1_0.dll"));
        if (NULL == hModule)
        {
            break;
        }

        m_XInput_9_1_0_GetState = (_Fn_XInputGetState)::GetProcAddress(hModule, "XInputGetState");
        nCode = MH_CreateHook(m_XInput_9_1_0_GetState, &Detour_XInput_9_1_0_GetState, reinterpret_cast<LPVOID*>(&Ori_XInput_9_1_0_GetState));
        if (MH_OK != nCode)
        {
            break;
        }

        nCode = MH_EnableHook(m_XInput_9_1_0_GetState);
        if (MH_OK != nCode)
        {
            break;
        }

        result = true;

    } while (false);

    if (!result)
    {
        m_XInput_9_1_0_GetState = nullptr;
    }

    return result;
}

bool CXInputHook::_HookXInput_1_1_GetState()
{
    MH_STATUS nCode = MH_STATUS::MH_OK;
    bool result = false;

    do
    {
        HMODULE hModule = ::GetModuleHandle(_T("XInput1_1.dll"));
        if (NULL == hModule)
        {
            break;
        }

        m_XInput_1_1_GetState = (_Fn_XInputGetState)::GetProcAddress(hModule, "XInputGetState");
        nCode = MH_CreateHook(m_XInput_1_1_GetState, &Detour_XInput_1_1_GetState, reinterpret_cast<LPVOID*>(&Ori_XInput_1_1_GetState));
        if (MH_OK != nCode)
        {
            break;
        }

        nCode = MH_EnableHook(m_XInput_1_1_GetState);
        if (MH_OK != nCode)
        {
            break;
        }

        result = true;

    } while (false);

    if (!result)
    {
        m_XInput_1_1_GetState = nullptr;
    }

    return result;
}

bool CXInputHook::_HookXInput_1_2_GetState()
{
    MH_STATUS nCode = MH_STATUS::MH_OK;
    bool result = false;

    do
    {
        HMODULE hModule = ::GetModuleHandle(_T("XInput1_2.dll"));
        if (NULL == hModule)
        {
            break;
        }

        m_XInput_1_2_GetState = (_Fn_XInputGetState)::GetProcAddress(hModule, "XInputGetState");
        nCode = MH_CreateHook(m_XInput_1_2_GetState, &Detour_XInput_1_2_GetState, reinterpret_cast<LPVOID*>(&Ori_XInput_1_2_GetState));
        if (MH_OK != nCode)
        {
            break;
        }

        nCode = MH_EnableHook(m_XInput_1_2_GetState);
        if (MH_OK != nCode)
        {
            break;
        }

        result = true;

    } while (false);

    if (!result)
    {
        m_XInput_1_2_GetState = nullptr;
    }

    return result;
}

bool CXInputHook::_HookXInput_1_3_GetState()
{
    MH_STATUS nCode = MH_STATUS::MH_OK;
    bool result = false;

    do
    {
        HMODULE hModule = ::GetModuleHandle(_T("XInput1_3.dll"));
        if (NULL == hModule)
        {
            break;
        }

        m_XInput_1_3_GetState = (_Fn_XInputGetState)::GetProcAddress(hModule, "XInputGetState");
        nCode = MH_CreateHook(m_XInput_1_3_GetState, &Detour_XInput_1_3_GetState, reinterpret_cast<LPVOID*>(&Ori_XInput_1_3_GetState));
        if (MH_OK != nCode)
        {
            break;
        }

        nCode = MH_EnableHook(m_XInput_1_3_GetState);
        if (MH_OK != nCode)
        {
            break;
        }

        result = true;

    } while (false);

    if (!result)
    {
        m_XInput_1_3_GetState = nullptr;
    }

    return result;
}

bool CXInputHook::_HookXInput_1_4_GetState()
{
    MH_STATUS nCode = MH_STATUS::MH_OK;
    bool result = false;

    do
    {
        HMODULE hModule = ::GetModuleHandle(_T("XInput1_4.dll"));
        if (NULL == hModule)
        {
            break;
        }

        m_XInput_1_4_GetState = (_Fn_XInputGetState)::GetProcAddress(hModule, "XInputGetState");
        nCode = MH_CreateHook(m_XInput_1_4_GetState, &Detour_XInput_1_4_GetState, reinterpret_cast<LPVOID*>(&Ori_XInput_1_4_GetState));
        if (MH_OK != nCode)
        {
            break;
        }

        nCode = MH_EnableHook(m_XInput_1_4_GetState);
        if (MH_OK != nCode)
        {
            break;
        }

        result = true;

    } while (false);

    if (!result)
    {
        m_XInput_1_4_GetState = nullptr;
    }

    return result;
}

bool CXInputHook::_HookXInput_1_4_GetStateEx()
{
    MH_STATUS nCode = MH_STATUS::MH_OK;
    bool result = false;

    do
    {
        HMODULE hModule = ::GetModuleHandle(_T("XInput1_4.dll"));
        if (NULL == hModule)
        {
            break;
        }

        m_XInput_1_4_GetStateEx = (_Fn_XInputGetState)::GetProcAddress(hModule, (LPCSTR)100);
        nCode = MH_CreateHook(m_XInput_1_4_GetStateEx, &Detour_XInput_1_4_GetStateEx, reinterpret_cast<LPVOID*>(&Ori_XInput_1_4_GetStateEx));
        if (MH_OK != nCode)
        {
            break;
        }

        nCode = MH_EnableHook(m_XInput_1_4_GetStateEx);
        if (MH_OK != nCode)
        {
            break;
        }

        result = true;

    } while (false);

    if (!result)
    {
        m_XInput_1_4_GetStateEx = nullptr;
    }

    return result;
}

bool CXInputHook::_HookXInput_1_3_XInputGetKeystroke()
{
    MH_STATUS nCode = MH_STATUS::MH_OK;
    bool result = false;

    do
    {
        HMODULE hModule = ::GetModuleHandle(_T("XInput1_3.dll"));
        if (NULL == hModule)
        {
            break;
        }

        m_XInput_1_3_XInputGetKeystroke = (_Fn_XInputGetKeystroke)::GetProcAddress(hModule, (LPCSTR)"XInputGetKeystroke");
        nCode = MH_CreateHook(m_XInput_1_3_XInputGetKeystroke, &Detour_XInput_1_3_XInputGetKeystroke, reinterpret_cast<LPVOID*>(&Ori_XInput_1_3_XInputGetKeystroke));
        if (MH_OK != nCode)
        {
            break;
        }

        nCode = MH_EnableHook(m_XInput_1_3_XInputGetKeystroke);
        if (MH_OK != nCode)
        {
            break;
        }

        result = true;

    } while (false);

    if (!result)
    {
        m_XInput_1_3_XInputGetKeystroke = nullptr;
    }

    return result;
}

bool CXInputHook::_HookXInput_1_4_XInputGetKeystroke()
{
    MH_STATUS nCode = MH_STATUS::MH_OK;
    bool result = false;

    do
    {
        HMODULE hModule = ::GetModuleHandle(_T("XInput1_4.dll"));
        if (NULL == hModule)
        {
            break;
        }

        m_XInput_1_4_XInputGetKeystroke = (_Fn_XInputGetKeystroke)::GetProcAddress(hModule, (LPCSTR)"XInputGetKeystroke");
        nCode = MH_CreateHook(m_XInput_1_4_XInputGetKeystroke, &Detour_XInput_1_4_XInputGetKeystroke, reinterpret_cast<LPVOID*>(&Ori_XInput_1_4_XInputGetKeystroke));
        if (MH_OK != nCode)
        {
            break;
        }

        nCode = MH_EnableHook(m_XInput_1_4_XInputGetKeystroke);
        if (MH_OK != nCode)
        {
            break;
        }

        result = true;

    } while (false);

    if (!result)
    {
        m_XInput_1_4_XInputGetKeystroke = nullptr;
    }

    return result;
}

_tstring CXInputHook::_GetExpandEnvStrings(const _tstring& strPath)
{
    TCHAR szBuf[MAX_PATH] = { 0 };
    ::ExpandEnvironmentStrings(strPath.c_str(), szBuf, _countof(szBuf));
    return szBuf;
}

_tstring CXInputHook::_GetCurrentModulePath()
{
    TCHAR szCurPath[MAX_PATH] = { 0 };
    ::GetModuleFileName(NULL, szCurPath, _countof(szCurPath));

    return szCurPath;
}

bool CXInputHook::_HookXInput()
{
    while (true)
    {
        DWORD dwWait = ::WaitForSingleObject(m_hAbortEvent, 1000);
        if (WAIT_TIMEOUT != dwWait)
        {
            break;
        }

        if (!m_XInput_9_1_0_GetState)
        {
            _HookXInput_9_1_0_GetState();
        }

        if (!m_XInput_1_1_GetState)
        {
            _HookXInput_1_1_GetState();
        }

        if (!m_XInput_1_2_GetState)
        {
            _HookXInput_1_2_GetState();
        }

        if (!m_XInput_1_3_GetState)
        {
            _HookXInput_1_3_GetState();
        }

        if (!m_XInput_1_4_GetState)
        {
            _HookXInput_1_4_GetState();
        }

        if (!m_XInput_1_4_GetStateEx)
        {
            _HookXInput_1_4_GetStateEx();
        }

        if (!m_XInput_1_3_XInputGetKeystroke)
        {
            _HookXInput_1_3_XInputGetKeystroke();
        }

        if (!m_XInput_1_4_XInputGetKeystroke)
        {
            _HookXInput_1_4_XInputGetKeystroke();
        }
    }

    return false;
}

bool CXInputHook::Initialize()
{
    MH_STATUS nCode = MH_STATUS::MH_OK;

    _tstring strExeBuf = _GetCurrentModulePath();
    _tstring strEnvBuf = _GetExpandEnvStrings(_T("%windir%"));

    // 不对系统应用进行HOOK 如: C:\Windows\*.exe
    if (0 == _tcsnicmp(strEnvBuf.c_str(), strExeBuf.c_str(), strEnvBuf.size()))
    {
        return true;
    }

    nCode = MH_Initialize();
    if (MH_OK != nCode)
    {
        return false;
    }

    m_fInit = true;

    if (m_fInit)
    {
        DWORD dwTID = 0;

        m_hAbortEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
        m_hExitEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);

        if (m_hAbortEvent && m_hExitEvent)
        {
            auto task = ([](LPVOID lpThreadParameter) -> DWORD {
                CXInputHook* pThis = (CXInputHook*)lpThreadParameter;
                pThis->_HookXInput();
                if (pThis->m_hExitEvent)
                {
                    ::SetEvent(pThis->m_hExitEvent);
                }
                return 0;
                }
            );

            // 创建线程进程 API HOOK 操作, 不要使用 std::thread, 会死锁
            m_hThread = ::CreateThread(NULL,
                0,
                task,
                this,
                0,
                &dwTID
            );
        }
    }

    return true;
}

void CXInputHook::Uninitialize()
{
    if (m_fInit)
    {
        if (m_hThread)
        {
            // 通知线程中止
            if (m_hAbortEvent)
            {
                ::SetEvent(m_hAbortEvent);
            }

            // 等待线程退出, 不要等待线程句柄, 会死锁
            ::WaitForSingleObject(m_hExitEvent, 5000);

            // 延迟等待线程结束, 否则会导致进程崩溃
            ::Sleep(200);

            // 关闭句柄资源
            ::CloseHandle(m_hThread);
            ::CloseHandle(m_hAbortEvent);
            ::CloseHandle(m_hExitEvent);

            m_hThread = NULL;
            m_hAbortEvent = NULL;
            m_hExitEvent = NULL;
        }

        if (m_XInput_9_1_0_GetState)
        {
            MH_DisableHook(m_XInput_9_1_0_GetState);
        }

        if (m_XInput_1_1_GetState)
        {
            MH_DisableHook(m_XInput_1_1_GetState);
        }

        if (m_XInput_1_2_GetState)
        {
            MH_DisableHook(m_XInput_1_2_GetState);
        }

        if (m_XInput_1_3_GetState)
        {
            MH_DisableHook(m_XInput_1_3_GetState);
        }

        if (m_XInput_1_4_GetState)
        {
            MH_DisableHook(m_XInput_1_4_GetState);
        }

        if (m_XInput_1_4_GetStateEx)
        {
            MH_DisableHook(m_XInput_1_4_GetStateEx);
        }

        MH_Uninitialize();
    }

    m_fInit = false;
}
