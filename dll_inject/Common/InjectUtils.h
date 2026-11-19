#pragma once

#include <string>
#include <windows.h>
#include <tchar.h>

#ifdef _UNICODE
using _tstring = std::wstring;
#else
using _tstring = std::string;
#endif

class InjectUtils
{
public:

    // 远程线程方式为指定进程加载 DLL 模块
    static bool LoadByRemoteThread(DWORD dwPID, const _tstring& strModulePath);

    // 远程线程方式为指定进程卸载 DLL 模块
    static bool UnloadByRemoteThread(DWORD dwPID, const _tstring& strModulePath);

private:

    static bool EnablePrivileges(HANDLE hProcess, LPCTSTR lpPrivilegesName);
    static FARPROC GetModuleProcAddress(LPCTSTR lpModulePath, LPCSTR lpProcName);
    static HMODULE GetProcessModuleHandle(HANDLE hProcess, const _tstring& strModulePath);
    static bool SetFilePermissions(const _tstring& strPath, const _tstring& strUser, DWORD dwPermissions);
    static bool SetRegistryPermissions(const _tstring& strPath, const _tstring& strUser, DWORD dwPermissions);
};

