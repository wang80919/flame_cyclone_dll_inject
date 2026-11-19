#include "pch.h"
#include "CWinHookApiDemo.h"

#if 0
#include <windows.h>
#include <tchar.h>
#include <stdlib.h>
#include "CWinHookApi.h"
#include "CXInputHook.h"

#pragma data_seg( ".mydata")
Hook_Dll_API HHOOK        g_hCBTHook = NULL;
Hook_Dll_API HHOOK        g_hKeyboardHook = NULL;
Hook_Dll_API HHOOK        g_hCallHook = NULL;
Hook_Dll_API HHOOK        g_hGetMsgHook = NULL;
#pragma data_seg()
#pragma comment(linker, "/section:.mydata,rws")

POINT GetChildWindowCenterPos(
    int nWidth,         //子窗口宽度
    int nHeight,        //子窗口高度
    HWND hParent        //父窗口句柄
)
{
    RECT rectParent = { 0 };
    LONG nParentW = 0;
    LONG nParentH = 0;

    // 如果父窗口句柄为空, 则以桌面窗口为父窗口
    if (NULL == hParent)
    {
        hParent = ::GetDesktopWindow();
    }

    // 获取并统计父窗口宽度和高度
    ::GetWindowRect(hParent, &rectParent);
    nParentW = rectParent.right - rectParent.left;
    nParentH = rectParent.bottom - rectParent.top;

    return {rectParent.left + (nParentW - nWidth) / 2, rectParent.top + (nParentH - nHeight) / 2};
}

LRESULT CALLBACK CBTProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode < HC_ACTION)
    {
        return ::CallNextHookEx(g_hCBTHook, nCode, wParam, lParam);
    }

    return ::CallNextHookEx(g_hCBTHook, nCode, wParam, lParam);

    // 即将激活一个窗口
    if (HCBT_ACTIVATE == nCode)
    {
        //指定要激活的窗口的句柄
        HWND hWndActivate = (HWND)wParam;
        LPCBTACTIVATESTRUCT pStruct = (LPCBTACTIVATESTRUCT)lParam;

        // 检查是对话框, 则修改显示位置为相对父窗口居中位置
        if (32770 == ::GetClassLongPtr(hWndActivate, GCW_ATOM))
        {
            BOOL fCenter = (BOOL)::GetProp(hWndActivate, _T("Center"));
            if (!fCenter)
            {
                RECT rtWnd = { 0 };
                ::GetWindowRect(hWndActivate, &rtWnd);
                POINT nPos = GetChildWindowCenterPos(rtWnd.right - rtWnd.left, rtWnd.bottom - rtWnd.top, ::GetParent(hWndActivate));
                ::MoveWindow(hWndActivate, nPos.x, nPos.y, rtWnd.right - rtWnd.left, rtWnd.bottom - rtWnd.top, true);
                ::SetProp(hWndActivate, _T("Center"), (HANDLE)1);
            }
        }
    }

    // 即将创建一个窗口
    if (HCBT_CREATEWND == nCode)
    {
        HWND hWndNew = (HWND)wParam;
        LPCBT_CREATEWND pStruct = (LPCBT_CREATEWND)lParam;
        HWND hParent = pStruct->lpcs->hwndParent;

        TCHAR szCaption[MAX_PATH] = { 0 };
        TCHAR szClass[MAX_PATH] = { 0 };

        ::GetWindowText(hWndNew, szCaption, _countof(szCaption));
        ::GetClassName(hWndNew, szClass, _countof(szClass));

        // 检查是对话框, 则修改显示位置为相对父窗口居中位置
        if (32770 == ::GetClassLongPtr(hWndNew, GCW_ATOM))
        {
            POINT nPos = GetChildWindowCenterPos(pStruct->lpcs->cx, pStruct->lpcs->cy, hParent);
            pStruct->lpcs->x = nPos.x;
            pStruct->lpcs->y = nPos.y;
        }
    }

    return ::CallNextHookEx(g_hCBTHook, nCode, wParam, lParam);
}

BOOL LoadCbtHook()
{
    UnloadCbtHook();

    g_hCBTHook = ::SetWindowsHookEx(WH_CBT, CBTProc, g_hModule, 0);
    return NULL != g_hCBTHook;
}

VOID UnloadCbtHook()
{
    if (NULL != g_hCBTHook)
    {
        ::UnhookWindowsHookEx(g_hCBTHook);
    }
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode < HC_ACTION)
    {
        return ::CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
    }

    WORD wVkCode = wParam;

    // https://learn.microsoft.com/zh-cn/windows/win32/inputdev/about-keyboard-input
    typedef struct _KEYSTATUS
    {
        DWORD wRepeat : 16;         //重复计数, 该值是用户按住键后重复击键的次数
        DWORD wScan : 8;            //扫描代码, 此值取决于 OEM
        DWORD fExtended : 1;        //指示键是扩展键，例如功能键还是数字键盘上的键。 如果键是扩展键，则值为 1;否则为 0
        DWORD Reserved : 4;         //保留
        DWORD fContext : 1;         //上下文代码。 如果 Alt 键关闭，则值为 1;否则为 0
        DWORD fPrevious : 1;        //上一个键状态。 如果在发送消息之前键关闭，则值为 1;如果键已打开，则为 0
        DWORD fTransition : 1;      //转换状态。 如果按下键，则值为 0;如果释放键，则值为 1
    }KEYSTATUS;
    return ::CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
}

BOOL LoadKeyboardHook()
{
    UnloadKeyboardHook();

    g_hKeyboardHook = ::SetWindowsHookEx(WH_KEYBOARD, KeyboardProc, g_hModule, 0);
    return NULL != g_hKeyboardHook;
}

VOID UnloadKeyboardHook()
{
    if (NULL != g_hKeyboardHook)
    {
        ::UnhookWindowsHookEx(g_hKeyboardHook);
    }
}

LRESULT CALLBACK CallWndProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode < 0)
    {
        return ::CallNextHookEx(g_hCallHook, nCode, wParam, lParam);
    }

    // 指定消息是否由当前进程发送
    BOOL fCurThreadSend = wParam;

    // 有关消息的详细信息
    LPCWPSTRUCT pStruct = (LPCWPSTRUCT)lParam;

    return ::CallNextHookEx(g_hCallHook, nCode, wParam, lParam);
}

BOOL LoadMsgHook()
{
    UnloadMsgHook();

    g_hCallHook = ::SetWindowsHookEx(WH_CALLWNDPROC, CallWndProc, g_hModule, 0);
    return NULL != g_hCallHook;
    
}

VOID UnloadMsgHook()
{
    if (NULL != g_hCallHook)
    {
        ::UnhookWindowsHookEx(g_hCallHook);
    }
}

LRESULT CALLBACK GetMsgProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode < HC_ACTION)
    {
        return ::CallNextHookEx(g_hGetMsgHook, nCode, wParam, lParam);
    }

    // 消息尚未从队列中删除
    if (PM_NOREMOVE == wParam)
    {

    }

    // 消息已从队列中删除
    if (PM_REMOVE == wParam)
    {

    }

    LPMSG pMsg = (LPMSG)lParam;

    return ::CallNextHookEx(g_hGetMsgHook, nCode, wParam, lParam);
}

BOOL LoadGetMsgHook()
{
    UnloadGetMsgHook();

    g_hGetMsgHook = ::SetWindowsHookEx(WH_GETMESSAGE, GetMsgProc, g_hModule, 0);
    return NULL != g_hGetMsgHook;
}

VOID UnloadGetMsgHook()
{
    if (NULL != g_hGetMsgHook)
    {
        ::UnhookWindowsHookEx(g_hGetMsgHook);
    }
}

LRESULT CALLBACK GetHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode < HC_ACTION)
    {
        return ::CallNextHookEx(g_hHook, nCode, wParam, lParam);
    }

    return ::CallNextHookEx(g_hHook, nCode, wParam, lParam);
}

#endif