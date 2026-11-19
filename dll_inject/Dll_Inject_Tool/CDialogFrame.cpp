#include "CDialogFrame.h"
#include <Windows.h>
#include <strsafe.h>
#include "resource.h"
#include "Win32Utils/CPathUtils.h"
#include "Win32Utils/CComFileDlg.h"
#include "Win32Utils/CStrUtils.h"
#include "MessageBoxCenter.h"
#include <thread>
#include <algorithm>
#include <dwmapi.h>
#include <xinput.h>
#include "../Common/InjectUtils.h"

#pragma comment(lib, "Dwmapi.lib")
#pragma comment(lib, "Xinput.lib")

#ifdef _WIN64
#pragma comment(lib, "../Bin/Hook_Dll.lib")
#else
#pragma comment(lib, "../Bin/Hook_Dll_x86.lib")

#endif // _WIN64

#define WM_TRAYICON                 (WM_USER + 10)

BEGIN_DLG_MESSAGE_MAP(CDialogFrame, CDialogBase)
    ON_DLG_MESSAGE(WM_INITDIALOG, &CDialogFrame::OnInitDialog)
    ON_DLG_MESSAGE(WM_TRAYICON, &CDialogFrame::OnTrayIcon)
    ON_DLG_MESSAGE(WM_CLOSE, &CDialogFrame::OnClose)
    ON_DLG_MESSAGE(WM_DISPLAYCHANGE, &CDialogFrame::OnDisplayChange)
    ON_DLG_MESSAGE(WM_DROPFILES, &CDialogFrame::OnDropFiles)
    ON_DLG_MESSAGE(WM_TIMER, &CDialogFrame::OnTimer)

    ON_DLG_MESSAGE(WM_LBUTTONDOWN, &CDialogFrame::OnLButtonDown)
    ON_DLG_MESSAGE(WM_LBUTTONUP, &CDialogFrame::OnLButtonUp)
    ON_DLG_MESSAGE(WM_MOUSEMOVE, &CDialogFrame::OnMouseMove)

    ON_DLG_COMMAND(ID_FILE_EXIT, &CDialogFrame::OnCommandExit)
    ON_DLG_COMMAND(ID_SETTINGS_TOPMOST, &CDialogFrame::OnTopmost)
    ON_DLG_COMMAND(IDC_BUTTON_REFRESH, &CDialogFrame::OnRefresh)
    ON_DLG_COMMAND(IDC_BUTTON_FIND_PID_INJECT, &CDialogFrame::OnFindPidInject)
    ON_DLG_COMMAND(IDC_BUTTON_FIND_NAME_INJECT, &CDialogFrame::OnFindNameInject)
    ON_DLG_COMMAND(IDC_BUTTON_DLL_PATH_INJECT, &CDialogFrame::OnDllPathInject)
    ON_DLG_COMMAND(IDC_BUTTON_INJECT_LOAD, &CDialogFrame::OnLoad)
    ON_DLG_COMMAND(IDC_BUTTON_INJECT_UNLOAD, &CDialogFrame::OnUnload)
    ON_DLG_COMMAND(IDC_BUTTON_DLL_PATH_HOOK, &CDialogFrame::OnDllPathHook)
    ON_DLG_COMMAND(IDC_BUTTON_HOOK_LOAD, &CDialogFrame::OnHookLoad)
    ON_DLG_COMMAND(IDC_BUTTON_HOOK_UNLOAD, &CDialogFrame::OnHookUnload)
    ON_DLG_COMMAND(IDC_CHECK_AUTO_REFRESH, &CDialogFrame::OnCheckAutoRefresh)
    ON_DLG_COMMAND_RANGE(IDC_RADIO_AUTO_REFRESH_0, IDC_RADIO_AUTO_REFRESH_2, &CDialogFrame::OnRadioRefreshSpeed)
    ON_DLG_COMMAND_RANGE(IDC_BUTTON_XBOX_ENABLE_0, IDC_BUTTON_XBOX_ENABLE_3, &CDialogFrame::OnXboxEnable)
    ON_DLG_COMMAND(IDC_BUTTON_XBOX_HOOK, &CDialogFrame::OnXboxHookEnable)
    ON_DLG_COMMAND(IDC_BUTTON_XBOX_MERGE, &CDialogFrame::OnXboxMerge)
    ON_DLG_COMMAND(IDC_CHECK_HOLD_A, &CDialogFrame::OnXboxHoldA)
    ON_DLG_COMMAND(IDC_CHECK_HOLD_B, &CDialogFrame::OnXboxHoldB)
    ON_DLG_COMMAND(IDC_CHECK_HOLD_X, &CDialogFrame::OnXboxHoldX)
    ON_DLG_COMMAND(IDC_CHECK_HOLD_Y, &CDialogFrame::OnXboxHoldY)
    ON_DLG_COMMAND(IDC_CHECK_HOLD_LB, &CDialogFrame::OnXboxHoldLB)
    ON_DLG_COMMAND(IDC_CHECK_HOLD_RB, &CDialogFrame::OnXboxHoldRB)
    ON_DLG_COMMAND(IDC_CHECK_BURST_A, &CDialogFrame::OnXboxBurstA)
    ON_DLG_COMMAND(IDC_CHECK_BURST_B, &CDialogFrame::OnXboxBurstB)
    ON_DLG_COMMAND(IDC_CHECK_BURST_X, &CDialogFrame::OnXboxBurstX)
    ON_DLG_COMMAND(IDC_CHECK_BURST_Y, &CDialogFrame::OnXboxBurstY)
    ON_DLG_COMMAND(IDC_CHECK_BURST_LB, &CDialogFrame::OnXboxBurstLB)
    ON_DLG_COMMAND(IDC_CHECK_BURST_RB, &CDialogFrame::OnXboxBurstRB)

    ON_DLG_NOTIFY(NM_CLICK, IDC_LIST_PROCESS, &CDialogFrame::ListViewRowClick)
    ON_DLG_NOTIFY(NM_DBLCLK, IDC_LIST_PROCESS, &CDialogFrame::ListViewRowClick)
    ON_DLG_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_PROCESS, &CDialogFrame::ListViewItemChanged)
    ON_DLG_NOTIFY(LVN_COLUMNCLICK, IDC_LIST_PROCESS, &CDialogFrame::ListViewColumnClick)
    ON_DLG_NOTIFY(LVN_ODFINDITEM, IDC_LIST_PROCESS, &CDialogFrame::ListViewItemFind)
    ON_DLG_NOTIFY(LVN_GETDISPINFO, IDC_LIST_PROCESS, &CDialogFrame::ListViewGetDisplayInfo)

END_DLG_MESSAGE_MAP()

LPCTSTR lpColName[] = {
    _T("序号"),
    _T("进程 ID"),
    _T("CPU"),
    _T("进程名"),
    _T("会话 ID"),
    _T("域名"),
    _T("用户名"),
    _T("句柄"),
    _T("线程"),
    _T("用户对象"),
    _T("GDI 对象"),
    _T("路径"),
};

double CDialogFrame::GetDpi()
{
    HDC hdc = ::GetDC(NULL);
    int t = ::GetDeviceCaps(hdc, DESKTOPHORZRES);
    int d = ::GetDeviceCaps(hdc, HORZRES);
    double fScale = ((double)t) / (double)d;

    ReleaseDC(NULL, hdc);
    return fScale;
}

LRESULT CDialogFrame::OnInitDialog(WPARAM wParam, LPARAM lParam)
{
    m_fScale = GetDpi();

    m_hHideWnd = CreateWindow(
        _T("Static"), _T("Hide"), WS_CHILDWINDOW, 0, 0, 0, 0,
        m_hWnd, NULL, GetModuleHandle(NULL), NULL);
    if (m_hHideWnd)
    {
        ::SetParent(m_hHideWnd, NULL);
    }

    m_hMainMenu = ::LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_MENU));
    if (m_hMainMenu)
    {
        ShowTrayIcon();
    }

    ::SetMenu(m_hWnd, m_hMainMenu);

    HICON hIconSmall = ::LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON));
    HICON hIconBig = ::LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON));
    if (hIconBig)
    {
        ::SendMessage(m_hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIconBig);
    }

    if (hIconSmall)
    {
        ::SendMessage(m_hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIconSmall);
    }

    CHANGEFILTERSTRUCT cfs = { 0 };
    cfs.cbSize = sizeof(cfs);

    //允许低权限进程拖动文件进来
    ChangeWindowMessageFilterEx(m_hWnd, WM_DROPFILES, MSGFLT_ALLOW, &cfs);
    ChangeWindowMessageFilterEx(m_hWnd, WM_COPYDATA, MSGFLT_ALLOW, &cfs);
    ChangeWindowMessageFilterEx(m_hWnd, WM_COPYDATA - 1, MSGFLT_ALLOW, &cfs);

    if (m_cfg.GetInt(_T("config"), _T("topmost"), true))
    {
        PostCommand(ID_SETTINGS_TOPMOST);
    }

    // 列表配置
    m_nActiveCol = m_cfg.GetInt(_T("list_config"), _T("active_col"), 1);
    m_nSortCol = m_cfg.GetInt(_T("list_config"), _T("sort_col"), 1);
    m_fDesc = m_cfg.GetInt(_T("list_config"), _T("sort_desc"), 0);

    SetItemString(IDC_EDIT_EXE_PATH_INJECT, m_cfg.GetString(_T("inject_config"), _T("exe_path")));
    SetItemString(IDC_EDIT_DLL_PATH_INJECT, m_cfg.GetString(_T("inject_config"), _T("dll_path")));
    SetItemInt(IDC_EDIT_PID_INJECT, m_cfg.GetInt(_T("inject_config"), _T("pid"), 0));
    SetItemInt(IDC_EDIT_PID_FIND, m_cfg.GetInt(_T("inject_config"), _T("pid_find"), 0));
    SetItemString(IDC_EDIT_NAME_INJECT, m_cfg.GetString(_T("inject_config"), _T("name")));

    bool fAutoRefreshEnable = m_cfg.GetInt(_T("inject_config"), _T("auto_refresh_enable"), 0);
    CheckButton(IDC_CHECK_AUTO_REFRESH, fAutoRefreshEnable);

    EnableRadioButton(IDC_RADIO_AUTO_REFRESH_0, IDC_RADIO_AUTO_REFRESH_2, m_cfg.GetInt(_T("inject_config"), _T("auto_refresh_enable"), 0));
    m_autoRefreshId = IDC_RADIO_AUTO_REFRESH_0 + m_cfg.GetInt(_T("inject_config"), _T("auto_refresh_speed"), 1);
    CheckRadioButton(IDC_RADIO_AUTO_REFRESH_0, IDC_RADIO_AUTO_REFRESH_2, m_autoRefreshId);

    if (fAutoRefreshEnable)
    {
        OnRadioRefreshSpeed(0, m_autoRefreshId, 0, IDC_RADIO_AUTO_REFRESH_0, IDC_RADIO_AUTO_REFRESH_2);
    }

    SetItemString(IDC_EDIT_EXE_PATH_HOOK, m_cfg.GetString(_T("hook_config"), _T("exe_path")));
    SetItemString(IDC_EDIT_DLL_PATH_HOOK, m_cfg.GetString(_T("hook_config"), _T("dll_path")));
    SetItemInt(IDC_EDIT_PID_HOOK, m_cfg.GetInt(_T("hook_config"), _T("pid"), 0));
    SetItemInt(IDC_EDIT_TID_HOOK, m_cfg.GetInt(_T("hook_config"), _T("tid"), 0));
    SetItemString(IDC_EDIT_HWND, m_cfg.GetString(_T("hook_config"), _T("hwnd")));
    SetItemString(IDC_EDIT_CAPTION, m_cfg.GetString(_T("hook_config"), _T("caption")));
    SetItemString(IDC_EDIT_CLASS, m_cfg.GetString(_T("hook_config"), _T("class")));
    SetItemString(IDC_EDIT_RECT, m_cfg.GetString(_T("hook_config"), _T("rect")));

    InitListView();

    PostCommand(IDC_BUTTON_REFRESH);

    _tstring strColWidth = m_cfg.GetString(_T("list_config"), _T("col_width"));
    std::vector<_tstring> vWidth = CStrUtils::SplitStr(strColWidth, _T(","));
    for (int i = 0; i < vWidth.size(); i++)
    {
        int nWidth = _tcstol(vWidth[i].c_str(), NULL, 10);
        if (0 != nWidth)
        {
            ListView_SetColumnWidth(m_hListView, i, nWidth);
        }
    }

    int iOrder[_countof(lpColName)] = { 0 };
    _tstring strColOrder = m_cfg.GetString(_T("list_config"), _T("col_order"));
    std::vector<_tstring> vOrder = CStrUtils::SplitStr(strColOrder, _T(","));
    for (int i = 0; i < vOrder.size(); i++)
    {
        iOrder[i] = _tcstol(vOrder[i].c_str(), NULL, 10);
    }

    if (!vOrder.empty())
    {
        ListView_SetColumnOrderArray(m_hListView, _countof(iOrder), iOrder);
    }

    PSHARE_DATA_PROTO pShareData = HookGetShareData();
    if (pShareData)
    {
        CheckButton(IDC_BUTTON_XBOX_ENABLE_0, HookIsEnableXinput(0));
        CheckButton(IDC_BUTTON_XBOX_ENABLE_1, HookIsEnableXinput(1));
        CheckButton(IDC_BUTTON_XBOX_ENABLE_2, HookIsEnableXinput(2));
        CheckButton(IDC_BUTTON_XBOX_ENABLE_3, HookIsEnableXinput(3));
    }

    m_taskInput = std::move(std::thread([this]() {
        HookUninitialize();
        GetXboxInput();
        }
    )
    );

#ifdef _WIN64
    _tstring strPipeName = _T("Dll_Hook_x86");
    m_hookPipe.Create(strPipeName.c_str());
    _tstring strWorkDir = CPathUtils::GetCurrentModuleDir();
    _tstring strCommand = strWorkDir + _T("\\") + _T("Dll_Hook_x86.exe") + _T(" ") + strPipeName;
    CProcessExUtils::CreateProcessNormal(_T(""), strCommand, strWorkDir, false, false);
#endif

    return TRUE;
}

void CDialogFrame::GetXboxInput()
{
    PSHARE_DATA_PROTO pConfig = HookGetShareData();
    XINPUT_STATE LastState[XUSER_MAX_COUNT] = { 0 };
    uint64_t dwPacketCount = 0;

    uint64_t lastTickCount = ::GetTickCount64();
    uint64_t curTickCount = ::GetTickCount64();

    DWORD dwPacketNumber[XUSER_MAX_COUNT] = { 0 };

    // 混合输入
    while (!m_fHookQuit)
    {
        if (!pConfig->fHook)
        {
            ::Sleep(10);
            continue;
        }

        XINPUT_STATE CurState[XUSER_MAX_COUNT] = { 0 };
        XINPUT_STATE MergeState = { 0 };

        int32_t nThumbLX = 0;
        int32_t nThumbLY = 0;
        int32_t nThumbRX = 0;
        int32_t nThumbRY = 0;
        uint32_t nLeftTrigger = 0;
        uint32_t nRightTrigger = 0;

        curTickCount = ::GetTickCount64();

        for (DWORD i = 0; i < XUSER_MAX_COUNT; i++)
        {
            dwPacketNumber[i]++;
            DWORD dwResult = XInputGetState(i, &CurState[i]);
            MergeState.dwPacketNumber = CurState[i].dwPacketNumber;

            if (pConfig->HoldA[i]) CurState[i].Gamepad.wButtons |= XINPUT_GAMEPAD_A;
            if (pConfig->HoldB[i]) CurState[i].Gamepad.wButtons |= XINPUT_GAMEPAD_B;
            if (pConfig->HoldX[i]) CurState[i].Gamepad.wButtons |= XINPUT_GAMEPAD_X;
            if (pConfig->HoldY[i]) CurState[i].Gamepad.wButtons |= XINPUT_GAMEPAD_Y;
            if (pConfig->HoldLB[i]) CurState[i].Gamepad.wButtons |= XINPUT_GAMEPAD_LEFT_SHOULDER;
            if (pConfig->HoldRB[i]) CurState[i].Gamepad.wButtons |= XINPUT_GAMEPAD_RIGHT_SHOULDER;

            if (curTickCount - lastTickCount >= 100)
            {
                if (pConfig->BurstA[i]) CurState[i].Gamepad.wButtons = LastState[i].Gamepad.wButtons ^ XINPUT_GAMEPAD_A;
                if (pConfig->BurstB[i]) CurState[i].Gamepad.wButtons = LastState[i].Gamepad.wButtons ^ XINPUT_GAMEPAD_B;
                if (pConfig->BurstX[i]) CurState[i].Gamepad.wButtons = LastState[i].Gamepad.wButtons ^ XINPUT_GAMEPAD_X;
                if (pConfig->BurstY[i]) CurState[i].Gamepad.wButtons = LastState[i].Gamepad.wButtons ^ XINPUT_GAMEPAD_Y;
                if (pConfig->BurstLB[i]) CurState[i].Gamepad.wButtons = LastState[i].Gamepad.wButtons ^ XINPUT_GAMEPAD_LEFT_SHOULDER;
                if (pConfig->BurstRB[i]) CurState[i].Gamepad.wButtons = LastState[i].Gamepad.wButtons ^ XINPUT_GAMEPAD_RIGHT_SHOULDER;
                LastState[i] = CurState[i];
                lastTickCount = ::GetTickCount64();
            }

            MergeState.Gamepad.wButtons |= CurState[i].Gamepad.wButtons;
            nThumbLX += CurState[i].Gamepad.sThumbLX;
            nThumbLY += CurState[i].Gamepad.sThumbLY;
            nThumbRX += CurState[i].Gamepad.sThumbRX;
            nThumbRY += CurState[i].Gamepad.sThumbRY;
            nLeftTrigger += CurState[i].Gamepad.bLeftTrigger;
            nRightTrigger += CurState[i].Gamepad.bRightTrigger;

        }

        // 扳机有效范围限定
        if (nLeftTrigger > UINT8_MAX) nLeftTrigger = UINT8_MAX;
        if (nRightTrigger > UINT8_MAX) nRightTrigger = UINT8_MAX;

        // 左摇杆轴有效范围限定
        if (nThumbLX < INT16_MIN) nThumbLX = INT16_MIN;
        if (nThumbLY < INT16_MIN) nThumbLY = INT16_MIN;
        if (nThumbRX < INT16_MIN) nThumbRX = INT16_MIN;
        if (nThumbRY < INT16_MIN) nThumbRY = INT16_MIN;

        // 右摇杆轴有效范围限定
        if (nThumbLX > INT16_MAX) nThumbLX = INT16_MAX;
        if (nThumbLY > INT16_MAX) nThumbLY = INT16_MAX;
        if (nThumbRX > INT16_MAX) nThumbRX = INT16_MAX;
        if (nThumbRY > INT16_MAX) nThumbRY = INT16_MAX;

        MergeState.Gamepad.bLeftTrigger = nLeftTrigger;
        MergeState.Gamepad.bRightTrigger = nRightTrigger;

        MergeState.Gamepad.sThumbLX = nThumbLX;
        MergeState.Gamepad.sThumbLY = nThumbLY;
        MergeState.Gamepad.sThumbRX = nThumbRX;
        MergeState.Gamepad.sThumbRY = nThumbRY;

        // 混合输入
        for (int i = 0; i < XUSER_MAX_COUNT; i++)
        {
            if (pConfig->fEnable[i])
            {
                if (pConfig->fMerge)
                {
                    memcpy(&pConfig->XinputState[i], &MergeState, sizeof(XINPUT_STATE));
                }
                else
                {
                    memcpy(&pConfig->XinputState[i], &CurState[i], sizeof(XINPUT_STATE));
                }
            }
            else
            {
                memset(&pConfig->XinputState[i], 0, sizeof(XINPUT_STATE));
            }

            pConfig->XinputState[i].dwPacketNumber = dwPacketNumber[i];
        }

        dwPacketCount++;

        ::Sleep(10);
    }
}

LRESULT CDialogFrame::OnXboxHookEnable(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd)
{
    PSHARE_DATA_PROTO pHookConfig = HookGetShareData();
    if (pHookConfig)
    {
        pHookConfig->fHook = IsButtonChecked(wID);
    }

    return (LRESULT)TRUE;
}

LRESULT CDialogFrame::OnXboxEnable(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd)
{
    PSHARE_DATA_PROTO pHookConfig = HookGetShareData();
    if (pHookConfig)
    {
        pHookConfig->fEnable[wID - wStart] = IsButtonChecked(wID);
    }

    return (LRESULT)TRUE;
}

LRESULT CDialogFrame::OnXboxMerge(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd)
{
    PSHARE_DATA_PROTO pHookConfig = HookGetShareData();
    if (pHookConfig)
    {
        pHookConfig->fMerge = IsButtonChecked(wID);
    }

    return (LRESULT)TRUE;
}

LRESULT CDialogFrame::OnXboxHoldA(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd)
{
    PSHARE_DATA_PROTO pHookConfig = HookGetShareData();
    if (pHookConfig)
    {
        for (int i = 0; i < XUSER_MAX_COUNT; i++)
        {
            pHookConfig->HoldA[i] = IsButtonChecked(wID);
        }
    }

    return (LRESULT)TRUE;
}

LRESULT CDialogFrame::OnXboxHoldB(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd)
{
    PSHARE_DATA_PROTO pHookConfig = HookGetShareData();
    if (pHookConfig)
    {
        for (int i = 0; i < XUSER_MAX_COUNT; i++)
        {
            pHookConfig->HoldB[i] = IsButtonChecked(wID);
        }
    }

    return (LRESULT)TRUE;
}

LRESULT CDialogFrame::OnXboxHoldX(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd)
{
    PSHARE_DATA_PROTO pHookConfig = HookGetShareData();
    if (pHookConfig)
    {
        for (int i = 0; i < XUSER_MAX_COUNT; i++)
        {
            pHookConfig->HoldX[i] = IsButtonChecked(wID);
        }
    }

    return (LRESULT)TRUE;
}

LRESULT CDialogFrame::OnXboxHoldY(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd)
{
    PSHARE_DATA_PROTO pHookConfig = HookGetShareData();
    if (pHookConfig)
    {
        for (int i = 0; i < XUSER_MAX_COUNT; i++)
        {
            pHookConfig->HoldY[i] = IsButtonChecked(wID);
        }
    }

    return (LRESULT)TRUE;
}

LRESULT CDialogFrame::OnXboxHoldLB(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd)
{
    PSHARE_DATA_PROTO pHookConfig = HookGetShareData();
    if (pHookConfig)
    {
        for (int i = 0; i < XUSER_MAX_COUNT; i++)
        {
            pHookConfig->HoldLB[i] = IsButtonChecked(wID);
        }
    }

    return (LRESULT)TRUE;
}

LRESULT CDialogFrame::OnXboxHoldRB(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd)
{
    PSHARE_DATA_PROTO pHookConfig = HookGetShareData();
    if (pHookConfig)
    {
        for (int i = 0; i < XUSER_MAX_COUNT; i++)
        {
            pHookConfig->HoldRB[i] = IsButtonChecked(wID);
        }
    }

    return (LRESULT)TRUE;
}

LRESULT CDialogFrame::OnXboxBurstA(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd)
{
    PSHARE_DATA_PROTO pHookConfig = HookGetShareData();
    if (pHookConfig)
    {
        for (int i = 0; i < XUSER_MAX_COUNT; i++)
        {
            pHookConfig->BurstA[i] = IsButtonChecked(wID);
        }
    }

    return (LRESULT)TRUE;
}

LRESULT CDialogFrame::OnXboxBurstB(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd)
{
    PSHARE_DATA_PROTO pHookConfig = HookGetShareData();
    if (pHookConfig)
    {
        for (int i = 0; i < XUSER_MAX_COUNT; i++)
        {
            pHookConfig->BurstB[i] = IsButtonChecked(wID);
        }
    }

    return (LRESULT)TRUE;
}

LRESULT CDialogFrame::OnXboxBurstX(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd)
{
    PSHARE_DATA_PROTO pHookConfig = HookGetShareData();
    if (pHookConfig)
    {
        for (int i = 0; i < XUSER_MAX_COUNT; i++)
        {
            pHookConfig->BurstX[i] = IsButtonChecked(wID);
        }
    }

    return (LRESULT)TRUE;
}

LRESULT CDialogFrame::OnXboxBurstY(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd)
{
    PSHARE_DATA_PROTO pHookConfig = HookGetShareData();
    if (pHookConfig)
    {
        for (int i = 0; i < XUSER_MAX_COUNT; i++)
        {
            pHookConfig->BurstY[i] = IsButtonChecked(wID);
        }
    }

    return (LRESULT)TRUE;
}

LRESULT CDialogFrame::OnXboxBurstLB(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd)
{
    PSHARE_DATA_PROTO pHookConfig = HookGetShareData();
    if (pHookConfig)
    {
        for (int i = 0; i < XUSER_MAX_COUNT; i++)
        {
            pHookConfig->BurstLB[i] = IsButtonChecked(wID);
        }
    }

    return (LRESULT)TRUE;
}

LRESULT CDialogFrame::OnXboxBurstRB(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd)
{
    PSHARE_DATA_PROTO pHookConfig = HookGetShareData();
    if (pHookConfig)
    {
        for (int i = 0; i < XUSER_MAX_COUNT; i++)
        {
            pHookConfig->BurstRB[i] = IsButtonChecked(wID);
        }
    }

    return (LRESULT)TRUE;
}

LRESULT CDialogFrame::OnClose(WPARAM wParam, LPARAM lParam)
{
    m_fQuit = true;

    if (m_taskInput.joinable())
    {
        m_fHookQuit = true;
        m_taskInput.join();
        m_fHookQuit = false;
    }

    HookUnload();

    // 列表配置
    int iOrder[_countof(lpColName)] = { 0 };
    BOOL fResult = ListView_GetColumnOrderArray(m_hListView, _countof(lpColName), iOrder);

    _tstring strColOrder;
    int nSize = _countof(lpColName);
    for (int i = 0; i < _countof(lpColName); i++)
    {
        strColOrder += CStrUtils::Format(_T("%d"), iOrder[i]);
        nSize--;
        if (0 != nSize)
        {
            strColOrder += _T(",");
        }
    }

    _tstring strColWidth;
    nSize = _countof(lpColName);
    for (int i = 0; i < _countof(lpColName); i++)
    {
        strColWidth += CStrUtils::Format(_T("%d"), ListView_GetColumnWidth(m_hListView, i));
        nSize--;
        if (0 != nSize)
        {
            strColWidth += _T(",");
        }
    }

    m_cfg.SetString(_T("list_config"), _T("col_width"), strColWidth);
    m_cfg.SetString(_T("list_config"), _T("col_order"), strColOrder);

    m_cfg.SetInt(_T("list_config"), _T("active_col"), m_nActiveCol);
    m_cfg.SetInt(_T("list_config"), _T("sort_col"), m_nSortCol);
    m_cfg.SetInt(_T("list_config"), _T("sort_desc"), m_fDesc);


    m_cfg.SetString(_T("inject_config"), _T("exe_path"), GetItemString(IDC_EDIT_EXE_PATH_INJECT));
    m_cfg.SetString(_T("inject_config"), _T("dll_path"), GetItemString(IDC_EDIT_DLL_PATH_INJECT));
    m_cfg.SetInt(_T("inject_config"), _T("pid"), GetItemInt(IDC_EDIT_PID_INJECT));
    m_cfg.SetInt(_T("inject_config"), _T("pid_find"), GetItemInt(IDC_EDIT_PID_FIND));
    m_cfg.SetString(_T("inject_config"), _T("name"), GetItemString(IDC_EDIT_NAME_INJECT));
    m_cfg.SetInt(_T("inject_config"), _T("auto_refresh_enable"), IsButtonChecked(IDC_CHECK_AUTO_REFRESH));
    m_cfg.SetInt(_T("inject_config"), _T("auto_refresh_speed"), m_autoRefreshId - IDC_RADIO_AUTO_REFRESH_0);

    m_cfg.SetString(_T("hook_config"), _T("exe_path"), GetItemString(IDC_EDIT_EXE_PATH_HOOK));
    m_cfg.SetString(_T("hook_config"), _T("dll_path"), GetItemString(IDC_EDIT_DLL_PATH_HOOK));
    m_cfg.SetInt(_T("hook_config"), _T("pid"), GetItemInt(IDC_EDIT_PID_HOOK));
    m_cfg.SetInt(_T("hook_config"), _T("tid"), GetItemInt(IDC_EDIT_TID_HOOK));
    m_cfg.SetString(_T("hook_config"), _T("hwnd"), GetItemString(IDC_EDIT_HWND));
    m_cfg.SetString(_T("hook_config"), _T("caption"), GetItemString(IDC_EDIT_CAPTION));
    m_cfg.SetString(_T("hook_config"), _T("class"), GetItemString(IDC_EDIT_CLASS));
    m_cfg.SetString(_T("hook_config"), _T("rect"), GetItemString(IDC_EDIT_RECT));

    return (LRESULT)FALSE;
}

LRESULT CDialogFrame::OnDisplayChange(WPARAM wParam, LPARAM lParam)
{
    m_fScale = GetDpi();
    return (LRESULT)FALSE;
}

LRESULT CDialogFrame::OnTimer(WPARAM wParam, LPARAM lParam)
{
    ListViewUpdate();
    return TRUE;
}

LRESULT CDialogFrame::OnDropFiles(WPARAM wParam, LPARAM lParam)
{
    HDROP hDrop = (HDROP)wParam;
    TCHAR szFile[MAX_PATH] = { 0 };
    UINT nCount = DragQueryFile(hDrop, (UINT)(-1), szFile, _countof(szFile));
    bool fInInject = false;
    bool fInHook = false;

    POINT ptCursor = { 0 };
    ::GetCursorPos(&ptCursor);

    {
        RECT rtTrack = { 0 };
        ::GetWindowRect(GetItem(IDC_STATIC_INJECT), &rtTrack);
        if (::PtInRect(&rtTrack, ptCursor))
        {
            fInInject = true;
        }
    }

    {
        RECT rtTrack = { 0 };
        ::GetWindowRect(GetItem(IDC_STATIC_HOOK), &rtTrack);
        if (::PtInRect(&rtTrack, ptCursor))
        {
            fInHook = true;
        }
    }

    _tstring strCurDir = CPathUtils::GetCurrentModuleDir();
    for (UINT i = 0; i < nCount; i++)
    {
        ::DragQueryFile(hDrop, i, szFile, _countof(szFile));
        if (_T("dll") == CPathUtils::GetFileExt(szFile))
        {
            if (fInInject)
            {
                SetItemString(IDC_EDIT_DLL_PATH_INJECT, szFile);
            }

            if (fInHook)
            {
                SetItemString(IDC_EDIT_DLL_PATH_HOOK, szFile);
            }

            break;
        }
    }

    DragFinish(hDrop);

    return TRUE;
}

LRESULT CDialogFrame::OnCommandExit(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd)
{
    EndDialog(IDOK);
    return (LRESULT)TRUE;
}

LRESULT CDialogFrame::OnTopmost(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd)
{
    HMENU hMenu = GetMenu(m_hWnd);
    bool fChecked = IsMenuItemChecked(hMenu, wID, false);
    fChecked = !fChecked;
    CheckMenuItem(hMenu, wID, fChecked, false);

    if (fChecked)
    {
        ::SetWindowPos(m_hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }
    else
    {
        ::SetWindowPos(m_hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }

    m_cfg.SetInt(_T("config"), _T("topmost"), fChecked);
    return (LRESULT)TRUE;
}

LRESULT CDialogFrame::OnRefresh(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd)
{
    ListViewUpdate();
    return (LRESULT)TRUE;
}

LRESULT CDialogFrame::OnFindPidInject(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd)
{
    _tstring strName = GetItemString(IDC_EDIT_PID_FIND);
    DWORD dwPID = GetItemInt(IDC_EDIT_PID_FIND);

    if (strName.empty())
    {
        return (LRESULT)TRUE;
    }

    ListViewUpdate();

    {
        size_t index = 0;
        for (const auto& item : m_procInfos)
        {
            if (item.UniqueProcessId == dwPID)
            {
                ListView_EnsureVisible(m_hListView, index, FALSE);
                ListView_SetItemState(m_hListView, index, LVIS_SELECTED, LVIS_SELECTED);
                break;
            }
            index++;
        }
    }

    return (LRESULT)TRUE;
}

LRESULT CDialogFrame::OnFindNameInject(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd)
{
    _tstring strName = GetItemString(IDC_EDIT_NAME_INJECT);

    if (strName.empty())
    {
        return (LRESULT)TRUE;
    }

    ListViewUpdate();

    size_t index = 0;
    for (const auto& item : m_procInfos)
    {
        if (_tstring::npos != CStrUtils::FindNoCase(item.ImageName, strName))
        {
            ListView_EnsureVisible(m_hListView, index, FALSE);
            ListView_SetItemState(m_hListView, index, LVIS_SELECTED, LVIS_SELECTED);
            break;
        }
        index++;
    }

    return (LRESULT)TRUE;
}

void CDialogFrame::ListViewUpdate()
{
    static CProcessExUtils::PROCESS_INFO_LIST sInfos;

    static bool fFirst = true;

    if (fFirst)
    {
        fFirst = false;

        std::thread([this]() {

            while (!m_fQuit)
            {
                CProcessExUtils::PROCESS_INFO_LIST infos = CProcessExUtils::GetProcessList();
                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    sInfos = infos;
                }
            }
            }).detach();
    }

    CProcessExUtils::PROCESS_INFO_LIST infos;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        infos = sInfos;
    }

    m_procInfos.resize(infos.size());

    size_t index = 0;
    for (auto& item : infos)
    {
        m_procInfos[index++] = std::move(item.second);
    }

    ListViewSort(m_nSortCol);
    ListViewUpdateHeader();
    ListView_SetItemCountEx(m_hListView, infos.size(), LVSICF_NOINVALIDATEALL | LVSICF_NOSCROLL);
    ListViewUpdateVisiableItem();
}

LRESULT CDialogFrame::OnDllPathInject(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd)
{
    static bool fBusy = false;

    if (fBusy)
    {
        return (LRESULT)TRUE;
    }

    fBusy = true;
    std::thread([this]() {

        static _tstring strPath;
        UINT uIndex = 0;
        COMDLG_FILTERSPEC filters[] = {
            { _T("动态链接库"), _T("*.dll") },
        };

        if (strPath.empty())
        {
            strPath = CPathUtils::GetCurrentModuleDir();
        }

        if (CComFileDlg::GetOpenPath(m_hWnd, strPath, uIndex, filters, _countof(filters)))
        {
            SetItemString(IDC_EDIT_DLL_PATH_INJECT, strPath);
        }

        fBusy = false;
        }
    ).detach();

    return (LRESULT)TRUE;
}

LRESULT CDialogFrame::OnLoad(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd)
{
    _tstring strPath = GetItemString(IDC_EDIT_DLL_PATH_INJECT);
    DWORD dwPID = GetItemInt(IDC_EDIT_PID_INJECT);

    if (!InjectUtils::LoadByRemoteThread(dwPID, strPath))
    {
        MessageBox(m_hWnd, _T("加载失败!"), _T("提示"), MB_OK | MB_ICONERROR);
    }

    return (LRESULT)TRUE;
}

LRESULT CDialogFrame::OnUnload(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd)
{
    _tstring strPath = GetItemString(IDC_EDIT_DLL_PATH_INJECT);
    DWORD dwPID = GetItemInt(IDC_EDIT_PID_INJECT);

    if (!InjectUtils::UnloadByRemoteThread(dwPID, strPath))
    {
        MessageBox(m_hWnd, _T("卸载失败!"), _T("提示"), MB_OK | MB_ICONERROR);
    }

    return (LRESULT)TRUE;
}

LRESULT CDialogFrame::OnDllPathHook(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd)
{
    static bool fBusy = false;

    if (fBusy)
    {
        return (LRESULT)TRUE;
    }

    fBusy = true;
    std::thread([this]() {

        static _tstring strPath;
        UINT uIndex = 0;
        COMDLG_FILTERSPEC filters[] = {
            { _T("动态链接库"), _T("*.dll") },
        };

        if (strPath.empty())
        {
            strPath = CPathUtils::GetCurrentModuleDir();
        }

        if (CComFileDlg::GetOpenPath(m_hWnd, strPath, uIndex, filters, _countof(filters)))
        {
            SetItemString(IDC_EDIT_DLL_PATH_HOOK, strPath);
        }

        fBusy = false;
        }
    ).detach();

    return (LRESULT)TRUE;
}

LRESULT CDialogFrame::OnHookLoad(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd)
{
    _tstring strPath = GetItemString(IDC_EDIT_DLL_PATH_HOOK);
    DWORD dwThreadID = GetItemInt(IDC_EDIT_TID_HOOK);

    do
    {
        if (!HookLoad(dwThreadID))
        {

#ifdef _WIN64
            PIPE_DATA_PROTO proto;
            proto.uCmd = ePipeCmd::eHook;
            proto.uParam = dwThreadID;
            m_hookPipe.Write(&proto, sizeof(proto), nullptr, 3000);
#endif
            MessageBox(m_hWnd, _T("安装 HOOK 失败!"), _T("提示"), MB_OK | MB_ICONERROR);
            break;
        }

        ::EnableWindow(GetItem(IDC_BUTTON_HOOK_UNLOAD), TRUE);
        ::EnableWindow(GetItem(IDC_BUTTON_HOOK_LOAD), FALSE);

        PSHARE_DATA_PROTO pShareData = HookGetShareData();
        if (pShareData)
        {
            CheckButton(IDC_BUTTON_XBOX_ENABLE_0, HookIsEnableXinput(0));
            CheckButton(IDC_BUTTON_XBOX_ENABLE_1, HookIsEnableXinput(1));
            CheckButton(IDC_BUTTON_XBOX_ENABLE_2, HookIsEnableXinput(2));
            CheckButton(IDC_BUTTON_XBOX_ENABLE_3, HookIsEnableXinput(3));
        }

    } while (false);

    return (LRESULT)TRUE;
}

LRESULT CDialogFrame::OnHookUnload(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd)
{
    _tstring strPath = GetItemString(IDC_EDIT_DLL_PATH_HOOK);

    do
    {
        if (!HookUnload())
        {
#ifdef _WIN64
            PIPE_DATA_PROTO proto;
            proto.uCmd = ePipeCmd::eUnHook;
            m_hookPipe.Write(&proto, sizeof(proto), nullptr, 3000);
#endif
            MessageBox(m_hWnd, _T("卸载 HOOK 失败!"), _T("提示"), MB_OK | MB_ICONERROR);
        }

        HookEnableXinput(0, true);
        HookEnableXinput(1, true);
        HookEnableXinput(2, true);
        HookEnableXinput(3, true);
        ::EnableWindow(GetItem(IDC_BUTTON_HOOK_UNLOAD), FALSE);
        ::EnableWindow(GetItem(IDC_BUTTON_HOOK_LOAD), TRUE);
    } while (false);

    return (LRESULT)TRUE;
}

LRESULT CDialogFrame::OnCheckAutoRefresh(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd)
{
    BOOL fChecked = IsButtonChecked(wID);

    if (fChecked)
    {
        EnableRadioButton(IDC_RADIO_AUTO_REFRESH_0, IDC_RADIO_AUTO_REFRESH_2, true);
        ::KillTimer(m_hWnd, AUTO_REFRESH_TIMER_ID);
        ::SetTimer(m_hWnd, AUTO_REFRESH_TIMER_ID, 500 + (m_autoRefreshId - IDC_RADIO_AUTO_REFRESH_0) * 500, NULL);
    }
    else
    {
        EnableRadioButton(IDC_RADIO_AUTO_REFRESH_0, IDC_RADIO_AUTO_REFRESH_2, false);
        ::KillTimer(m_hWnd, AUTO_REFRESH_TIMER_ID);
    }

    return (LRESULT)TRUE;
}

LRESULT CDialogFrame::OnRadioRefreshSpeed(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd)
{
    CheckRadioButton(wStart, wEnd, wID);

    m_autoRefreshId = wID;

    if (::IsWindowEnabled(GetItem(wID)))
    {
        ::KillTimer(m_hWnd, AUTO_REFRESH_TIMER_ID);
        ::SetTimer(m_hWnd, AUTO_REFRESH_TIMER_ID, 500 + (wID - wStart) * 500, NULL);
    }

    return (LRESULT)TRUE;
}

LRESULT CDialogFrame::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    LONG xPos = GET_X_LPARAM(lParam); 
    LONG yPos = GET_Y_LPARAM(lParam);

    RECT rtTrack = { 0 };
    ::GetWindowRect(GetItem(IDC_STATIC_CAPTURE), &rtTrack);

    POINT pt = { rtTrack.left, rtTrack.top };
    ::ScreenToClient(m_hWnd, &pt);

    LONG offsetX = pt.x - rtTrack.left;
    LONG offsetY = pt.y - rtTrack.top;
    ::OffsetRect(&rtTrack, offsetX, offsetY);

    if (::PtInRect(&rtTrack, { xPos, yPos }))
    {
        m_fTrack = true;
        ::SetCapture(m_hWnd);

        HCURSOR hCursor = ::LoadCursor(GetModuleHandle(NULL), MAKEINTRESOURCE(IDC_CURSOR));
        ::SetCursor(hCursor);

        HICON hIcon = ::LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON_CAPTURE_YES));
        ::SendDlgItemMessage(m_hWnd, IDC_STATIC_CAPTURE, STM_SETIMAGE, IMAGE_ICON, (LONG)hIcon);
    }

    return (LRESULT)TRUE;
}

LRESULT CDialogFrame::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    if (m_fTrack)
    {
        m_fTrack = false;
        ::ReleaseCapture();
        ::SetCursor(NULL);
        HICON hIcon = ::LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON_CAPTURE_NO));
        ::SendDlgItemMessage(m_hWnd, IDC_STATIC_CAPTURE, STM_SETIMAGE, IMAGE_ICON, (LONG)hIcon);
    }

    return (LRESULT)TRUE;
}

LRESULT CDialogFrame::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
    LONG xPos = GET_X_LPARAM(lParam); 
    LONG yPos = GET_Y_LPARAM(lParam);

    do
    {
        if (!m_fTrack)
        {
            break;
        }

        POINT pt = { xPos, yPos };
        ::ClientToScreen(m_hWnd, &pt);

        HWND hWnd = ::WindowFromPoint(pt);
        if (!hWnd)
        {
            break;
        }

        DWORD dwPID = 0;
        DWORD dwTID = ::GetWindowThreadProcessId(hWnd, &dwPID);
        if (::GetCurrentProcessId() == dwPID)
        {
            break;
        }

        _tstring strPath = CProcessExUtils::GetPath(dwPID);
        SetItemInt(IDC_EDIT_PID_HOOK, dwPID, false);
        SetItemInt(IDC_EDIT_TID_HOOK, dwTID, false);
        SetItemString(IDC_EDIT_EXE_PATH_HOOK, strPath);

        TCHAR szBuf[MAX_PATH] = { 0 };
        ::GetClassName(hWnd, szBuf, _countof(szBuf));
        SetItemString(IDC_EDIT_CLASS, szBuf);
        ::GetWindowText(hWnd, szBuf, _countof(szBuf));
        SetItemString(IDC_EDIT_CAPTION, szBuf);
        ::StringCchPrintf(szBuf, _countof(szBuf), _T("0x%08X"), hWnd);
        SetItemString(IDC_EDIT_HWND, szBuf);

        RECT rtWnd = { 0 };
        ::GetWindowRect(hWnd, &rtWnd);

        rtWnd.left = (double)rtWnd.left * m_fScale;
        rtWnd.top = (double)rtWnd.top * m_fScale;
        rtWnd.right = (double)rtWnd.right * m_fScale;
        rtWnd.bottom = (double)rtWnd.bottom * m_fScale;
        
        ::StringCchPrintf(szBuf, _countof(szBuf), _T("(%d,%d) - (%d,%d) %d * %d"),
            rtWnd.left, rtWnd.top,
            rtWnd.right, rtWnd.bottom,
            rtWnd.right - rtWnd.left,
            rtWnd.bottom - rtWnd.top
        );

        SetItemString(IDC_EDIT_RECT, szBuf);


    } while (false);

    return (LRESULT)TRUE;
}

LRESULT CDialogFrame::OnTrayIcon(WPARAM wParam, LPARAM lParam)
{
    if (WM_LBUTTONDBLCLK == lParam)
    {
        ShowForeground(m_hWnd);
    }

    if (NULL != m_hHideWnd)
    {
        ::SetForegroundWindow(m_hHideWnd);
    }

    if (WM_RBUTTONUP == lParam)
    {
        HWND hForegroundWindow = GetForegroundWindow();
        if (hForegroundWindow == m_hHideWnd || hForegroundWindow == m_hWnd)
        {
            POINT pt = { 0 };
            ::GetCursorPos(&pt);
            CreatePopMenu(pt.x, pt.y, false);
        }
    }

    return FALSE;
}

bool CDialogFrame::ShowForeground(HWND hWnd)
{
    HWND hForeWnd = ::GetForegroundWindow();
    DWORD dwForeID = ::GetWindowThreadProcessId(hForeWnd, NULL);
    DWORD dwCurID = ::GetCurrentThreadId();
    ::AttachThreadInput(dwCurID, dwForeID, TRUE);
    BOOL isSuc = ::SetForegroundWindow(hWnd);
    ::AttachThreadInput(dwCurID, dwForeID, FALSE);

    if (isSuc)
    {
        //return true;
    }

    if (!::IsWindowVisible(hWnd))
    {
        ::ShowWindow(hWnd, SW_SHOW);
    }

    if (IsIconic(hWnd))
    {
        ::ShowWindow(hWnd, SW_SHOWNORMAL);
    }

    return ::SetForegroundWindow(hWnd);
}

void CDialogFrame::ShowTrayIcon()
{
    NOTIFYICONDATA nid = { 0 };
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = m_hWnd;
    nid.uID = 0;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = ::LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON));
    StringCchCopy(nid.szTip, _countof(nid.szTip), _T("Demo"));
    Shell_NotifyIcon(NIM_ADD, &nid);
}

void CDialogFrame::DeleteTrayIcon()
{
    NOTIFYICONDATA nid = { 0 };
    nid.cbSize = sizeof(nid);
    nid.hWnd = m_hWnd;
    nid.uID = 0;
    nid.uFlags = 0;
    Shell_NotifyIcon(NIM_DELETE, &nid);
}

void CDialogFrame::CreatePopMenu(int xPos, int yPos, bool isTop)
{
    MENUITEMINFO mii = { 0 };
    TCHAR szBuf[MAX_PATH] = { 0 };

    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_TYPE | MIIM_STATE | MIIM_SUBMENU | MIIM_ID;
    mii.fType = MFT_STRING | MFT_OWNERDRAW;
    mii.dwTypeData = szBuf;

    if (!m_hWnd)
    {
        return;
    }

    int nIndex = 0;

    if (NULL == m_hPopMenu)
    {
        m_hPopMenu = CreatePopupMenu();
        for (int i = 0; i < 1; i++)
        {
            mii.cch = _countof(szBuf);
            ::GetMenuItemInfo(GetSubMenu(m_hMainMenu, 1), i, TRUE, &mii);
            ::InsertMenuItem(m_hPopMenu, nIndex++, TRUE, &mii);
        }
        mii.cch = _countof(szBuf);
        ::GetMenuItemInfo(GetSubMenu(m_hMainMenu, 0), 0, TRUE, &mii);
        ::InsertMenuItem(m_hPopMenu, nIndex++, TRUE, &mii);
    }
    else
    {
        for (int i = 0; i < 1; i++)
        {
            mii.cch = _countof(szBuf);
            ::GetMenuItemInfo(GetSubMenu(m_hMainMenu, 1), i, TRUE, &mii);
            ::SetMenuItemInfo(m_hPopMenu, nIndex++, TRUE, &mii);
        }
        mii.cch = _countof(szBuf);
        ::GetMenuItemInfo(GetSubMenu(m_hMainMenu, 0), 0, TRUE, &mii);
        ::SetMenuItemInfo(m_hPopMenu, nIndex++, TRUE, &mii);
    }

    TrackPopupMenuEx(m_hPopMenu, TPM_LEFTALIGN | (isTop ? TPM_BOTTOMALIGN : TPM_TOPALIGN),
        xPos, yPos, m_hWnd, NULL);
}

void CDialogFrame::InitListView()
{
    m_hListView = GetItem(IDC_LIST_PROCESS);

    ULONG_PTR ulStyle = 0;
    ulStyle = GetWindowLongPtr(m_hListView, GWL_STYLE);
    ulStyle |= LVS_SHOWSELALWAYS;
    SetWindowLongPtr(m_hListView, GWL_STYLE, ulStyle);

    DWORD dwExStyle =
        //LVS_EX_CHECKBOXES |         //为列表视图控件中的项启用检查框 
        LVS_EX_GRIDLINES |          //显示项和子项周围的网格线
        LVS_EX_FULLROWSELECT |      //选中某个项后，将突出显示该项及其所有子项
        LVS_EX_DOUBLEBUFFER |       //通过双缓冲进行绘制，从而减少闪烁
        LVS_EX_FLATSB |             //在列表视图中启用平面滚动条
        LVS_EX_HEADERDRAGDROP |     //启用列表视图控件中列的拖放重新排序
        LVS_EX_BORDERSELECT |   // 选择项目时更改边框颜色，而不是突出显示该项。
        LVS_EX_HEADERINALLVIEWS |   //在所有视图模式下显示列标题
        //LVS_EX_AUTOCHECKSELECT;     //单击一下即可自动选择检查框
        0 ;

    // 设置扩展样式
    ListView_SetExtendedListViewStyleEx(m_hListView, dwExStyle, dwExStyle);

    ListView_SetTextBkColor(m_hListView, CLR_NONE);
    ListView_SetTextColor(m_hListView, RGB(0, 0, 0));

    //初始化表头
    {
        LVCOLUMN lvc = { 0 };
        int nCumWidth[] = {
            48,
            56,
            64,
            128,
            56,
            128,
            128,
            48,
            48,
            64,
            64,
            1024,
        };

        //设置列文本
        for (int i = 0; i < _countof(lpColName); i++)
        {
            lvc.mask = LVCF_TEXT | LVCF_FMT | LVCF_WIDTH;
            lvc.pszText = (LPTSTR)lpColName[i];
            lvc.iSubItem = i;
            lvc.cx = nCumWidth[i] + 32;
            lvc.iImage = 1;

            ListView_InsertColumn(m_hListView, i, &lvc);
        }
    }
}

LRESULT CDialogFrame::ListViewRowClick(WORD wID, LPNMHDR pNmhdr)
{
    LPNMITEMACTIVATE lpNmItem = (LPNMITEMACTIVATE)pNmhdr;
    if (-1 != lpNmItem->iItem)
    {
        CProcessExUtils::PROCESS_INFO& info = m_procInfos[lpNmItem->iItem];
        SetItemInt(IDC_EDIT_PID_INJECT, info.UniqueProcessId);
        SetItemString(IDC_EDIT_EXE_PATH_INJECT, info.ImageName);

        m_nActiveCol = lpNmItem->iSubItem;
        ListViewUpdateHeader();
    }

    return TRUE;
}

LRESULT CDialogFrame::ListViewItemChanged(WORD wID, LPNMHDR pNmhdr)
{
    LPNMLISTVIEW lpNmItem = (LPNMLISTVIEW)pNmhdr;
    if (-1 != lpNmItem->iItem)
    {
        CProcessExUtils::PROCESS_INFO& info = m_procInfos[lpNmItem->iItem];
        SetItemInt(IDC_EDIT_PID_INJECT, info.UniqueProcessId);
        SetItemString(IDC_EDIT_EXE_PATH_INJECT, info.ImageName);
    }

    return TRUE;
}

VOID CDialogFrame::ListViewUpdateVisiableItem()
{
    int nTop = ListView_GetTopIndex(m_hListView);
    int nPage = ListView_GetCountPerPage(m_hListView);

    ::SendMessage(m_hListView, WM_SETREDRAW, FALSE, 0);
    //更新可见项
    for (auto i = nTop; i <= nTop + nPage; i++)
    {
        ListView_Update(m_hListView, i);
    }
    ::SendMessage(m_hListView, WM_SETREDRAW, TRUE, 0);
}

void CDialogFrame::ListViewSort(int nCol)
{
    switch (nCol)
    {
    case eColIndex::eOrder:
        //std::reverse(m_procInfos.begin(), m_procInfos.end());
        break;
    case eColIndex::ePid:
        std::sort(m_procInfos.begin(), m_procInfos.end(), [](const CProcessExUtils::PROCESS_INFO& a, const CProcessExUtils::PROCESS_INFO& b) {
            return a.UniqueProcessId < b.UniqueProcessId;
            }
        );
        break;
    case eColIndex::eCpu:
        std::sort(m_procInfos.begin(), m_procInfos.end(), [](const CProcessExUtils::PROCESS_INFO& a, const CProcessExUtils::PROCESS_INFO& b) {
            return a.CpuUsage < b.CpuUsage;
            }
        );
        break;
    case eColIndex::eName:
        std::sort(m_procInfos.begin(), m_procInfos.end(), [](const CProcessExUtils::PROCESS_INFO& a, const CProcessExUtils::PROCESS_INFO& b) {
            return _tcsicmp(a.ImageName.c_str(), b.ImageName.c_str()) < 0;
            }
        );
        break;
    case eColIndex::eSessionId:
        std::sort(m_procInfos.begin(), m_procInfos.end(), [](const CProcessExUtils::PROCESS_INFO& a, const CProcessExUtils::PROCESS_INFO& b) {
            return a.SessionId < b.SessionId;
            }
        );
        break;
    case eColIndex::eDomainName:
        std::sort(m_procInfos.begin(), m_procInfos.end(), [](const CProcessExUtils::PROCESS_INFO& a, const CProcessExUtils::PROCESS_INFO& b) {
            return _tcsicmp(a.SidInfo.DomainName.c_str(), b.SidInfo.DomainName.c_str()) < 0;
            }
        );
        break;
    case eColIndex::eUserName:
        std::sort(m_procInfos.begin(), m_procInfos.end(), [](const CProcessExUtils::PROCESS_INFO& a, const CProcessExUtils::PROCESS_INFO& b) {
            return _tcsicmp(a.SidInfo.UserName.c_str(), b.SidInfo.UserName.c_str()) < 0;
            }
        );
        break;
    case eColIndex::eHandleCount:
        std::sort(m_procInfos.begin(), m_procInfos.end(), [](const CProcessExUtils::PROCESS_INFO& a, const CProcessExUtils::PROCESS_INFO& b) {
            return a.HandleCount < b.HandleCount;
            }
        );
        break;
    case eColIndex::eThreads:
        std::sort(m_procInfos.begin(), m_procInfos.end(), [](const CProcessExUtils::PROCESS_INFO& a, const CProcessExUtils::PROCESS_INFO& b) {
            return a.NumberOfThreads < b.NumberOfThreads;
            }
        );
        break;
    case eColIndex::eUserCount:
        std::sort(m_procInfos.begin(), m_procInfos.end(), [](const CProcessExUtils::PROCESS_INFO& a, const CProcessExUtils::PROCESS_INFO& b) {
            return a.UserCount < b.UserCount;
            }
        );
        break;
    case eColIndex::eGdiCount:
        std::sort(m_procInfos.begin(), m_procInfos.end(), [](const CProcessExUtils::PROCESS_INFO& a, const CProcessExUtils::PROCESS_INFO& b) {
            return a.GdiCount < b.GdiCount;
            }
        );
        break;
    case eColIndex::eFilePath:
        std::sort(m_procInfos.begin(), m_procInfos.end(), [](const CProcessExUtils::PROCESS_INFO& a, const CProcessExUtils::PROCESS_INFO& b) {
            return _tcsicmp(a.FilePath.c_str(), b.FilePath.c_str()) < 0;
            }
        );
        break;
    }

    if (0 != nCol)
    {
        m_nSortCol = nCol;
    }

    if (m_fDesc)
    {
        std::reverse(m_procInfos.begin(), m_procInfos.end());
    }
}

void CDialogFrame::ListViewUpdateHeader()
{
    // 更新列名
    TCHAR szBuf[MAX_PATH] = { 0 };
    LVCOLUMN lvc = { 0 };
    lvc.mask = LVCF_TEXT | LVCF_FMT | LVCF_WIDTH;
    lvc.pszText = szBuf;
    lvc.cchTextMax = _countof(szBuf);

    for (int i = 0; i < 11; i++)
    {
        ListView_GetColumn(m_hListView, i, &lvc);
        _tstring strName;

        if (m_nActiveCol == i)
        {
            strName += _T("* ");
        }

        strName += lpColName[i];

        if (0 != i)
        {
            if (m_nSortCol == i)
            {
                strName += m_fDesc ? _T(" ↓") : _T(" ↑");
            }
        }

        ::StringCchCopy(szBuf, _countof(szBuf), strName.c_str());
        ListView_SetColumn(m_hListView, i, &lvc);
    }
}

LRESULT CDialogFrame::ListViewItemFind(WORD wID, LPNMHDR pNmhdr)
{
    LPNMLVFINDITEM lpFindItem = (LPNMLVFINDITEM)pNmhdr;
    int iStart = lpFindItem->iStart;
    LPCTSTR lpStr = lpFindItem->lvfi.psz;

    if (eColIndex::eOrder == m_nActiveCol)
    {
        LPTSTR pEnd = (LPTSTR)lpStr;
        int index = _tcstol(lpStr, &pEnd, 10);

        if (_T('\0') == *pEnd)
        {
            if (index < 1)
            {
                index = 1;
            }

            return index - 1;
        }
    }

    if (eColIndex::ePid == m_nActiveCol)
    {
        LPTSTR pEnd = (LPTSTR)lpStr;
        int dwPID = _tcstol(lpStr, &pEnd, 10);

        for (int i = iStart; i < m_procInfos.size(); i++)
        {
            if (m_procInfos[i].UniqueProcessId == dwPID)
            {
                return i;
            }
        }

        for (int i = 0; i < iStart; i++)
        {
            if (m_procInfos[i].UniqueProcessId == dwPID)
            {
                return i;
            }
        }
    }

    if (eColIndex::eSessionId == m_nActiveCol)
    {
        LPTSTR pEnd = (LPTSTR)lpStr;
        int dwSessionId = _tcstol(lpStr, &pEnd, 10);

        for (int i = iStart; i < m_procInfos.size(); i++)
        {
            if (m_procInfos[i].SessionId == dwSessionId)
            {
                return i;
            }
        }

        for (int i = 0; i < iStart; i++)
        {
            if (m_procInfos[i].SessionId == dwSessionId)
            {
                return i;
            }
        }
    }

    if (eColIndex::eName == m_nActiveCol)
    {
        for (int i = iStart; i < m_procInfos.size(); i++)
        {
            if (_tstring::npos != CStrUtils::FindNoCase(m_procInfos[i].ImageName, lpStr))
            {
                return i;
            }
        }

        for (int i = 0; i < iStart; i++)
        {
            if (_tstring::npos != CStrUtils::FindNoCase(m_procInfos[i].ImageName, lpStr))
            {
                return i;
            }
        }
    }

    if (eColIndex::eDomainName == m_nActiveCol)
    {
        for (int i = iStart; i < m_procInfos.size(); i++)
        {
            if (_tstring::npos != CStrUtils::FindNoCase(m_procInfos[i].SidInfo.DomainName, lpStr))
            {
                return i;
            }
        }

        for (int i = 0; i < iStart; i++)
        {
            if (_tstring::npos != CStrUtils::FindNoCase(m_procInfos[i].SidInfo.DomainName, lpStr))
            {
                return i;
            }
        }
    }

    if (eColIndex::eUserName == m_nActiveCol)
    {
        for (int i = iStart; i < m_procInfos.size(); i++)
        {
            if (_tstring::npos != CStrUtils::FindNoCase(m_procInfos[i].SidInfo.UserName, lpStr))
            {
                return i;
            }
        }

        for (int i = 0; i < iStart; i++)
        {
            if (_tstring::npos != CStrUtils::FindNoCase(m_procInfos[i].SidInfo.UserName, lpStr))
            {
                return i;
            }
        }
    }

    if (eColIndex::eFilePath == m_nActiveCol)
    {
        for (int i = iStart; i < m_procInfos.size(); i++)
        {
            if (_tstring::npos != CStrUtils::FindNoCase(m_procInfos[i].ImageName, lpStr))
            {
                return i;
            }
        }

        for (int i = 0; i < iStart; i++)
        {
            if (_tstring::npos != CStrUtils::FindNoCase(m_procInfos[i].ImageName, lpStr))
            {
                return i;
            }
        }
    }

    return -1;
}

LRESULT CDialogFrame::ListViewColumnClick(WORD wID, LPNMHDR pNmhdr)
{
    LPNM_LISTVIEW pnmv = (LPNM_LISTVIEW)pNmhdr;

    if (m_nActiveCol != pnmv->iSubItem && 0 != pnmv->iSubItem)
    {
        m_fDesc = false;
    }
    else
    {
        m_fDesc = !m_fDesc;
    }

    m_nActiveCol = pnmv->iSubItem;
    if (0 != pnmv->iSubItem)
    {
        m_nSortCol = pnmv->iSubItem;
    }

    ListViewSort(m_nSortCol);

    ListViewUpdateHeader();
    ListViewUpdateVisiableItem();

    return TRUE;
}

LRESULT CDialogFrame::ListViewGetDisplayInfo(WORD wID, LPNMHDR pNmhdr)
{
    LV_DISPINFO* lpdi = (LV_DISPINFO*)pNmhdr;

    if (lpdi->item.mask & LVIF_TEXT)
    {
        int nIndex = lpdi->item.iSubItem;
        switch (nIndex)
        {
        case eColIndex::eOrder:
            StringCchPrintf(lpdi->item.pszText, lpdi->item.cchTextMax, _T("%d"), lpdi->item.iItem + 1);
            break;
        case eColIndex::ePid:
            StringCchPrintf(lpdi->item.pszText, lpdi->item.cchTextMax, _T("%d"), m_procInfos[lpdi->item.iItem].UniqueProcessId);
            break;
        case eColIndex::eCpu:
            StringCchPrintf(lpdi->item.pszText, lpdi->item.cchTextMax, _T("%.2g"), m_procInfos[lpdi->item.iItem].CpuUsage * 100.0f);
            break;
        case eColIndex::eName:
            StringCchPrintf(lpdi->item.pszText, lpdi->item.cchTextMax, _T("%s"), m_procInfos[lpdi->item.iItem].ImageName.c_str());
            break;
        case eColIndex::eSessionId:
            StringCchPrintf(lpdi->item.pszText, lpdi->item.cchTextMax, _T("%d"), m_procInfos[lpdi->item.iItem].SessionId);
            break;
        case eColIndex::eDomainName:
            StringCchPrintf(lpdi->item.pszText, lpdi->item.cchTextMax, _T("%s"), m_procInfos[lpdi->item.iItem].SidInfo.DomainName.c_str());
            break;
        case eColIndex::eUserName:
            StringCchPrintf(lpdi->item.pszText, lpdi->item.cchTextMax, _T("%s"), m_procInfos[lpdi->item.iItem].SidInfo.UserName.c_str());
            break;
        case eColIndex::eHandleCount:
            StringCchPrintf(lpdi->item.pszText, lpdi->item.cchTextMax, _T("%d"), m_procInfos[lpdi->item.iItem].HandleCount);
            break;
        case eColIndex::eThreads:
            StringCchPrintf(lpdi->item.pszText, lpdi->item.cchTextMax, _T("%d"), m_procInfos[lpdi->item.iItem].NumberOfThreads);
            break;
        case eColIndex::eUserCount:
            StringCchPrintf(lpdi->item.pszText, lpdi->item.cchTextMax, _T("%d"), m_procInfos[lpdi->item.iItem].UserCount);
            break;
        case eColIndex::eGdiCount:
            StringCchPrintf(lpdi->item.pszText, lpdi->item.cchTextMax, _T("%d"), m_procInfos[lpdi->item.iItem].GdiCount);
            break;
        case eColIndex::eFilePath:
            StringCchPrintf(lpdi->item.pszText, lpdi->item.cchTextMax, _T("%s"), m_procInfos[lpdi->item.iItem].FilePath.c_str());
            break;
        }
    }

    return TRUE;
}