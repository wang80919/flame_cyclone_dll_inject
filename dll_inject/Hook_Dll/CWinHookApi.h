#pragma once
#include <windows.h>
#include <Xinput.h>
#include <stdint.h>

#define XUSER_MAX_COUNT                 4

// 共享内存名
#define SHARE_MEM_NAME _T("103638CA-BC11-4949-811B-A282E7F67AEC")

// 管道通信协议
enum ePipeCmd :uint32_t {
    eNone,          // 无动作
    eHook,          // Hook
    eUnHook,        // 取消Hook
    eExit,          // 退出进程
};

// 共享数据结构
typedef struct _SHARE_DATA_PROTO
{
    XINPUT_STATE XinputState[XUSER_MAX_COUNT];          // 控制器状态
    BOOL fEnable[XUSER_MAX_COUNT];                      // 控制器启用状态
    BOOL HoldA[XUSER_MAX_COUNT];                        // A键按住
    BOOL HoldB[XUSER_MAX_COUNT];                        // B键按住
    BOOL HoldX[XUSER_MAX_COUNT];                        // X键按住
    BOOL HoldY[XUSER_MAX_COUNT];                        // Y键连发
    BOOL HoldLB[XUSER_MAX_COUNT];                       // LB键按住
    BOOL HoldRB[XUSER_MAX_COUNT];                       // RB键连发
    BOOL BurstA[XUSER_MAX_COUNT];                       // A键连发
    BOOL BurstB[XUSER_MAX_COUNT];                       // B键连发
    BOOL BurstX[XUSER_MAX_COUNT];                       // X键连发
    BOOL BurstY[XUSER_MAX_COUNT];                       // Y键连发
    BOOL BurstLB[XUSER_MAX_COUNT];                      // LB键按住
    BOOL BurstRB[XUSER_MAX_COUNT];                      // RB键连发
    BOOL fMerge;                                        // 混合输入
    BOOL fHook;                                         // HOOK输入
}SHARE_DATA_PROTO, *PSHARE_DATA_PROTO;

#pragma pack(1)
typedef struct _PIPE_DATA_PROTO
{
    ePipeCmd        uCmd;
    uint32_t        uParam;

    _PIPE_DATA_PROTO()
        :
        uCmd(ePipeCmd::eNone),
        uParam(0)
    {

    }

}PIPE_DATA_PROTO;
#pragma pack()

#ifdef Hook_Dll_EXPORTS
#define Hook_Dll_API __declspec(dllexport)
#else
#define Hook_Dll_API __declspec(dllimport)
#endif

extern "C"
{
    // 安装API HOOK
    Hook_Dll_API BOOL HookInitialize();

    // 卸载API HOOK
    Hook_Dll_API VOID HookUninitialize(BOOL fDestroyShareData = FALSE);

    // 加载钩子
    Hook_Dll_API HHOOK HookLoad(DWORD dwThreadId);

    // 卸载钩子
    Hook_Dll_API BOOL HookUnload();

    // 是否 HOOK Xinput 输入
    Hook_Dll_API BOOL HookIsEnable();

    // 启用 HOOK 输入
    Hook_Dll_API BOOL HookEnable(BOOL fEnable);

    // 是否启用 Xinput 输入
    Hook_Dll_API BOOL HookIsEnableXinput(DWORD dwUserIndex);

    // 启用 Xinput 输入
    Hook_Dll_API BOOL HookEnableXinput(DWORD dwUserIndex, BOOL fEnable);

    // 是否混合 Xinput 输入
    Hook_Dll_API BOOL HookIsMergeXinput();

    // 混合 Xinput 输入
    Hook_Dll_API BOOL HookMergeXinput(BOOL fEnable);

    // 获取 Xinput 输入状态
    Hook_Dll_API DWORD HookGetXinputState(DWORD dwUserIndex, XINPUT_STATE* pState);

    // 设置 Xinput 输入状态
    Hook_Dll_API VOID HookSetXinputState(DWORD dwUserIndex, XINPUT_STATE* pState);

    // 获取共享数据
    Hook_Dll_API PSHARE_DATA_PROTO HookGetShareData();
}
