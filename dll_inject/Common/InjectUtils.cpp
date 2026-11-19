#include "InjectUtils.h"
#include <psapi.h>
#include <accctrl.h>
#include <aclapi.h>
#include <sddl.h>

#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

typedef NTSTATUS (WINAPI* PfnNtCreateThreadEx)(
    _Out_ PHANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ LPVOID ObjectAttributes,
    _In_ HANDLE ProcessHandle,
    _In_ LPTHREAD_START_ROUTINE StartRoutine,
    _In_opt_ PVOID Argument,
    _In_ ULONG CreateFlags,
    _In_ SIZE_T ZeroBits,
    _In_ SIZE_T StackSize,
    _In_ SIZE_T MaximumStackSize,
    _In_opt_ LPVOID AttributeList
    );

bool InjectUtils::LoadByRemoteThread(DWORD dwPID, const _tstring& strModulePath)
{
    HANDLE hProcess = NULL;
    HANDLE hThread = NULL;
    PVOID pRemoteMem = NULL;
    SIZE_T path_length = 0;
    bool fResult = false;

    do
    {
        // 修改模块权限
        if (!SetFilePermissions(strModulePath, _T("ALL APPLICATION PACKAGES"), GENERIC_READ | GENERIC_EXECUTE))
        {
            break;
        }

        // 提权
        if (!EnablePrivileges(GetCurrentProcess(), SE_DEBUG_NAME))
        {
            break;
        }

        // 打开进程
        DWORD dwDesiredAccess = PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_CREATE_THREAD;
        hProcess = ::OpenProcess(dwDesiredAccess, FALSE, dwPID);
        if (!hProcess)
        {
            break;
        }

        // 申请远程内存
        path_length = (strModulePath.size() + 1) * sizeof(TCHAR);
        pRemoteMem = ::VirtualAllocEx(hProcess, NULL, path_length, MEM_COMMIT, PAGE_READWRITE);
        if (!pRemoteMem)
        {
            break;
        }

        // 向远程内存写入模块路径
        if (!::WriteProcessMemory(hProcess, pRemoteMem, strModulePath.c_str(), path_length, NULL))
        {
            break;
        }

        // 获取 LoadLibraryW 函数
        FARPROC _LoadLibrary = GetModuleProcAddress(_T("kernel32.dll"), "LoadLibraryW");
        if (!_LoadLibrary)
        {
            break;
        }

        // 获取 NtCreateThreadEx 函数
        PfnNtCreateThreadEx _NtCreateThreadEx = (PfnNtCreateThreadEx)GetModuleProcAddress(_T("Ntdll.dll"), "NtCreateThreadEx");
        if (!_NtCreateThreadEx)
        {
            break;
        }

        // 创建远程线程
        if (!NT_SUCCESS(_NtCreateThreadEx(&hThread, PROCESS_ALL_ACCESS, NULL, hProcess, (LPTHREAD_START_ROUTINE)_LoadLibrary, pRemoteMem, 0, 0, 0, 0, NULL)))
        {
            break;
        }

        // 等待线程结束
        if (WAIT_OBJECT_0 != ::WaitForSingleObject(hThread, INFINITE))
        {
            break;
        }

        // 获取返回值
        DWORD dwExitCode = 0;
        ::GetExitCodeThread(hThread, &dwExitCode);
        if (!dwExitCode)
        {
            break;
        }

        fResult = true;

    } while (false);

    // 关闭线程句柄
    if (hThread)
    {
        ::CloseHandle(hThread);
    }

    // 释放远程内存
    if (pRemoteMem)
    {
        ::VirtualFreeEx(hProcess, pRemoteMem, 0, MEM_RELEASE);
    }

    // 关闭进程句柄
    if (hProcess)
    {
        ::CloseHandle(hProcess);
    }

    return fResult;
}

bool InjectUtils::UnloadByRemoteThread(DWORD dwPID, const _tstring& strModulePath)
{
    HANDLE hProcess = NULL;
    HANDLE hThread = NULL;
    bool fResult = false;

    do
    {
        // 提权
        if (!EnablePrivileges(GetCurrentProcess(), SE_DEBUG_NAME))
        {
            break;
        }

        // 打开进程
        DWORD dwDesiredAccess = PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ;
        hProcess = ::OpenProcess(dwDesiredAccess, FALSE, dwPID);
        if (!hProcess)
        {
            break;
        }

        // 获取指定模块句柄
        HANDLE hTargetModule = GetProcessModuleHandle(hProcess, strModulePath);
        if (!hTargetModule)
        {
            break;
        }

        // 获取 FreeLibrary 函数
        FARPROC _FreeLibrary = GetModuleProcAddress(_T("kernel32.dll"), "FreeLibrary");
        if (NULL == _FreeLibrary)
        {
            break;
        }

        // 获取 NtCreateThreadEx 函数
        PfnNtCreateThreadEx _NtCreateThreadEx = (PfnNtCreateThreadEx)GetModuleProcAddress(_T("Ntdll.dll"), "NtCreateThreadEx");
        if (!_NtCreateThreadEx)
        {
            break;
        }

        // 创建远程线程
        if (!NT_SUCCESS(_NtCreateThreadEx(&hThread, PROCESS_ALL_ACCESS, NULL, hProcess, (LPTHREAD_START_ROUTINE)_FreeLibrary, (LPVOID)hTargetModule, 0, 0, 0, 0, NULL)))
        {
            break;
        }

        // 等待线程结束
        if (WAIT_OBJECT_0 != ::WaitForSingleObject(hThread, INFINITE))
        {
            break;
        }

        // 获取返回值
        DWORD dwExitCode = 0;
        ::GetExitCodeThread(hThread, &dwExitCode);
        if (!dwExitCode)
        {
            break;
        }

        fResult = true;

    } while (false);

    // 关闭线程句柄
    if (hThread)
    {
        ::CloseHandle(hThread);
    }

    // 关闭进程句柄
    if (hProcess)
    {
        ::CloseHandle(hProcess);
    }

    return fResult;
}

bool InjectUtils::EnablePrivileges(
    HANDLE hProcess, 
    LPCTSTR lpPrivilegesName
)
{
    HANDLE hToken = NULL;
    LUID luidValue = { 0 };
    TOKEN_PRIVILEGES tokenPrivileges = { 0 };
    BOOL fSuccess = FALSE;

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

        fSuccess = true;

    } while (false);

    if (hToken)
    {
        ::CloseHandle(hToken);
    }

    return fSuccess;
}

FARPROC InjectUtils::GetModuleProcAddress(LPCTSTR lpModulePath, LPCSTR lpProcName)
{
    FARPROC ProcAddr = NULL;

    // 获取模块句柄
    HMODULE hModule = ::GetModuleHandle(lpModulePath);
    if (!hModule)
    {
        return NULL;
    }

    // 获取函数地址
    ProcAddr = ::GetProcAddress(hModule, lpProcName);

    return ProcAddr;
}

HMODULE InjectUtils::GetProcessModuleHandle(HANDLE hProcess, const _tstring& strModulePath)
{
    HMODULE* phMods = NULL;
    HMODULE hModule = NULL;
    DWORD cbNeeded = 0;
    DWORD dwModSize = 0;

    do
    {
        // 枚举进程模块
        if (!::EnumProcessModulesEx(hProcess, NULL, NULL, &cbNeeded, LIST_MODULES_ALL))
        {
            break;
        }

        // 分配枚举进程模块所需内存
        dwModSize = cbNeeded;
        phMods = (HMODULE*)::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, dwModSize);
        if (NULL == phMods)
        {
            break;
        }

        // 枚举进程模块
        if (!::EnumProcessModulesEx(hProcess, phMods, dwModSize, &cbNeeded, LIST_MODULES_ALL))
        {
            break;
        }

        // 检索模块
        size_t nModuleCnt = cbNeeded / sizeof(HMODULE);
        for (size_t i = 0; i < nModuleCnt; i++)
        {
            TCHAR szModName[MAX_PATH] = { 0 };
            if (::GetModuleFileNameEx(hProcess, phMods[i], szModName, _countof(szModName)))
            {
                if (0 == _tcsicmp(szModName, strModulePath.c_str()))
                {
                    hModule = phMods[i];
                    break;
                }
            }
        }

    } while (false);

    // 释放模块句柄内存
    if (phMods)
    {
        ::HeapFree(::GetProcessHeap(), 0, phMods);
    }

    return hModule;
}

bool InjectUtils::SetFilePermissions(const _tstring& strPath, const _tstring& strUser, DWORD dwPermissions)
{
    TCHAR szUserName[MAX_PATH] = { 0 };
    PSECURITY_DESCRIPTOR pSD = NULL;
    EXPLICIT_ACCESS eaAccess = { 0 };
    SECURITY_INFORMATION siInfo = DACL_SECURITY_INFORMATION;
    PACL pOldDACL = NULL;
    PACL pNewDACL = NULL;
    DWORD dwResult = ERROR_SUCCESS;
    bool fResult = false;

    do
    {
        // 获取指向现有DACL的指针
        dwResult = ::GetNamedSecurityInfo(strPath.c_str(), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, &pOldDACL, NULL, &pSD);
        if (dwResult != ERROR_SUCCESS)
        {
            break;
        }

        _tcscpy_s(szUserName, _countof(szUserName), strUser.c_str());

        eaAccess.grfAccessPermissions = dwPermissions;
        eaAccess.grfAccessMode = SET_ACCESS;
        eaAccess.grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
        eaAccess.Trustee.TrusteeForm = TRUSTEE_IS_NAME;
        eaAccess.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
        eaAccess.Trustee.ptstrName = szUserName;

        // 创建一个新的ACL，将新的ACE合并到现有的DACL中。
        dwResult = ::SetEntriesInAcl(1, &eaAccess, pOldDACL, &pNewDACL);
        if (ERROR_SUCCESS != dwResult)
        {
            break;
        }

        // DACL 将新的ACL附加为对象的DACL。
        dwResult = ::SetNamedSecurityInfo((LPTSTR)strPath.c_str(), SE_FILE_OBJECT, siInfo, NULL, NULL, pNewDACL, NULL);
        if (ERROR_SUCCESS != dwResult)
        {
            break;
        }

        fResult = true;

    } while (false);

    if (pSD != NULL)
    {
        ::LocalFree((HLOCAL)pSD);
    }
    if (pNewDACL != NULL)
    {
        ::LocalFree((HLOCAL)pNewDACL);
    }

    return fResult;
}

bool InjectUtils::SetRegistryPermissions(const _tstring& strPath, const _tstring& strUser, DWORD dwPermissions)
{
    TCHAR szUserName[MAX_PATH] = { 0 };
    PSECURITY_DESCRIPTOR pSD = NULL;
    EXPLICIT_ACCESS eaAccess = { 0 };
    SECURITY_INFORMATION siInfo = DACL_SECURITY_INFORMATION;
    PACL pOldDACL = NULL;
    PACL pNewDACL = NULL;
    DWORD dwResult = ERROR_SUCCESS;
    bool fResult = false;

    do
    {
        // 获取指向现有DACL的指针
        dwResult = ::GetNamedSecurityInfo(strPath.c_str(), SE_REGISTRY_KEY, DACL_SECURITY_INFORMATION, NULL, NULL, &pOldDACL, NULL, &pSD);
        if (dwResult != ERROR_SUCCESS)
        {
            break;
        }

        _tcscpy_s(szUserName, _countof(szUserName), strUser.c_str());

        eaAccess.grfAccessPermissions = dwPermissions;
        eaAccess.grfAccessMode = SET_ACCESS;
        eaAccess.grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
        eaAccess.Trustee.TrusteeForm = TRUSTEE_IS_NAME;
        eaAccess.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
        eaAccess.Trustee.ptstrName = (LPWSTR)szUserName;

        // 创建一个新的ACL，将新的ACE合并到现有的DACL中。
        dwResult = ::SetEntriesInAcl(1, &eaAccess, pOldDACL, &pNewDACL);
        if (ERROR_SUCCESS != dwResult)
        {
            break;
        }

        // DACL 将新的ACL附加为对象的DACL。
        dwResult = ::SetNamedSecurityInfo((LPTSTR)strPath.c_str(), SE_REGISTRY_KEY, siInfo, NULL, NULL, pNewDACL, NULL);
        if (ERROR_SUCCESS != dwResult)
        {
            break;
        }

        fResult = true;

    } while (false);

    if (pSD != NULL)
    {
        ::LocalFree((HLOCAL)pSD);
    }

    if (pNewDACL != NULL)
    {
        ::LocalFree((HLOCAL)pNewDACL);
    }

    return fResult;
}