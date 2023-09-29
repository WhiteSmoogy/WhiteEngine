#include "Thread.h"
#include <WFramework/WCLib/NativeAPI.h>
#include <WFramework/Win32/WCLib/NLS.h>

#if WFL_Win32
	void white::threading::SetThreadDescription(HANDLE hThread, const char* lpThreadDescription)
	{
		// SetThreadDescription is only available from Windows 10 version 1607 / Windows Server 2016
		//
		// So in order to be compatible with older Windows versions we probe for the API at runtime
		// and call it only if available.

		typedef HRESULT(WINAPI* SetThreadDescriptionFnPtr)(HANDLE hThread, PCWSTR lpThreadDescription);

		static SetThreadDescriptionFnPtr RealSetThreadDescription = (SetThreadDescriptionFnPtr)GetProcAddress(GetModuleHandleA("kernel32.dll"), "SetThreadDescription");

		if (RealSetThreadDescription)
		{
			RealSetThreadDescription(hThread, platform_ex::Windows::MBCSToWCS(lpThreadDescription).c_str());
		}
	}
#endif
