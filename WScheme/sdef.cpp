#include "sdef.h"
#include <WFramework/WCLib/Platform.h>

#ifdef WS_BUILD_DLL
#ifdef WFL_Win32
#include <Windows.h>
BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	return TRUE;
}

#endif
#endif