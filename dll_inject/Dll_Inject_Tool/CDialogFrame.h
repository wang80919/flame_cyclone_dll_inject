#pragma once
#include "CWindow/CDialogBase.h"
#include "Win32Utils/CIniHelper.h"
#include "MessageBoxCenter.h"
#include "Win32Utils/CProcessExUtils.h"
#include "Win32Utils/CNamedPipe.h"
#include "../Hook_Dll/CWinHookApi.h"
#include <vector>
#include <thread>
#include <mutex>

#define AUTO_REFRESH_TIMER_ID           (200)

enum eColIndex
{
    eOrder,
    ePid,
    eCpu,
    eName,
    eSessionId,
    eDomainName,
    eUserName,
    eHandleCount,
    eThreads,
    eUserCount,
    eGdiCount,
    eFilePath
};

class CDialogFrame :
    public CDialogBase
{
public:


protected:

    dlg_msg LRESULT OnInitDialog(WPARAM wParam, LPARAM lParam);
	dlg_msg LRESULT OnClose(WPARAM wParam, LPARAM lParam);
    dlg_msg LRESULT OnDisplayChange(WPARAM wParam, LPARAM lParam);
    dlg_msg LRESULT OnLButtonDown(WPARAM wParam, LPARAM lParam);
    dlg_msg LRESULT OnLButtonUp(WPARAM wParam, LPARAM lParam);
    dlg_msg LRESULT OnMouseMove(WPARAM wParam, LPARAM lParam);
    dlg_msg LRESULT OnDropFiles(WPARAM wParam, LPARAM lParam);
    dlg_msg LRESULT OnTimer(WPARAM wParam, LPARAM lParam);

    dlg_command LRESULT OnTrayIcon(WPARAM wParam, LPARAM lParam);
    dlg_command LRESULT OnCommandExit(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd);
    dlg_command LRESULT OnTopmost(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd);
    dlg_command LRESULT OnRefresh(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd);
    dlg_command LRESULT OnFindPidInject(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd);
    dlg_command LRESULT OnFindNameInject(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd);
    dlg_command LRESULT OnDllPathInject(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd);
    dlg_command LRESULT OnLoad(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd);
    dlg_command LRESULT OnUnload(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd);
    dlg_command LRESULT OnDllPathHook(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd);
    dlg_command LRESULT OnHookLoad(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd);
    dlg_command LRESULT OnHookUnload(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd);
    dlg_command LRESULT OnCheckAutoRefresh(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd);
    dlg_command LRESULT OnRadioRefreshSpeed(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd);
    dlg_command LRESULT OnXboxHookEnable(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd);
    dlg_command LRESULT OnXboxEnable(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd);
    dlg_command LRESULT OnXboxMerge(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd);
    dlg_command LRESULT OnXboxHoldA(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd);
    dlg_command LRESULT OnXboxHoldB(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd);
    dlg_command LRESULT OnXboxHoldX(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd);
    dlg_command LRESULT OnXboxHoldY(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd);
    dlg_command LRESULT OnXboxHoldLB(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd);
    dlg_command LRESULT OnXboxHoldRB(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd);
    dlg_command LRESULT OnXboxBurstA(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd);
    dlg_command LRESULT OnXboxBurstB(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd);
    dlg_command LRESULT OnXboxBurstX(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd);
    dlg_command LRESULT OnXboxBurstY(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd);
    dlg_command LRESULT OnXboxBurstLB(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd);
    dlg_command LRESULT OnXboxBurstRB(WORD wNotify, WORD wID, HWND hWnd, WORD wStart, WORD wEnd);

    dlg_notify LRESULT ListViewRowClick(WORD wID, LPNMHDR pNmhdr);
    dlg_notify LRESULT ListViewItemChanged(WORD wID, LPNMHDR pNmhdr);
    dlg_notify LRESULT ListViewColumnClick(WORD wID, LPNMHDR pNmhdr);
    dlg_notify LRESULT ListViewItemFind(WORD wID, LPNMHDR pNmhdr);
    dlg_notify LRESULT ListViewGetDisplayInfo(WORD wID, LPNMHDR pNmhdr);

private:

    void ListViewUpdateHeader();
    void ListViewUpdate();
    void ListViewSort(int nCol);
    void ListViewUpdateVisiableItem();
    void ShowTrayIcon();
    void DeleteTrayIcon();
    void CreatePopMenu(int xPos, int yPos, bool isTop);
    bool ShowForeground(HWND hWnd);
    void InitListView();
    void GetXboxInput();
    double GetDpi();

private:
    HMENU               m_hPopMenu = NULL;
    HMENU               m_hMainMenu = NULL;
    HWND                m_hHideWnd = NULL;
    BOOL                m_fTrack = FALSE;

    CMessageBoxCenter   m_BoxCenter;
    CIniHelper          m_cfg;
    CNamedPipe          m_hookPipe;
    HWND                m_hListView = NULL;
    int                 m_nSortCol = 0;
    int                 m_nActiveCol = 0;
    bool                m_fDesc = 0;
    WORD                m_autoRefreshId = 0;
    double              m_fScale = 1.0f;
    bool                m_fQuit = false;

    std::vector<CProcessExUtils::PROCESS_INFO>  m_procInfos;

    std::thread         m_taskInput;
    std::mutex          m_mutex;
    bool                m_fHookQuit = false;

    DECLARE_DLG_MESSAGE_MAP()
};