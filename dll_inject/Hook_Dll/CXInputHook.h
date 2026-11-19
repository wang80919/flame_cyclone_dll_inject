#pragma once

#include <Xinput.h>
#include <string>

typedef struct _XINPUT_KEYSTROKE
{
    WORD    VirtualKey;
    WCHAR   Unicode;
    WORD    Flags;
    BYTE    UserIndex;
    BYTE    HidCode;
} XINPUT_KEYSTROKE, * PXINPUT_KEYSTROKE;

typedef DWORD(WINAPI* _Fn_XInputGetState)(DWORD dwUserIndex, XINPUT_STATE* pState);
typedef DWORD(WINAPI* _Fn_XInputGetKeystroke)(DWORD dwUserIndex, DWORD dwReserved, PXINPUT_KEYSTROKE pKeystroke);

#ifdef _UNICODE
using _tstring = std::wstring;
#else
using _tstring = std::string;
#endif

class CXInputHook
{
public:

    bool Initialize();
    void Uninitialize();

    CXInputHook();
    ~CXInputHook();

private:

    bool _HookXInput_9_1_0_GetState();
    bool _HookXInput_1_1_GetState();
    bool _HookXInput_1_2_GetState();
    bool _HookXInput_1_3_GetState();
    bool _HookXInput_1_4_GetState();
    bool _HookXInput_1_4_GetStateEx();

    bool _HookXInput_9_1_0_XInputGetKeystroke();
    bool _HookXInput_1_1_XInputGetKeystroke();
    bool _HookXInput_1_2_XInputGetKeystroke();
    bool _HookXInput_1_3_XInputGetKeystroke();
    bool _HookXInput_1_4_XInputGetKeystroke();

    bool _HookXInput();
    _tstring _GetExpandEnvStrings(const _tstring& strPath);
    _tstring _GetCurrentModulePath();

private:
    static CXInputHook      m_Instance;
    bool                    m_fInit;
    HANDLE                  m_hThread;
    HANDLE                  m_hExitEvent;
    HANDLE                  m_hAbortEvent;

    _Fn_XInputGetState      m_XInput_9_1_0_GetState;
    _Fn_XInputGetState      m_XInput_1_1_GetState;
    _Fn_XInputGetState      m_XInput_1_2_GetState;
    _Fn_XInputGetState      m_XInput_1_3_GetState;
    _Fn_XInputGetState      m_XInput_1_4_GetState;
    _Fn_XInputGetState      m_XInput_1_4_GetStateEx;

    _Fn_XInputGetKeystroke      m_XInput_1_3_XInputGetKeystroke;
    _Fn_XInputGetKeystroke      m_XInput_1_4_XInputGetKeystroke;
};

