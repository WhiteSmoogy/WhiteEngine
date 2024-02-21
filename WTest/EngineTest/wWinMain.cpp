
#define TEST_CODE 1

#include "Legacy_Forward.h"
#include "VisBuffer.h"
#include "Runtime/Path.h"
#include "WFramework/Win32/WCLib/Mingw32.h"
#include "Core/Threading/Thread.h"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/msvc_sink.h"

using namespace platform::Render;
using namespace WhiteEngine::Render;
using namespace platform_ex::Windows;

namespace wm = white::math;
namespace we = WhiteEngine;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#ifndef NDEBUG
class DebugBreakSink : public spdlog::sinks::base_sink<std::mutex>
{
protected:
	void sink_it_(const spdlog::details::log_msg& msg) override
	{
		if (IsDebuggerPresent())
		{
			DebugBreak();
		}
	}

	void flush_() override
	{}
};
#endif

void SetupLog()
{
	auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>((we::PathSet::EngineIntermediateDir() / "logs/EngineTest.log").string(), true);
	auto msvc_sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();

#ifndef NDEBUG
	auto break_sink = std::make_shared<DebugBreakSink>();
	break_sink->set_level(spdlog::level::err);
#endif

	spdlog::set_default_logger(white::share_raw(new spdlog::logger("spdlog", { file_sink,msvc_sink
#ifndef NDEBUG
		,break_sink
#endif
		})));
#ifndef NDEBUG
	spdlog::set_level(spdlog::level::debug);
#else
	spdlog::set_level(spdlog::level::info);
#endif

	file_sink->set_level(spdlog::level::info);
	spdlog::flush_every(std::chrono::seconds(5));
	spdlog::flush_on(spdlog::level::warn);
	file_sink->set_pattern("[%H:%M:%S:%e][%^%l%$][thread %t] %v");
	msvc_sink->set_pattern("[thread %t] %v");

	white::FetchCommonLogger().SetSender([=](platform::Descriptions::RecordLevel lv, platform::Logger& logger, const char* str) {
		switch (lv)
		{
		case platform::Descriptions::Emergent:
		case platform::Descriptions::Alert:
		case platform::Descriptions::Critical:
			spdlog::critical(str);
			break;
		case platform::Descriptions::Err:
			spdlog::error(str);
			break;
		case platform::Descriptions::Warning:
			spdlog::warn(str);
			break;
		case platform::Descriptions::Notice:
			file_sink->flush();
		case platform::Descriptions::Informative:
			spdlog::info(str);
			break;
		case platform::Descriptions::Debug:
			spdlog::trace(str);
			break;
		}
		}
	);
}


void SetupD3DDll()
{
	using namespace platform::Descriptions;
	using namespace white::inttype;
	try {
		WCL_CallF_Win32(LoadLibraryW, L"d3d12.dll");
		WCL_CallF_Win32(LoadLibraryW, L"dxgi.dll");
	}
	CatchExpr(platform_ex::Windows::Win32Exception&, TraceDe(Warning, "d3d12 win32 op failed."))
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR cmdLine, int nCmdShow)
{
	SetupLog();
	SetupD3DDll();
	white::threading::SetThreadDescription(GetCurrentThread(), "WhiteMain");

	static auto pInitGuard = WhiteEngine::System::InitGlobalEnvironment();

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	//ForwardTest Test(L"Forward");
	VisBufferTest Test(L"VisBuffer");
	ImGui_ImplWin32_Init(Test.GetNativeHandle());

	Test.Create();
	Test.Run();

	return 0;
}



