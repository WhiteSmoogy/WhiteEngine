#include "HostWindow.h"
#include "GUIApplication.h"

namespace white
{
	using namespace Drawing;

#if WF_Hosted
	namespace Host
	{

		Window::Window(NativeWindowHandle h_wnd)
			: Window(h_wnd, FetchGUIHost())
		{}
		Window::Window(NativeWindowHandle h_wnd, GUIHost& h)
			: HostWindow(h_wnd), host(h)
#if WFL_Win32
			, InputHost(*this)
#endif
		{
			h.AddMappedItem(h_wnd, make_observer(this));
		}
		Window::~Window()
		{
			host.get().RemoveMappedItem(GetNativeHandle());
		}

#	endif

	} // namespace Host;

} // namespace white;
