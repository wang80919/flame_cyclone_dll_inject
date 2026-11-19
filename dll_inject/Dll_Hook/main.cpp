#include <tchar.h>
#include <windows.h>
#include "Win32Utils/CNamedPipe.h"
#include "../Hook_Dll/CWinHookApi.h"

#ifdef _WIN64
#pragma comment(lib, "../Bin/Hook_Dll.lib")
#else
#pragma comment(lib, "../Bin/Hook_Dll_x86.lib")
#endif

int _tmain(int argc, LPCTSTR argv[])
{
	PIPE_DATA_PROTO     proto;
	CNamedPipe          pipe;
	LPCTSTR             lpPipeName = NULL;

	if (argc < 2)
	{
		return -1;
	}

	lpPipeName = argv[1];

	// 打开通信管道
	if (!pipe.Open(lpPipeName, 5000))
	{
		return -1;
	}

	HookUninitialize();
	HookLoad(0);

	PSHARE_DATA_PROTO pConfig = HookGetShareData();
	bool    m_fHookQuit = false;
	while(!m_fHookQuit)
	{
		// 读取管道数据
		_tprintf(_T("Read......\n"));
		if (!pipe.Read(&proto, sizeof(proto)))
		{
			_tprintf(_T("Error\n"));
			break;
		}

		_tprintf(_T("uCmd: %d uParam: %d\n"), proto.uCmd, proto.uParam);
		switch (proto.uCmd)
		{
		case ePipeCmd::eHook:
		{
			HookLoad(proto.uParam);
		}
		break;
		case ePipeCmd::eUnHook:
		{
			HookUnload();
		}
		break;
		case ePipeCmd::eExit:
		{
            m_fHookQuit = true;
		}
		break;
		default:
			break;
		}

	} while (false);

	pipe.Close();

	PSHARE_DATA_PROTO lpData = (PSHARE_DATA_PROTO)HookGetShareData();
	if (lpData)
	{
		memset(lpData->XinputState, 0, sizeof(lpData));
	}

	pConfig->fHook = false;
	HookUnload();

	return 0;
}
