#include <iostream>
#include "../Common/InjectUtils.h"

int main()
{
    InjectUtils::LoadByRemoteThread(13764, _T(R"(D:\FC_Gitee\dll_inject\dll_inject\Bin\Dll_Demo.dll)"));
    InjectUtils::UnloadByRemoteThread(13764, _T(R"(D:\FC_Gitee\dll_inject\dll_inject\Bin\Dll_Demo.dll)"));

    return 0;
}
