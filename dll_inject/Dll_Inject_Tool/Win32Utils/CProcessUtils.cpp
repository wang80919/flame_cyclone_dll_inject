#include "CProcessUtils.h"
#include <tlhelp32.h>
#include <psapi.h>
#include <strsafe.h>
#include <stdint.h>
#include <WtsApi32.h>
#include <sddl.h>
#pragma comment( lib, "Wtsapi32.lib" )
#pragma comment( lib, "Advapi32.lib" )

#define NT_SUCCESS(Status)  (((NTSTATUS)(Status)) >= 0)
#define STATUS_SUCCESS  ((NTSTATUS)0x00000000L)

#define DPSAPI_VERSION = 1
#pragma comment(lib, "Psapi.lib")

std::wstring CProcessUtils::_MultiStrToWStr(
    UINT CodePage, 
    const std::string& str
)
{
    //计算缓冲区所需的字符长度
    int cchWideChar = ::MultiByteToWideChar(CodePage, 0, str.c_str(), -1, NULL, NULL);
    std::wstring strResult(cchWideChar, 0);

    //成功则返回写入到指示的缓冲区的字符数
    size_t nConverted = ::MultiByteToWideChar(CodePage, 0, str.c_str(), (int)str.size(), &strResult[0], (int)strResult.size());

    //调整内容长度
    strResult.resize(nConverted);
    return strResult;
}

std::wstring CProcessUtils::_TStrToWStr(
    const _tstring& str
)
{
#ifdef _UNICODE
    return str;
#else
    return _MultiStrToWStr(CP_ACP, str);
#endif
}

bool CProcessUtils::EnablePrivileges(
    HANDLE hProcess, 
    LPCTSTR lpPrivilegesName
)
{
    HANDLE hToken = NULL;
    LUID luidValue = { 0 };
    TOKEN_PRIVILEGES tokenPrivileges = { 0 };
    BOOL isSuccess = FALSE;

    do
    {
        if (!::OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES, &hToken))
        {
            break;
        }

        if (!::LookupPrivilegeValue(NULL, lpPrivilegesName, &luidValue))
        {
            break;
        }

        tokenPrivileges.PrivilegeCount = 1;
        tokenPrivileges.Privileges[0].Luid = luidValue;
        tokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        if (!::AdjustTokenPrivileges(hToken, FALSE, &tokenPrivileges, 0, NULL, NULL))
        {
            break;
        }

        isSuccess = true;

    } while (false);

    if (hToken)
    {
        ::CloseHandle(hToken);
    }

    return isSuccess;
}

// 操作系统 版本号
// Windows 11               10.0*
// Windows 10               10.0*
// Windows Server 2022      10.0*
// Windows Server 2019      10.0*
// Windows Server 2016      10.0*
// Windows 8.1              6.3*
// Windows Server 2012 R2   6.3*
// Windows 8                6.2
// Windows Server 2012      6.2
// Windows 7                6.1
// Windows Server 2008 R2   6.1
// Windows Server 2008      6.0
// Windows Vista            6.0
// Windows Server 2003 R2   5.2
// Windows Server 2003      5.2
// Windows XP 64 位版本     5.2
// Windows XP               5.1
// Windows 2000             5.0
static bool _GetNtVersionNumbers(DWORD* pMajor, DWORD* pMinor, DWORD* pBuild)
{
    typedef VOID (NTAPI* NTPROC)(DWORD* dwMajor, DWORD* dwMinor, DWORD* dwBuild);
    HMODULE hModule = NULL;
    static NTPROC ProcAddress = NULL;
    bool fResult = false;

    if (NULL == ProcAddress)
    {
        hModule = ::LoadLibrary(_T("ntdll.dll"));
        if (NULL != hModule)
        {
            ProcAddress = (NTPROC)::GetProcAddress(hModule, "RtlGetNtVersionNumbers");
        }
    }

    if (NULL != ProcAddress)
    {
        fResult = true;

        ProcAddress(pMajor, pMinor, pBuild);
        *pBuild &= 0xffff;
    }

    if (NULL != hModule)
    {
        ::FreeLibrary(hModule);
    }

    return fResult;
}

static bool IsWindowsXPOrServer2003()
{
    DWORD dwMajor = 0;
    DWORD dwMinor = 0;
    DWORD dwBuild = 0;

    if (!_GetNtVersionNumbers(&dwMajor, &dwMinor, &dwBuild))
    {
        return false;
    }

    if (dwMajor == 5 && dwMinor >= 1)
    {
        return true;
    }

    return false;
}

static DWORD GetQueryAccess()
{
    static DWORD dwDesiredAccess = PROCESS_QUERY_LIMITED_INFORMATION;
    if (0 != dwDesiredAccess)
    {
        return dwDesiredAccess;
    }

    if (IsWindowsXPOrServer2003())
    {
        dwDesiredAccess = PROCESS_QUERY_INFORMATION;
    }

    return dwDesiredAccess;
}

bool CProcessUtils::CreateProcessNormal(
    const _tstring& strExePath/* = _T("")*/, 
    const _tstring& strCommand/* = _T("")*/, 
    const _tstring& strCurDir/* = _T("")*/, 
    bool bShow,
    bool bWait
)
{
    SECURITY_ATTRIBUTES   sa = { 0 };
    STARTUPINFO si = { 0 };
    PROCESS_INFORMATION pi = { 0 };

    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;
    sa.nLength = sizeof(sa);

    si.cb = sizeof(STARTUPINFO);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = bShow ? SW_SHOW : SW_HIDE;
    si.hStdInput = NULL;
    si.hStdOutput = NULL;
    si.hStdError = NULL;

    _tstring exePath = strExePath;
    _tstring exeCommand = strCommand;
    _tstring exeCurDir = strCurDir;

    LPCTSTR lpApplicationName = NULL;
    LPTSTR lpCommandLine = NULL;
    LPCTSTR lpCurrentDir = NULL;

    if (!exePath.empty())
    {
        lpApplicationName = exePath.c_str();
    }

    if (!exeCommand.empty())
    {
        lpCommandLine = (LPTSTR)exeCommand.c_str();
    }

    if (!exeCurDir.empty())
    {
        lpCurrentDir = exeCurDir.c_str();
    }

    if (!::CreateProcess(lpApplicationName, lpCommandLine, NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL, lpCurrentDir, &si, &pi))
    {
        return false;
    }

    if (bWait)
    {
        ::WaitForSingleObject(pi.hProcess, INFINITE);
    }

    ::CloseHandle(pi.hProcess);
    ::CloseHandle(pi.hThread);

    return true;
}

_tstring CProcessUtils::GetPath(DWORD dwPID, bool bNtPath)
{
    _tstring strPath;
    HANDLE hProcess = NULL;
    TCHAR szBuffer[MAX_PATH] = { 0 };
    DWORD dwSize = _countof(szBuffer);

    do
    {
        hProcess = ::OpenProcess(GetQueryAccess(), FALSE, dwPID);
        if (NULL == hProcess)
        {
            break;
        }

        // 句柄必须具有 PROCESS_QUERY_INFORMATION 或 PROCESS_QUERY_LIMITED_INFORMATION 访问权限。
        // Windows Server 2003 和 Windows XP： 句柄必须具有 PROCESS_QUERY_INFORMATION 访问权限。
        if (0 == ::GetProcessImageFileName(hProcess, szBuffer, dwSize))
        {
            break;
        }

        if (bNtPath)
        {
            strPath = DosPathToNtPath(szBuffer);
        }
        else
        {
            strPath = szBuffer;
        }

    } while (false);

    if (NULL != hProcess)
    {
        ::CloseHandle(hProcess);
    }

    return strPath;
}

_tstring CProcessUtils::GetPathEx(DWORD dwPID, bool bNtPath)
{
    _tstring strPath;
    HANDLE hProcess = NULL;
    TCHAR szBuffer[MAX_PATH] = { 0 };
    DWORD dwSize = _countof(szBuffer);

    do
    {
        hProcess = ::OpenProcess(GetQueryAccess(), FALSE, dwPID);
        if (NULL == hProcess)
        {
            break;
        }

        // 最低受支持的客户端: Windows Vista [仅限桌面应用]
        // 最低受支持的服务器: Windows Server 2008 [仅限桌面应用]
        // 进程句柄权限: PROCESS_QUERY_INFORMATION或PROCESS_QUERY_LIMITED_INFORMATION
        if (!::QueryFullProcessImageName(hProcess, bNtPath ? 0 : PROCESS_NAME_NATIVE, szBuffer, &dwSize))
        {
            break;
        }

        strPath = szBuffer;

    } while (false);

    if (NULL != hProcess)
    {
        ::CloseHandle(hProcess);
    }

    return strPath;
}

_tstring CProcessUtils::GetParentPath(DWORD dwPID, bool bNtPath)
{
    _tstring strPath;
    HANDLE hProcessSnap = NULL;
    PROCESSENTRY32 pe32 = { 0 };
    pe32.dwSize = sizeof(pe32);

    do
    {
        hProcessSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (INVALID_HANDLE_VALUE == hProcessSnap)
        {
            break;
        }

        if (!::Process32First(hProcessSnap, &pe32))
        {
            break;
        }

        do
        {
            if (pe32.th32ProcessID == dwPID)
            {
                strPath = GetPath(pe32.th32ParentProcessID, bNtPath);
                break;
            }

        } while (::Process32Next(hProcessSnap, &pe32));

    } while (false);

    if (INVALID_HANDLE_VALUE != hProcessSnap)
    {
        ::CloseHandle(hProcessSnap);
    }

    return strPath;
}

DWORD CProcessUtils::GetParentID(DWORD dwPID)
{
    HANDLE hProcessSnap = NULL;
    PROCESSENTRY32 pe32 = { 0 };
    pe32.dwSize = sizeof(pe32);
    DWORD dwParentProcessId = 0;

    do
    {
        hProcessSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (INVALID_HANDLE_VALUE == hProcessSnap)
        {
            break;
        }

        if (!::Process32First(hProcessSnap, &pe32))
        {
            break;
        }

        do
        {
            if (pe32.th32ProcessID == dwPID)
            {
                dwParentProcessId = pe32.th32ParentProcessID;
                break;
            }

        } while (::Process32Next(hProcessSnap, &pe32));

    } while (false);

    if (INVALID_HANDLE_VALUE != hProcessSnap)
    {
        ::CloseHandle(hProcessSnap);
    }

    return dwParentProcessId;
}

std::vector<_tstring> CProcessUtils::GetModules(DWORD dwPID)
{
    std::vector<_tstring> modulesList;
    TCHAR szModName[MAX_PATH] = { 0 };
    HMODULE* phMods = NULL;
    HANDLE hProcess = NULL;
    DWORD cbNeeded = 0;
    DWORD dwModSize = 0;

    do
    {
        hProcess = ::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwPID);
        if (NULL == hProcess)
        {
            break;
        }

        if (!::EnumProcessModulesEx(hProcess, NULL, NULL, &cbNeeded, LIST_MODULES_ALL))
        {
            break;
        }

        dwModSize = cbNeeded;

        phMods = (HMODULE*)::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, dwModSize);
        if (NULL == phMods)
        {
            break;
        }

        if (!::EnumProcessModulesEx(hProcess, phMods, dwModSize, &cbNeeded, LIST_MODULES_ALL))
        {
            break;
        }

        size_t nModuleCnt = (cbNeeded / sizeof(HMODULE));
        for (size_t i = 0; i < nModuleCnt; i++)
        {
            if (::GetModuleFileNameEx(hProcess, phMods[i], szModName, _countof(szModName)))
            {
                modulesList.push_back(szModName);
            }
        }

    } while (false);

    if (NULL != hProcess)
    {
        ::CloseHandle(hProcess);
    }

    if (phMods)
    {
        ::HeapFree(::GetProcessHeap(), 0, phMods);
    }

    return modulesList;
}

bool CProcessUtils::GetPEHeader(DWORD dwPID, PE_HEADER_INFO* pPEHeader)
{
    HMODULE hModule = NULL;
    LPVOID OldValue = NULL;
    BOOL isDisableWow64Fs = ::Wow64DisableWow64FsRedirection(&OldValue);
    bool fSuccess = false;

    if (nullptr == pPEHeader)
    {
        return false;
    }

    do
    {
        DWORD dwFlags = LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE;
        hModule = ::LoadLibraryEx(
            GetPath(dwPID).c_str(),
            0,
            dwFlags
        );

        if (NULL == hModule)
        {
            break;
        }

        LPBYTE pHeader = (BYTE*)hModule;
        BYTE* pImageData = (BYTE*)((ULONG_PTR)pHeader & ~((ULONG_PTR)0x03));
        PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)pImageData;
        if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
        {
            break;
        }

        PIMAGE_NT_HEADERS pNtHeader = (IMAGE_NT_HEADERS*)((BYTE*)(pDosHeader)+(DWORD)(pDosHeader->e_lfanew));
        if (IMAGE_NT_SIGNATURE != pNtHeader->Signature)
        {
            break;
        }

        // 检查 是否为 32位程序可选头
        if (IMAGE_NT_OPTIONAL_HDR32_MAGIC == pNtHeader->OptionalHeader.Magic)
        {
            pPEHeader->m_NtHeadersMagic = IMAGE_NT_OPTIONAL_HDR32_MAGIC;
            pPEHeader->m_NtHeaders32 = *(PIMAGE_NT_HEADERS32)pNtHeader;
        }
        // 检查 是否为 64位程序可选头
        else if (IMAGE_NT_OPTIONAL_HDR64_MAGIC == pNtHeader->OptionalHeader.Magic)
        {
            pPEHeader->m_NtHeadersMagic = IMAGE_NT_OPTIONAL_HDR64_MAGIC;
            pPEHeader->m_NtHeaders64 = *(PIMAGE_NT_HEADERS64)pNtHeader;
        }
        else
        {
            break;
        }

        pPEHeader->m_DosHeader = *(PIMAGE_DOS_HEADER)pImageData;
        fSuccess = true;

    } while (false);

    if (isDisableWow64Fs)
    {
        ::Wow64RevertWow64FsRedirection(OldValue);
    }

    if (NULL != hModule)
    {
        ::FreeLibrary(hModule);
    }

    return fSuccess;
}

bool CProcessUtils::WaitForProcess(
    DWORD dwPID,
    DWORD dwMilliseconds/* = INFINITE*/
)
{
    HANDLE hProcess = NULL;
    bool fResult = false;
        
    hProcess = ::OpenProcess(SYNCHRONIZE, FALSE, dwPID);
    if (NULL == hProcess)
    {
        return false;
    }

    fResult = (WAIT_OBJECT_0 == ::WaitForSingleObject(hProcess, dwMilliseconds));
    ::CloseHandle(hProcess);

    return fResult;
}

std::map<DWORD, PROCESS_NAME_INFO> CProcessUtils::GetProcessNames()
{
    std::map<DWORD, PROCESS_NAME_INFO> mapResult;
    PWTS_PROCESS_INFO pPi = NULL;
    DWORD dwCount = 0;
    do
    {
        if (!::WTSEnumerateProcesses(NULL, 0, 1, &pPi, &dwCount))
        {
            break;
        }

        for (DWORD i = 0; i < dwCount; i++)
        {
            PSID pUserSid = pPi[i].pUserSid;
            TCHAR szName[MAX_PATH] = { 0 };
            DWORD cchName = _countof(szName);
            TCHAR szDomainName[MAX_PATH] = { 0 };
            DWORD cchDomainName = _countof(szDomainName);
            SID_NAME_USE sidNameUse = SidTypeUser;

            if (::LookupAccountSid(NULL, pUserSid, szName, &cchName, szDomainName, &cchDomainName, &sidNameUse))
            {
            }

            PROCESS_NAME_INFO info;
            info.SessionId = pPi[i].SessionId;
            info.ProcessName = pPi[i].pProcessName;
            info.ProcessId = pPi[i].ProcessId;
            info.UserName = szName;
            info.DomainName = szDomainName;

            mapResult.insert(std::make_pair(pPi[i].ProcessId, info));
        }

    } while (false);

    if (pPi)
    {
        ::WTSFreeMemory(pPi);
    }

    return mapResult;
}

std::map<DWORD, PROC_INFO> CProcessUtils::GetProcessInfos(const _tstring strName/* = _T("")*/)
{
    std::map<DWORD, PROC_INFO> infos;
    HANDLE hProcessSnap = NULL;
    PROCESSENTRY32 pe32 = { 0 };
    pe32.dwSize = sizeof(pe32);

    do
    {
        hProcessSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (INVALID_HANDLE_VALUE == hProcessSnap)
        {
            break;
        }

        if (!::Process32First(hProcessSnap, &pe32))
        {
            break;
        }

        do
        {
            PROC_INFO info;
            info.dwParentPID = pe32.th32ParentProcessID;
            info.dwPID = pe32.th32ProcessID;
            info.strExeFile = pe32.szExeFile;
            info.cntThreads = pe32.cntThreads;
            info.nPriClassBase = pe32.pcPriClassBase;

            HANDLE hProcess = NULL;
            DWORD dwHandleCount = 0;
            DWORD dwGdiCount = 0;
            DWORD dwUserCount = 0;

            // 获取句柄数量, GUI对象数量, 用户对象数量
            do
            {
                // 打开进程
                hProcess = ::OpenProcess(GetQueryAccess(), FALSE, info.dwPID);
                if (NULL == hProcess)
                {
                    break;
                }

                ::GetProcessHandleCount(hProcess, &dwHandleCount);
                dwGdiCount = GetGuiResources(hProcess, GR_GDIOBJECTS);
                dwUserCount = GetGuiResources(hProcess, GR_USEROBJECTS);



            } while (false);

            if (NULL != hProcess)
            {
                ::CloseHandle(hProcess);
            }

            if (strName.empty())
            {
                info.dwHandleCount = dwHandleCount;
                info.dwGdiCount = dwGdiCount;
                info.dwUserCount = dwUserCount;
                info.strFilePath = GetPath(info.dwPID);

                infos.insert(std::make_pair(info.dwPID, info));
            }
            else
            {
                if (0 == _tcsicmp(strName.c_str(), pe32.szExeFile))
                {
                    info.dwHandleCount = dwHandleCount;
                    info.dwGdiCount = dwGdiCount;
                    info.dwUserCount = dwUserCount;
                    info.strFilePath = GetPath(info.dwPID);

                    infos.insert(std::make_pair(info.dwPID, info));
                }
            }

        } while (::Process32Next(hProcessSnap, &pe32));

    } while (false);

    if (INVALID_HANDLE_VALUE != hProcessSnap)
    {
        ::CloseHandle(hProcessSnap);
    }

    // 获取进程用户名信息
    std::map<DWORD, PROCESS_NAME_INFO> mapResult = GetProcessNames();

    for (auto& item : infos)
    {
        auto itFind = mapResult.find(item.first);
        if (mapResult.end() != itFind)
        {
            item.second.strDomainName = itFind->second.DomainName;
            item.second.strUserName = itFind->second.UserName;
            item.second.dwSessionId = itFind->second.SessionId;
        }
    }

    return infos;
}

std::map<DWORD, PROC_NODE> CProcessUtils::GetProcessNodes()
{
    std::map<DWORD, PROC_NODE> mapResult;
    std::map<DWORD, PROC_INFO> infos = CProcessUtils::GetProcessInfos();

    for (const auto& item : infos)
    {
        if (mapResult.end() != mapResult.find(item.first))
        {
            continue;
        }

        if (0 == item.second.dwParentPID || 
            infos.end() == infos.find(item.second.dwParentPID)
            )
        {
            PROC_NODE node = _GetProcessNode(infos, item.first);
            mapResult.insert(std::make_pair(item.first, node));
        }
    }

    return mapResult;
}

std::vector<MODULE_INFO> CProcessUtils::GetModuleInfos(DWORD dwPID)
{
    std::vector<MODULE_INFO> vModules;
    HANDLE hModuleSnap = NULL;
    MODULEENTRY32 me32 = { 0 };
    me32.dwSize = sizeof(me32);

    do
    {
        hModuleSnap = ::CreateToolhelp32Snapshot(
            TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32,
            dwPID
        );

        if (INVALID_HANDLE_VALUE == hModuleSnap)
        {
            break;
        }

        if (!::Module32First(hModuleSnap, &me32))
        {
            break;
        }

        do
        {
            MODULE_INFO info;
            info.th32ProcessID = me32.th32ProcessID;
            info.GlblcntUsage = me32.GlblcntUsage;
            info.modBaseAddr = me32.modBaseAddr;
            info.modBaseSize = me32.modBaseSize;
            info.hModule = me32.hModule;
            info.strExePath = me32.szExePath;
            info.strModule = me32.szModule;

            vModules.push_back(info);

        } while (::Module32Next(hModuleSnap, &me32));

    } while (false);

    if (INVALID_HANDLE_VALUE != hModuleSnap)
    {
        ::CloseHandle(hModuleSnap);
    }

    return vModules;
}

PROC_NODE CProcessUtils::GetProcessNode(DWORD dwPID)
{
    std::map<DWORD, PROC_INFO> procInfos = CProcessUtils::GetProcessInfos();
    return _GetProcessNode(procInfos, dwPID);
}

PROC_NODE CProcessUtils::_GetProcessNode(
    const std::map<DWORD, PROC_INFO>& procInfos,
    DWORD dwPID
)
{
    PROC_NODE procNode;
    for (auto& item : procInfos)
    {
        const PROC_INFO& info = item.second;
        if (item.first == dwPID)
        {
            procNode.ProcInfo = info;
        }
        else if (info.dwParentPID == dwPID)
        {
            PROC_NODE subNode = _GetProcessNode(procInfos, info.dwPID);
            procNode.NodeList.push_back(subNode);
        }
    }

    return procNode;
}

bool CProcessUtils::KillProcess(DWORD dwPID)
{
    HANDLE hProcess = NULL;
    BOOL fSuccess = false;

    // 杀死进程
    hProcess = ::OpenProcess(PROCESS_TERMINATE, FALSE, dwPID);
    if (NULL == hProcess)
    {
        return false;
    }

    fSuccess = ::TerminateProcess(hProcess, 0);
    ::CloseHandle(hProcess);

    return fSuccess;
}

bool CProcessUtils::_KillProcessNode(const PROC_NODE& procNode)
{
    HANDLE hProcess = NULL;
    BOOL fSuccess = false;

    // 杀死子进程
    for (auto item : procNode.NodeList)
    {
        _KillProcessNode(item);
    }

    // 杀死进程
    hProcess = ::OpenProcess(PROCESS_TERMINATE, FALSE, procNode.ProcInfo.dwPID);
    if (NULL == hProcess)
    {
        return false;
    }

    fSuccess = ::TerminateProcess(hProcess, 0);
    ::CloseHandle(hProcess);

    return fSuccess;
}

bool CProcessUtils::KillProcessNode(DWORD dwPID)
{
    return _KillProcessNode(GetProcessNode(dwPID));
}

_tstring CProcessUtils::DosPathToNtPath(const _tstring& strPath)
{
    _tstring strResultPath;
    TCHAR szDriveStrings[MAX_PATH] = { 0 };
    TCHAR szDosBuf[MAX_PATH] = { 0 };
    TCHAR szResultBuf[MAX_PATH] = { 0 };
    LPTSTR pDriveStr = NULL;

    // 获取盘符名到缓冲
    if (::GetLogicalDriveStrings(_countof(szDriveStrings), szDriveStrings))
    {
        // 遍历盘符名
        for (int i = 0; i < _countof(szDriveStrings); i += 4)
        {
            pDriveStr = &szDriveStrings[i];
            pDriveStr[2] = _T('\0');

            // 查询盘符对应的DOS设备名称
            if (!::QueryDosDevice(pDriveStr, szDosBuf, _countof(szDosBuf)))
            {
                break;
            }

            // 对比路径前缀
            size_t nLen = _tcslen(szDosBuf);
            if (0 == _tcsnicmp(strPath.c_str(), szDosBuf, nLen))
            {
                lstrcpy(szResultBuf, pDriveStr);
                lstrcat(szResultBuf, strPath.c_str() + nLen);
                strResultPath = szResultBuf;
                break;
            }
        }
    }

    return strResultPath;
}

_tstring CProcessUtils::NtPathToDosPath(const _tstring& strPath)
{
    _tstring strResultPath;
    TCHAR szDriveStrings[MAX_PATH] = { 0 };
    TCHAR szDosBuf[MAX_PATH] = { 0 };
    TCHAR szResultBuf[MAX_PATH] = { 0 };
    LPTSTR pDriveStr = NULL;

    // 获取盘符名到缓冲
    if (::GetLogicalDriveStrings(_countof(szDriveStrings), szDriveStrings))
    {
        // 遍历盘符名
        for (int i = 0; i < _countof(szDriveStrings); i += 4)
        {
            pDriveStr = &szDriveStrings[i];
            pDriveStr[2] = _T('\0');

            // 查询盘符对应的DOS设备名称
            if (!::QueryDosDevice(pDriveStr, szDosBuf, _countof(szDosBuf)))
            {
                break;
            }

            // 对比路径前缀
            size_t nLen = _tcslen(pDriveStr);
            if (0 == _tcsnicmp(strPath.c_str(), pDriveStr, nLen))
            {
                lstrcpy(szResultBuf, szDosBuf);
                lstrcat(szResultBuf, strPath.c_str() + nLen);
                strResultPath = szResultBuf;
                break;
            }
        }
    }

    return strResultPath;
}

DWORD CProcessUtils::GetSubsystemType(DWORD dwPID)
{
    PE_HEADER_INFO peHeader = { 0 };

    if (!GetPEHeader(dwPID, &peHeader))
    {
        return 0;
    }

    if (IMAGE_NT_OPTIONAL_HDR32_MAGIC == peHeader.m_NtHeadersMagic)
    {
        return peHeader.m_NtHeaders32.OptionalHeader.Subsystem;
    }

    if (IMAGE_NT_OPTIONAL_HDR64_MAGIC == peHeader.m_NtHeadersMagic)
    {
        return peHeader.m_NtHeaders64.OptionalHeader.Subsystem;
    }

    return 0;
}

WORD CProcessUtils::GetMachine(DWORD dwPID)
{
    PE_HEADER_INFO peHeader = { 0 };

    if (!GetPEHeader(dwPID, &peHeader))
    {
        return 0;
    }

    if (IMAGE_NT_OPTIONAL_HDR32_MAGIC == peHeader.m_NtHeadersMagic)
    {
        return peHeader.m_NtHeaders32.FileHeader.Machine;
    }

    if (IMAGE_NT_OPTIONAL_HDR64_MAGIC == peHeader.m_NtHeadersMagic)
    {
        return peHeader.m_NtHeaders64.FileHeader.Machine;
    }

    return 0;
}

bool CProcessUtils::IsX86(DWORD dwPID)
{
    return IMAGE_FILE_MACHINE_I386 == GetMachine(dwPID);
}

bool CProcessUtils::IsX64(DWORD dwPID)
{
    return IMAGE_FILE_MACHINE_AMD64 == GetMachine(dwPID);
}

bool CProcessUtils::IsARM32(DWORD dwPID)
{
    WORD wMachine = GetMachine(dwPID);
    return wMachine >= IMAGE_FILE_MACHINE_ARM && wMachine <= IMAGE_FILE_MACHINE_ARMNT;
}

bool CProcessUtils::IsARM64(DWORD dwPID)
{
    return IMAGE_FILE_MACHINE_ARM64 == GetMachine(dwPID);
}

bool CProcessUtils::IsWindowsGUI(DWORD dwPID)
{
    return IMAGE_SUBSYSTEM_WINDOWS_GUI == GetSubsystemType(dwPID);
}

bool CProcessUtils::IsWindowsCUI(DWORD dwPID)
{
    return IMAGE_SUBSYSTEM_WINDOWS_CUI == GetSubsystemType(dwPID);
}

bool CProcessUtils::SuspendProcess(DWORD dwPID)
{
    typedef NTSTATUS (*_NtSuspendProcess)(HANDLE hProcess);
    _NtSuspendProcess NtSuspendProcess = NULL;
    HANDLE hProcess = NULL;
    HMODULE hModule = NULL;
    bool fResult = false;

    do
    {
        hModule = ::LoadLibrary(_T("ntdll.dll"));
        if(NULL == hModule)
        {
            break;
        }

        hProcess = ::OpenProcess(PROCESS_SUSPEND_RESUME, FALSE, dwPID);
        if (NULL == hProcess)
        {
            break;
        }

        NtSuspendProcess = (_NtSuspendProcess)::GetProcAddress(hModule, "NtSuspendProcess");
        if (NULL == NtSuspendProcess)
        {
            break;
        }

        fResult = (STATUS_SUCCESS == NtSuspendProcess(hProcess));

    } while (false);

    if (hProcess)
    {
        ::CloseHandle(hProcess);
    }

    if (hModule)
    {
        ::FreeLibrary(hModule);
    }

    return fResult;
}

bool CProcessUtils::ResumeProcess(DWORD dwPID)
{
    typedef NTSTATUS (*_NtResumeProcess)(HANDLE hProcess);
    _NtResumeProcess NtResumeProcess = NULL;
    HANDLE hProcess = NULL;
    HMODULE hModule = NULL;
    bool fResult = false;

    do
    {
        hModule = ::LoadLibrary(_T("ntdll.dll"));
        if(NULL == hModule)
        {
            break;
        }

        hProcess = ::OpenProcess(PROCESS_SUSPEND_RESUME, FALSE, dwPID);
        if (NULL == hProcess)
        {
            break;
        }

        NtResumeProcess = (_NtResumeProcess)::GetProcAddress(hModule, "NtResumeProcess");
        if (NULL == NtResumeProcess)
        {
            break;
        }

        fResult = (STATUS_SUCCESS == NtResumeProcess(hProcess));

    } while (false);

    if (hProcess)
    {
        ::CloseHandle(hProcess);
    }

    if (hModule)
    {
        ::FreeLibrary(hModule);
    }

    return fResult;
}

bool CProcessUtils::SuspendProcessNode(DWORD dwPID)
{
    return _SuspendProcessNode(GetProcessNode(dwPID));
}

bool CProcessUtils::ResumeProcessNode(DWORD dwPID)
{
    return _ResumeProcessNode(GetProcessNode(dwPID));
}

bool CProcessUtils::_SuspendProcessNode(const PROC_NODE& procNode)
{
    bool fResult = false;

    do
    {
        // 暂停主进程
        if (!SuspendProcess(procNode.ProcInfo.dwPID))
        {
            break;
        }

        // 暂停子进程
        for (const auto& item : procNode.NodeList)
        {
            if (!_SuspendProcessNode(item))
            {
                break;
            }
        }

        fResult = true;

    } while (false);

    return fResult;
}

bool CProcessUtils::_ResumeProcessNode(const PROC_NODE& procNode)
{
    bool fResult = false;

    do
    {
        // 恢复主进程
        if (!ResumeProcess(procNode.ProcInfo.dwPID))
        {
            break;
        }

        // 恢复子进程
        for (const auto& item : procNode.NodeList)
        {
            if (!_ResumeProcessNode(item))
            {
                break;
            }
        }

        fResult = true;

    } while (false);

    return fResult;
}

DWORD CProcessUtils::GetProcessHandleCount(DWORD dwPID)
{
    HANDLE hProcess = NULL;
    DWORD dwHandleCount = 0;
    DWORD dwGdiCount = 0;
    DWORD dwUserCount = 0;

    do
    {
        // 打开进程
        hProcess = ::OpenProcess(GetQueryAccess(), FALSE, dwPID);
        if (NULL == hProcess)
        {
            break;
        }

        ::GetProcessHandleCount(hProcess, &dwHandleCount);
        dwGdiCount = GetGuiResources(hProcess, GR_GDIOBJECTS);
        dwUserCount = GetGuiResources(hProcess, GR_USEROBJECTS);

    } while (false);


    if (NULL != hProcess)
    {
        ::CloseHandle(hProcess);
    }

    return dwHandleCount;
}

std::vector<PROCESS_GROUPS_INFO> CProcessUtils::GetProcessGroupsInfo(DWORD dwPID)
{
    std::vector<PROCESS_GROUPS_INFO> vResult;
    HANDLE hToken = NULL;
    PTOKEN_GROUPS pGroupInfo = NULL;
    SID_NAME_USE SidType = SidTypeUser;
    TCHAR szName[MAX_PATH] = { 0 };
    TCHAR szDomain[MAX_PATH] = { 0 };
    PSID pSID = NULL;
    SID_IDENTIFIER_AUTHORITY SIDAuth = SECURITY_NT_AUTHORITY;
    HANDLE hProcess = NULL;
    DWORD dwSize = 0;

    do
    {
        // 打开进程
        hProcess = ::OpenProcess(GetQueryAccess(), FALSE, dwPID);
        if (NULL == hProcess)
        {
            break;
        }

        // 打开进程令牌
        if (!::OpenProcessToken(hProcess, TOKEN_QUERY, &hToken))
        {
            break;
        }

        // 获取令牌信息所需缓冲大小
        if (!::GetTokenInformation(hToken, TokenGroups, NULL, dwSize, &dwSize))
        {
            if (ERROR_INSUFFICIENT_BUFFER != ::GetLastError())
            {
                break;
            }
        }

        // 分配缓冲
        pGroupInfo = (PTOKEN_GROUPS)::GlobalAlloc(GPTR, dwSize);
        if (NULL == pGroupInfo)
        {
            break;
        }

        // 获取令牌组信息
        if (!::GetTokenInformation(hToken, TokenGroups, pGroupInfo, dwSize, &dwSize))
        {
            break;
        }

        // 分配和初始化一个安全标识符
        if (!::AllocateAndInitializeSid(&SIDAuth, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
            0, 0, 0, 0, 0, 0,
            &pSID))
        {
            break;
        }

        // 检索SID信息
        for (DWORD i = 0; i < pGroupInfo->GroupCount; i++)
        {
            PROCESS_GROUPS_INFO info;

            dwSize = _countof(szName);
            if (!::LookupAccountSid(NULL, pGroupInfo->Groups[i].Sid, szName, &dwSize, szDomain, &dwSize, &SidType))
            {
                if (ERROR_NONE_MAPPED != ::GetLastError())
                {
                    break;
                }

                _tcscpy_s(szName, dwSize, _T("NONE_MAPPED"));
            }

            LPTSTR pStringSid = NULL;
            if (ConvertSidToStringSid(pGroupInfo->Groups[i].Sid, &pStringSid))
            {
                info.SID = pStringSid;
                ::LocalFree(pStringSid);
            }

            info.DomainName = szDomain;
            info.UserName = szName;
            info.Attributes = pGroupInfo->Groups[i].Attributes;
            vResult.push_back(info);
        }

    } while (false);

    if (pSID)
    {
        ::FreeSid(pSID);
    }

    if (pGroupInfo)
    {
        ::GlobalFree(pGroupInfo);
    }

    if (NULL != hProcess)
    {
        ::CloseHandle(hProcess);
    }

    return vResult;
}

DWORD CProcessUtils::_DuplicateProcessToken(
    const _tstring& strName,
    DWORD dwSessionId, 
    DWORD dwDesiredAccess,
    PHANDLE phToken
)
{
    PROCESSENTRY32 pe = { 0 };
    PRIVILEGE_SET ps = { 0 };
    HANDLE hSnapshot = INVALID_HANDLE_VALUE;
    BOOL fResult = FALSE;
    DWORD dwErr = ERROR_SUCCESS;
    BOOL fFound = FALSE;

    ps.PrivilegeCount = 1;
    ps.Control = PRIVILEGE_SET_ALL_NECESSARY;

    do
    {
        if (!::LookupPrivilegeValue(NULL, SE_TCB_NAME, &ps.Privilege[0].Luid))
        {
            break;
        }

        hSnapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (INVALID_HANDLE_VALUE == hSnapshot)
        {
            break;
        }

        pe.dwSize = sizeof(pe);
        if (!::Process32First(hSnapshot, &pe))
        {
            break;
        }

        do
        {
            HANDLE hProcess = NULL;
            HANDLE hToken = NULL;
            DWORD dwRetLen = 0;
            DWORD sid = 0;

            if (0 != _tcsicmp(pe.szExeFile, strName.c_str()))
            {
                continue;
            }

            hProcess = ::OpenProcess(GetQueryAccess(), FALSE, pe.th32ProcessID);
            if (!hProcess)
            {
                break;
            }

            do
            {
                BOOL fTcb = FALSE;

                if (!::OpenProcessToken(hProcess, TOKEN_QUERY | TOKEN_DUPLICATE, &hToken))
                {
                    break;
                }

                if (!::PrivilegeCheck(hToken, &ps, &fTcb) && fTcb)
                {
                    break;
                }

                if (!::GetTokenInformation(hToken, TokenSessionId, &sid, sizeof(sid), &dwRetLen) && sid == dwSessionId)
                {
                    break;
                }

                if (!::DuplicateTokenEx(hToken, dwDesiredAccess, NULL, SecurityImpersonation, TokenImpersonation, phToken))
                {
                    break;
                }

                fFound = true;
                dwErr = ERROR_SUCCESS;

            } while (false);

            if (hToken)
            {
                ::CloseHandle(hToken);
            }

            if (hProcess)
            {
                ::CloseHandle(hProcess);
            }

            if (fFound)
            {
                break;
            }

        } while (::Process32Next(hSnapshot, &pe));

        if (!fFound)
        {
            dwErr = ERROR_NOT_FOUND;
            break;
        }

        fResult = true;

    } while (false);

    if (!fResult)
    {
        dwErr = ::GetLastError();
        if (!fFound)
        {
            dwErr = ERROR_NOT_FOUND;
        }
    }

    if (INVALID_HANDLE_VALUE != hSnapshot)
    {
        ::CloseHandle(hSnapshot);
    }

    return dwErr;
}

DWORD CProcessUtils::_CreateProcessWithUIAccess(
    const _tstring& strExePath, 
    const _tstring& strCommand,
    const _tstring& strCurDir/* = _T("")*/,
    bool fUIAccess/* = true*/
)
{
    HANDLE hTokenUIAccess = NULL;
    DWORD dwErr = ERROR_SUCCESS;
    BOOL fResult = FALSE;
    _tstring exePath = strExePath;
    _tstring exeCommand = strCommand;
    _tstring exeCurDir = strCurDir;

    LPCTSTR lpApplicationName = NULL;
    LPTSTR lpCommandLine = NULL;
    LPCTSTR lpCurrentDir = NULL;

    if (!strExePath.empty())
    {
        lpApplicationName = exePath.c_str();
    }

    if (!strCommand.empty())
    {
        lpCommandLine = (LPTSTR)exeCommand.c_str();
    }

    if (!exeCurDir.empty())
    {
        lpCurrentDir = exeCurDir.c_str();
    }

    do
    {
        STARTUPINFO si = { 0 };
        PROCESS_INFORMATION pi = { 0 };

        dwErr = _CreateUIAccessToken(&hTokenUIAccess);
        if (ERROR_SUCCESS != dwErr)
        {
            break;
        }

        if (!fUIAccess)
        {
            BOOL fTokenInformation = FALSE;
            ::SetTokenInformation(hTokenUIAccess, TokenUIAccess, &fTokenInformation, sizeof(fTokenInformation));
        }

        ::GetStartupInfo(&si);
        if (!::CreateProcessAsUser(hTokenUIAccess, lpApplicationName, lpCommandLine, NULL, NULL, FALSE, 0, NULL, lpCurrentDir, &si, &pi))
        {
            break;
        }

        ::CloseHandle(pi.hProcess);
        ::CloseHandle(pi.hThread);

    } while (false);

    if (!fResult)
    {
        dwErr = ::GetLastError();
    }

    if (hTokenUIAccess)
    {
        ::CloseHandle(hTokenUIAccess);
    }

    return dwErr;
}

bool CProcessUtils::CreateProcessWithPidToken(
    DWORD pid, 
    const _tstring& strExePath, 
    const _tstring& strCommand,
    const _tstring& strCurDir
)
{
    STARTUPINFOW si = { 0 };
    PROCESS_INFORMATION pi = { 0 };
    HANDLE hProcess = NULL;
    HANDLE hProcessToken = NULL;
    HANDLE hDuplicateToken = NULL;
    bool fResult = false;

    si.cb = sizeof(STARTUPINFO);

    std::wstring exePath = _TStrToWStr(strExePath);
    std::wstring exeCommand = _TStrToWStr(strCommand);
    std::wstring exeCurDir = _TStrToWStr(strCurDir);

    LPCWSTR lpApplicationName = NULL;
    LPWSTR lpCommandLine = NULL;
    LPCWSTR lpCurrentDir = NULL;

    if (!exePath.empty())
    {
        lpApplicationName = exePath.c_str();
    }

    if (!exeCommand.empty())
    {
        lpCommandLine = (LPWSTR)exeCommand.c_str();
    }

    if (!exeCurDir.empty())
    {
        lpCurrentDir = exeCurDir.c_str();
    }

    do
    {
        // 打开进程句柄
        hProcess = ::OpenProcess(GetQueryAccess(), TRUE, pid);
        if (NULL == hProcess)
        {
            break;
        }

        // 打开进程令牌
        DWORD dwOpenDesiredAccess = TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY | TOKEN_QUERY;
        if (!::OpenProcessToken(hProcess, dwOpenDesiredAccess, &hProcessToken))
        {
            break;
        }

        // 模拟登录用户
        if (!::ImpersonateLoggedOnUser(hProcessToken))
        {
            break;
        }

        // 终止客户端应用程序的模拟
        ::RevertToSelf();

        // 创建一个复制现有令牌的新访问令牌
        DWORD dwDuplicateDesiredAccess = TOKEN_ADJUST_DEFAULT | TOKEN_ADJUST_SESSIONID | TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY;
        if (!::DuplicateTokenEx(hProcessToken, dwDuplicateDesiredAccess, NULL, SecurityImpersonation, TokenPrimary, &hDuplicateToken))
        {
            break;
        }

        // 使用令牌创建进程
        // 调用 CreateProcessWithTokenW 的进程必须具有SE_IMPERSONATE_NAME特权。
        if (!::CreateProcessWithTokenW(hDuplicateToken, LOGON_WITH_PROFILE, lpApplicationName, lpCommandLine, 0, NULL, lpCurrentDir, &si, &pi))
        {
            break;
        }

        fResult = true;

        ::CloseHandle(pi.hProcess);
        ::CloseHandle(pi.hThread);

    } while (false);

    if (hDuplicateToken)
    {
        ::CloseHandle(hDuplicateToken);
    }

    if (hProcessToken)
    {
        ::CloseHandle(hProcessToken);
    }

    if (hProcess)
    {
        ::CloseHandle(hProcess);
    }

    return fResult;
}

DWORD CProcessUtils::_CreateUIAccessToken(
    PHANDLE phToken
)
{
    DWORD dwErr = ERROR_SUCCESS;
    HANDLE hTokenSelf = NULL;
    HANDLE hWinlogonToken = NULL;
    BOOL fResult = FALSE;

    do
    {
        DWORD dwDesiredAccess = TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY | TOKEN_ADJUST_DEFAULT;
        DWORD dwSessionId = 0;
        DWORD dwRetLen = 0;
        BOOL bUIAccess = TRUE;

        if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY | TOKEN_DUPLICATE, &hTokenSelf))
        {
            break;
        }

        if (!::GetTokenInformation(hTokenSelf, TokenSessionId, &dwSessionId, sizeof(dwSessionId), &dwRetLen))
        {
            break;
        }

        dwErr = _DuplicateProcessToken(_T("winlogon.exe"), dwSessionId, TOKEN_IMPERSONATE, &hWinlogonToken);
        if (ERROR_SUCCESS != dwErr)
        {
            break;
        }

        if (!::SetThreadToken(NULL, hWinlogonToken))
        {
            break;
        }

        if (!::DuplicateTokenEx(hTokenSelf, dwDesiredAccess, NULL, SecurityAnonymous, TokenPrimary, phToken))
        {
            break;
        }

        if (!::SetTokenInformation(*phToken, TokenUIAccess, &bUIAccess, sizeof(bUIAccess)))
        {
            break;
        }

        ::RevertToSelf();

        fResult = TRUE;

    } while (false);

    if (!fResult)
    {
        if (*phToken)
        {
            ::CloseHandle(*phToken);
            *phToken = NULL;
        }

        dwErr = ::GetLastError();
    }

    if (hWinlogonToken)
    {
        ::CloseHandle(hWinlogonToken);
    }

    if (hTokenSelf)
    {
        ::CloseHandle(hTokenSelf);
    }

    return dwErr;
}

DWORD CProcessUtils::CreateProcessWithUIAccess(
    const _tstring& strExePath,
    const _tstring& strCommand,
    const _tstring& strCurDir/* = _T("")*/
)
{
    return _CreateProcessWithUIAccess(strExePath, strCommand, strCurDir, true);
}

DWORD CProcessUtils::CreateProcessNoUIAccess(
    const _tstring& strExePath,
    const _tstring& strCommand,
    const _tstring& strCurDir/* = _T("")*/
)
{
    return _CreateProcessWithUIAccess(strExePath, strCommand, strCurDir, false);
}

bool CProcessUtils::CreateProcessWithSystem(
    const _tstring& strExePath, 
    const _tstring& strCommand,
    const _tstring& strCurDir/* = _T("")*/
)
{
    std::map<DWORD, PROC_INFO> infos = GetProcessInfos(_T("winlogon.exe"));
    if (infos.empty())
    {
        return false;
    }

    return CreateProcessWithPidToken(infos.begin()->first, strExePath, strCommand, strCurDir);
}

BOOL CProcessUtils::CheckProcessUIAccess(
    DWORD dwPid,
    DWORD *pdwErr, 
    DWORD *pfUIAccess
)
{
    HANDLE hProcess = NULL;
    HANDLE hToken = NULL;
    DWORD dwRetLen = 0;
    BOOL result = FALSE;

    do
    {
        hProcess = ::OpenProcess(PROCESS_QUERY_INFORMATION , FALSE, dwPid);
        if (!hProcess)
        {
            break;
        }

        if (!::OpenProcessToken(hProcess, TOKEN_QUERY, &hToken))
        {
            break;
        }

        if (!::GetTokenInformation(hToken, TokenUIAccess, pfUIAccess, sizeof(*pfUIAccess), &dwRetLen))
        {
            break;
        }

        result = TRUE;

    } while (false);

    if (!result)
    {
        *pdwErr = ::GetLastError();
    }

    if (hProcess)
    {
        ::CloseHandle(hProcess);
    }

    if (hToken)
    {
        ::CloseHandle(hToken);
    }

    return result;
}

void CProcessUtils::_EnumProcessWindows(std::map<HWND, WND_INFO>& infos, HWND hWnd, DWORD dwPid)
{
    HWND childWindow = NULL;
    ULONG nMaxTimes = 0x4000;

    //使用 FindWindowEx 函数搜索子窗口
    while (nMaxTimes > 0 && (childWindow = ::FindWindowEx(hWnd, childWindow, NULL, NULL)))
    {
        ULONG processId = 0;
        ULONG threadId = 0;

        threadId = ::GetWindowThreadProcessId(childWindow, &processId);

        if (dwPid == processId)
        {
            TCHAR szClassBuf[MAX_PATH] = { 0 };
            TCHAR szTextBuf[MAX_PATH] = { 0 };
            ::GetClassName(childWindow, szClassBuf, _countof(szClassBuf));
            ::GetWindowText(childWindow, szTextBuf, _countof(szTextBuf));

            WND_INFO info;
            info.hWnd = childWindow;
            info.dwPid = processId;
            info.dwTid = threadId;
            info.strClass = szClassBuf;
            info.strText = szTextBuf;

            infos.insert(std::make_pair(childWindow, info));
        }

        nMaxTimes--;
    }
}

std::map<HWND, WND_INFO> CProcessUtils::GetProcessWindows(DWORD dwPid)
{
    std::map<HWND, WND_INFO> infos;

    //枚举 桌面窗口 的子窗口
    _EnumProcessWindows(infos, ::GetDesktopWindow(), dwPid);

    //枚举 仅消息窗口 的子窗口
    _EnumProcessWindows(infos, HWND_MESSAGE, dwPid);
    return infos;
}

WND_NODE CProcessUtils::_GetWindowNode(HWND hWnd, std::vector<MODULE_INFO>& modules)
{
    TCHAR szClassBuf[MAX_PATH] = { 0 };
    TCHAR szTextBuf[MAX_PATH] = { 0 };
    ::GetClassName(hWnd, szClassBuf, _countof(szClassBuf));
    ::GetWindowText(hWnd, szTextBuf, _countof(szTextBuf));

    WND_NODE node;
    node.hWnd = hWnd;
    node.strClass = szClassBuf;
    node.strText = szTextBuf;
    node.dwTid = ::GetWindowThreadProcessId(hWnd, &node.dwPid);

    HMODULE hModule = (HMODULE)::GetWindowLongPtr(hWnd, GWLP_HINSTANCE);

    for (const auto& item : modules)
    {
        if (hModule == item.hModule)
        {
            node.strModule = item.strModule;
        }
    }

    HWND childWindow = NULL;
    ULONG nMaxTimes = 0x4000;

    //使用 FindWindowEx 函数搜索子窗口
    while (nMaxTimes > 0 && (childWindow = ::FindWindowEx(hWnd, childWindow, NULL, NULL)))
    {
        node.NodeList.push_back(_GetWindowNode(childWindow, modules));

        nMaxTimes--;
    }

    return node;
}

std::map<HWND, WND_NODE> CProcessUtils::GetProcessWindowNodes(DWORD dwPid)
{
    std::map<HWND, WND_NODE> nodes;
    std::map<HWND, WND_INFO> infos;
    std::vector<MODULE_INFO> moduleInfos  = GetModuleInfos(dwPid);

    //枚举 桌面窗口 的子窗口
    _EnumProcessWindows(infos, ::GetDesktopWindow(), dwPid);

    //枚举 仅消息窗口 的子窗口
    _EnumProcessWindows(infos, HWND_MESSAGE, dwPid);

    for (const auto& item : infos)
    {
        WND_NODE node = _GetWindowNode(item.first, moduleInfos);
        nodes.insert(std::make_pair(item.first, node));
    }

    return nodes;
}