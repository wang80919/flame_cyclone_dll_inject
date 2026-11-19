#pragma once

#include <wtypesbase.h>
#include <tchar.h>
#include <vector>
#include <map>
#include <string>

#ifdef _UNICODE
using _tstring = std::wstring;
#else
using _tstring = std::string;
#endif

// 进程信息
typedef struct _PROC_INFO
{
    DWORD       dwSessionId;            // 会话ID
    DWORD       dwPID;                  // 进程ID
    DWORD       dwParentPID;            // 父进程ID
    _tstring    strFilePath;            // 可执行文件的名称
    _tstring    strExeFile;             // 可执行文件的名称
    _tstring    strDomainName;          // 域名
    _tstring    strUserName;            // 用户名
    LONG        nPriClassBase;          // 此进程创建的任何线程的基本优先级
    DWORD       cntThreads;             // 进程启动的执行线程
    DWORD       dwGdiCount;             // GDI对象计数
    DWORD       dwUserCount;            // 用户对象计数
    DWORD       dwHandleCount;          // 句柄计数
}PROC_INFO;

// 模块信息
typedef struct _MODULE_INFO
{
    _tstring    strModule;              // 模块名
    _tstring    strExePath;             // 模块路径

    DWORD   th32ProcessID;              // 要检查其模块的进程标识符
    DWORD   GlblcntUsage;               // 模块的负载计数，通常没有意义，通常等于0xFFFF
    BYTE*   modBaseAddr;                // 拥有进程上下文中模块的基址
    DWORD   modBaseSize;                // 模块的大小（以字节为单位）
    HMODULE hModule;                    // 拥有进程上下文中模块的句柄

}MODULE_INFO;

// 进程节点信息
typedef struct _PROC_NODE
{
    PROC_INFO   ProcInfo;               // 进程信息
    std::vector<_PROC_NODE> NodeList;   // 子进程列表
}PROC_NODE;

// PE文件头信息
typedef struct _PE_HEADER_INFO
{
    IMAGE_DOS_HEADER    m_DosHeader;            // Dos头
    IMAGE_NT_HEADERS32  m_NtHeaders32;          // NT头(32位)
    IMAGE_NT_HEADERS64  m_NtHeaders64;          // NT头(64位)
    WORD                m_NtHeadersMagic;       // NT头魔数
}PE_HEADER_INFO;

// 进程名信息
typedef struct _PROCESS_NAME_INFO
{
    DWORD SessionId;
    DWORD ProcessId;
    _tstring ProcessName;
    _tstring UserName;
    _tstring DomainName;
} PROCESS_NAME_INFO, *PPROCESS_NAME_INFO;

// 进程组信息
typedef struct _PROCESS_GROUPS_INFO
{
    _tstring DomainName;
    _tstring UserName;
    _tstring SID;
    DWORD Attributes;
} PROCESS_GROUPS_INFO, *PPROCESS_GROUPS_INFO;

// 窗口信息
typedef struct _WND_INFO
{
    HWND hWnd;
    DWORD dwPid;
    DWORD dwTid;
    _tstring strClass;
    _tstring strText;

}WND_INFO;

// 窗口节点信息
typedef struct _WND_NODE
{
    HWND hWnd;                          // 窗口句柄
    DWORD dwPid;                        // 窗口进程ID
    DWORD dwTid;                        // 窗口线程ID
    _tstring strClass;                  // 窗口类名
    _tstring strText;                   // 窗口名
    _tstring strModule;                 // 窗口关联的模块名
    std::vector<_WND_NODE> NodeList;    // 子窗口列表
}WND_NODE;

class CProcessUtils
{
public:

    //
    // 进程提权
    // @param: hProcess             进程句柄
    // @param: lpPrivilegesName     特权名
    // @ret: bool                   操作是否成功
    static bool EnablePrivileges(HANDLE hProcess, LPCTSTR lpPrivilegesName);

    //
    // @brief: 创建进程
    // @param: strExePath           可执行文件路径
    // @param: strCommand           命令参数
    // @param: strCurDir            程序的当前目录
    // @param: bShow                是否显示程序
    // @param: bWait                是否等待程序直到其退出
    // @ret: bool                   操作是否成功
    static bool CreateProcessNormal(
        const _tstring& strExePath = _T(""),
        const _tstring& strCommand = _T(""),
        const _tstring& strCurDir = _T(""), 
        bool bShow = true, 
        bool bWait = true
    );

    //
    // @brief: 创建 带有UI 权限的进程
    // @param: strExePath           可执行文件路径
    // @param: strCommand           命令参数
    // @param: strCurDir            程序的当前目录
    static DWORD CreateProcessWithUIAccess(
        const _tstring& strExePath, 
        const _tstring& strCommand,
        const _tstring& strCurDir = _T("")
    );

    //
    // @brief: 创建不带UI权限的进程
    // @param: strExePath           可执行文件路径
    // @param: strCommand           命令参数
    // @param: strCurDir            程序的当前目录
    static DWORD CreateProcessNoUIAccess(
        const _tstring& strExePath, 
        const _tstring& strCommand,
        const _tstring& strCurDir = _T("")
    );

    //
    // @brief: 创建 SYSTEM 权限的进程
    // @param: strExePath           可执行文件路径
    // @param: strCommand           命令参数
    // @param: strCurDir            程序的当前目录
    static bool CreateProcessWithSystem(
        const _tstring& strExePath, 
        const _tstring& strCommand,
        const _tstring& strCurDir = _T("")
    );

    //
    // @brief: 检查指定进程是否有UI权限
    // @param: dwPid                进程ID
    // @param: pdwErr               输出错误码
    // @param: pfUIAccess           输出权限
    // @param: 操作是否成功
    // 
    static BOOL CheckProcessUIAccess(
        DWORD dwPid,
        DWORD *pdwErr, 
        DWORD *pfUIAccess
    );

    //
    // @brief: 获取进程可执行文件路径
    // @param: dwPID                进程ID
    // @param: bNtPath              是否为Win32 样式的 NT 路径
    // @ret: _tstring               进程可执行文件路径
    static _tstring GetPath(DWORD dwPID, bool bNtPath = true);

    //
    // @brief: 获取进程可执行文件路径
    // @param: dwPID                进程ID
    // @param: bNtPath              是否为Win32 样式的 NT 路径
    // @ret: _tstring               进程可执行文件路径
    static _tstring GetPathEx(DWORD dwPID, bool bNtPath = true);

    //
    // @brief: 获取父进程可执行文件路径
    // @param: dwPID                进程ID
    // @param: bNtPath              是否为Win32 样式的 NT 路径
    // @ret: _tstring               进程可执行文件路径
    static _tstring GetParentPath(DWORD dwPID, bool bNtPath = true);

    //
    // @brief: 获取进程名列表
    // @ret: std::map<DWORD, PROCESS_NAME_INFO>     进程名列表
    static std::map<DWORD, PROCESS_NAME_INFO> GetProcessNames();

    //
    // @brief: 获取所有进程信息
    // @ret: std::map<DWORD, PROC_INFO>  进程信息
    static std::map<DWORD, PROC_INFO> GetProcessInfos(const _tstring strName = _T(""));

    //
    // @brief: 获取所有进程节点
    // @ret: std::map<DWORD, PROC_NODE>  进程节点信息
    static std::map<DWORD, PROC_NODE> GetProcessNodes();

    //
    // @brief: 获取指定进程的模块信息(注意: 32位进程不能获取64位进程的模块信息)
    // @param: dwPID                进程ID
    // @ret: std::vector<MODULE_INFO>    模块信息
    static std::vector<MODULE_INFO> GetModuleInfos(DWORD dwPID);

    //
    // @brief: 获取进程加载的模块列表(注意: 32位进程不能获取64位进程的模块信息)
    // @param: dwPID                进程ID
    // @ret: _tstring               模块列表
    static std::vector<_tstring> GetModules(DWORD dwPID);

    //
    // @brief: 获取指定进程的进程节点
    // @ret: std::map<DWORD, PROC_INFO>  进程信息
    static PROC_NODE GetProcessNode(DWORD dwPID);

    //
    // @brief: 获取父进程的进程ID
    // @param: dwPID                进程ID
    // @ret: DWORD                  父进程的进程ID
    static DWORD GetParentID(DWORD dwPID);

    //
    // @brief: 等待进程结束
    // @param: dwPID                进程ID
    // @param: dwMilliseconds       超时时间
    // @ret: bool                   操作是否成功
    static bool WaitForProcess(DWORD dwPID, DWORD dwMilliseconds = INFINITE);

    //
    // @brief: 杀死指定进程
    // @param: dwPID                进程ID
    // @ret: bool                   操作是否成功
    static bool KillProcess(DWORD dwPID);

    //
    // @brief: 杀死指定进程ID的进程节点
    // @param: dwPID                进程ID
    // @ret: bool                   操作是否成功
    static bool KillProcessNode(DWORD dwPID);

    //
    // @brief: DOS路径转Nt路径
    // @param: strPath              路径
    // @ret: _tstring               Win32 样式的 NT 路径
    static _tstring DosPathToNtPath(const _tstring& strPath);

    //
    // @brief: Nt路径转DOS路径
    // @param: strPath              路径
    // @ret: _tstring               DOS路径
    static _tstring NtPathToDosPath(const _tstring& strPath);

    //
    // @brief: 获取指定进程的PE头信息
    // @param: dwPID                进程ID
    // @param: pPEHeader            PE头信息缓冲
    // @ret: bool                   操作是否成功
    static bool GetPEHeader(DWORD dwPID, PE_HEADER_INFO* pPEHeader);

    //
    // @brief: 获取进程的子系统类型
    // @param: dwPID                进程ID
    // @param: dwMilliseconds       超时时间
    // @ret: DWORD                  子系统类型
    //                              2: IMAGE_SUBSYSTEM_WINDOWS_GUI (windows 图形用户界面 (GUI) 子系统)
    //                              3: IMAGE_SUBSYSTEM_WINDOWS_CUI (Windows 字符模式用户界面 (CUI) 子系统)
    static DWORD GetSubsystemType(DWORD dwPID);

    //
    // @brief: 获取进程所运行的计算机的体系结构类型
    // @param: dwPID                进程ID
    // @ret: DWORD                  计算机的体系结构类型
    //                              0x014C: IMAGE_FILE_MACHINE_I386 (X86 平台)
    //                              0x8664: IMAGE_FILE_MACHINE_AMD64 (X64 平台)
    static WORD GetMachine(DWORD dwPID);

    //
    // @brief: 进程是否为x86平台
    // @param: dwPID                进程ID
    // @ret: bool                   检查结果
    static bool IsX86(DWORD dwPID);

    //
    // @brief: 进程是否为 X64 平台
    // @param: dwPID                进程ID
    // @ret: bool                   检查结果
    static bool IsX64(DWORD dwPID);

    //
    // @brief: 进程是否为 ARM32 平台
    // @param: dwPID                进程ID
    // @ret: bool                   检查结果
    static bool IsARM32(DWORD dwPID);

    //
    // @brief: 进程是否为 ARM64 平台
    // @param: dwPID                进程ID
    // @ret: bool                   检查结果
    static bool IsARM64(DWORD dwPID);

    //
    // @brief: 进程是否运行于 windows 图形用户界面 (GUI) 子系统
    // @param: dwPID                进程ID
    // @ret: bool                   检查结果
    static bool IsWindowsGUI(DWORD dwPID);

    //
    // @brief: 进程是否运行于Windows 字符模式用户界面 (CUI) 子系统
    // @param: dwPID                进程ID
    // @ret: bool                   检查结果
    static bool IsWindowsCUI(DWORD dwPID);

    //
    // @brief: 挂起进程
    // @param: dwPID                进程ID
    // @ret: bool                   检查结果
    static bool SuspendProcess(DWORD dwPID);

    //
    // @brief: 挂起进程节点
    // @param: dwPID                进程ID
    // @ret: bool                   检查结果
    static bool SuspendProcessNode(DWORD dwPID);

    //
    // @brief: 恢复进程
    // @param: dwPID                进程ID
    // @ret: bool                   检查结果
    static bool ResumeProcess(DWORD dwPID);

    //
    // @brief: 恢复进程节点
    // @param: dwPID                进程ID
    // @ret: bool                   检查结果
    static bool ResumeProcessNode(DWORD dwPID);

    //
    // @brief: 获取进程组信息
    // @param: dwPID                进程ID
    // @ret: std::vector<PROCESS_GROUPS_INFO>       进程组信息
    static std::vector<PROCESS_GROUPS_INFO> GetProcessGroupsInfo(DWORD dwPID);

    //
    // @brief: 获取进程中句柄计数
    // @param: dwPID                进程ID
    // @ret: DWORD                  句柄计数
    static DWORD GetProcessHandleCount(DWORD dwPID);

    //
    // @brief: 获取进程窗口列表
    // @param: dwPID                进程ID
    // @ret: std::map<HWND, WND_INFO>                  窗口列表
    static std::map<HWND, WND_INFO> GetProcessWindows(DWORD dwPid);

    //
    // @brief: 获取进程窗口节点列表
    // @param: dwPID                进程ID
    // @ret: std::map<HWND, WND_NODE>                  窗口节点列表
    static std::map<HWND, WND_NODE> GetProcessWindowNodes(DWORD dwPid);

private:

    //
    // 创建UI权限令牌
    // @param: phToken              输出令牌缓冲
    // @ret: DWORD                  错误码
    static DWORD _CreateUIAccessToken(PHANDLE phToken);

    //
    // 创建指定进程的令牌
    // @param: pid                  进程ID
    // @param: strExePath           可执行文件路径
    // @param: strCommand           命令参数
    // @param: strCurDir            程序的当前目录
    // @ret: bool                   是否操作成功
    static bool CreateProcessWithPidToken(
        DWORD pid, 
        const _tstring& strExePath, 
        const _tstring& strCommand,
        const _tstring& strCurDir
    );

    //
    // @brief: 杀死进程节点
    // @param: procNode             进程节点
    // @ret: bool                   操作是否成功
    static bool _KillProcessNode(const PROC_NODE& procNode);

    //
    // @brief: 挂起进程节点
    // @param: procNode             进程节点
    // @ret: bool                   检查结果
    static bool _SuspendProcessNode(const PROC_NODE& procNode);

    //
    // @brief: 恢复进程节点
    // @param: procNode             进程节点
    // @ret: bool                   检查结果
    static bool _ResumeProcessNode(const PROC_NODE& procNode);

    //
    // @brief: 获取指定进程的进程节点
    // @param: procInfos            进程信息
    // @param: dwPID                进程ID
    // @ret: std::map<DWORD, PROC_INFO>  进程信息
    static PROC_NODE _GetProcessNode(
        const std::map<DWORD, PROC_INFO>& procInfos,
        DWORD dwPID
    );

    //
    // @brief: 复制指定进程名的令牌
    // @param: strName              进程名
    // @param: dwSessionId          会话ID
    // @param: dwDesiredAccess      权限
    // @param: phToken              输出令牌缓冲
    // @ret: DWORD                  操作结果错误码
    static DWORD _DuplicateProcessToken(
        const _tstring& strName, 
        DWORD dwSessionId, 
        DWORD dwDesiredAccess, 
        PHANDLE phToken
    );

    // @brief: 创建带有UI权限的进程
    // @param: strExePath           可执行文件路径
    // @param: strCommand           命令参数
    // @param: strCurDir            程序的当前目录
    // @param: fUIAccess            是否带有UI权限
    // @ret: DWORD                  操作结果错误码
    static DWORD _CreateProcessWithUIAccess(
        const _tstring& strExePath, 
        const _tstring& strCommand,
        const _tstring& strCurDir = _T(""),
        bool fUIAccess = true
    );

    // 多字节字符串转宽字节字符串
    static std::wstring _MultiStrToWStr(UINT CodePage,const std::string& str);

    // 字符串转宽字节字符串
    static std::wstring _TStrToWStr(const _tstring& str);

    // 枚举进程窗口
    static void _EnumProcessWindows(std::map<HWND, WND_INFO>& infos, HWND hWnd, DWORD dwPid);

    // 获取子窗口节点
    static WND_NODE _GetWindowNode(HWND hWnd, std::vector<MODULE_INFO>& modules);
};