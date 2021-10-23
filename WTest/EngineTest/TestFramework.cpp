#include "TestFramework.h"
#include "WFramework/Helper/Initialization.H"
#include <WFramework/Win32/WCLib/Mingw32.h>
#include <WFramework/Helper/GUIApplication.h>
#include <commctrl.h>

using namespace Test;
using namespace Drawing;
#if WFL_Win32
wconstexpr const double g_max_free_fps(1000);
std::chrono::nanoseconds host_sleep(std::uint64_t(1000000000 / g_max_free_fps));
#endif

LRESULT TestFrameWorkSubclassproc(
	HWND hWnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam,
	UINT_PTR uIdSubclass,
	DWORD_PTR dwRefData
)
{
	auto pFrameWork = reinterpret_cast<TestFrameWork*>(dwRefData);

	if (pFrameWork->SubWndProc(hWnd, uMsg, wParam, lParam))
		return true;

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

TestFrameWork::TestFrameWork(const std::wstring_view & name)
{
	static GUIApplication app;

	auto init_node = Test::FetchRoot()["LTest"]["Init"];

	using namespace Host;

#if WFL_Win32
	p_wnd_thrd.reset(new WindowThread([this] {
		auto ret = unique_ptr<Window>(new Window(CreateNativeWindow(
			WindowClassName, { 1280, 720 }, L"TestFrame", WS_TILED | WS_CAPTION
			| WS_SYSMENU | WS_MINIMIZEBOX),
			app.GetGUIHostRef()));

		if (!SetWindowSubclass(ret->GetNativeHandle(), TestFrameWorkSubclassproc, 0, (DWORD_PTR)this))
		{
			throw platform_ex::Windows::Win32Exception(GetLastError());
		}

		return ret;
	}));


	while (!p_wnd_thrd->GetWindowPtr())
		// TODO: Resolve magic sleep duration.
		std::this_thread::sleep_for(host_sleep);

	p_wnd_thrd->GetWindowPtr()->Show();
	p_wnd_thrd->GetWindowPtr()->MessageMap[WM_DESTROY] += []{
		white::PostQuitMessage(0);
	};

	const auto h_wnd(p_wnd_thrd->GetWindowPtr()->GetNativeHandle());

	app.GetGUIHostRef().MapPoint = [this](const Point& pt) {
		return app.GetGUIHostRef().MapTopLevelWindowPoint(pt);
	};
#endif
}

bool TestFrameWork::SubWndProc(HWND hWnd,
	UINT uMsg,::WPARAM wParam, ::LPARAM lParam)
{
	return false;
}

platform_ex::MessageMap& TestFrameWork::GetMessageMap() {
	return p_wnd_thrd->GetWindowPtr()->MessageMap;
}

TestFrameWork::~TestFrameWork()
{
	OnDestroy();
}

void TestFrameWork::Create()
{
	OnCreate();
}

void TestFrameWork::Run()
{
	auto p_shl = make_shared<Shells::Shell>();
#if WF_Hosted
	FetchGUIHost().ExitOnAllWindowThreadCompleted = true;
#endif
	if WB_UNLIKELY(!FetchAppInstance().Switch(shared_ptr<Shell>(p_shl)))
		throw GeneralEvent("Failed activating the main shell.");
	while (FetchGlobalInstance().DealMessage())
		Update(0);
}

white::uint32 TestFrameWork::Update(white::uint32 pass)
{
	return DoUpdate(pass);
}

void TestFrameWork::OnCreate()
{
}

void TestFrameWork::OnDestroy()
{
}

platform_ex::NativeWindowHandle TestFrameWork ::GetNativeHandle() {
	return p_wnd_thrd->GetWindowPtr()->GetNativeHandle();
}



