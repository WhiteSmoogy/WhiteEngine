#pragma once

#include "WBase/sutility.h"
#include "WBase/winttype.hpp"
#include <string_view>
#include <WFramework/WCLib/NativeAPI.h>
#include <WFramework/Helper/WindowThread.h>

namespace Test {
	using namespace white;


	//简化测试代码框架的基类,理论上:
	//游戏应该由启动器载入逻辑模块来接管程序，这也可作为启动器和逻辑模块的初步抽象接口。

	//Create 
	//Run

	//OnCreate
	//OnDestroy()
	//DoUpdate()

	class TestFrameWork :white::noncopyable {
	public:
		//更新函数所做的状态或者需要完成的事情 但是并不能默认不写Return啊
		enum UpdateRet {
			Nothing = 0,
		};

	public:
		explicit TestFrameWork(const std::wstring_view &name);

		virtual ~TestFrameWork();

		void Create();
		void Run();

		platform_ex::NativeWindowHandle GetNativeHandle();

		std::thread::native_handle_type GetThreadNativeHandle()
		{
			return p_wnd_thrd->GetNativeHandle();
		}

		platform_ex::MessageMap& GetMessageMap();

		virtual bool SubWndProc(HWND hWnd,
			UINT uMsg, ::WPARAM wParam, ::LPARAM lParam);
	protected:
		white::uint32 Update(white::uint32 pass);

	private:
		virtual void OnCreate();
		virtual void OnDestroy();
		virtual white::uint32 DoUpdate(white::uint32 pass) = 0;

#if WFL_Win32
		unique_ptr<Host::WindowThread> p_wnd_thrd;
#endif

	};
}