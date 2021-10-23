#include "Logger.h"
#if WFL_Win32
#include <csignal>
#include <WFramework/WCLib/NativeAPI.h>
#include <WFramework/Win32/WCLib/Consoles.h>
#include <WFramework/Win32/WCLib/NLS.h>
#endif
#if WF_Multithread == 1
#include <WBase/concurrency.h>
#endif
#include <WBase/cformat.h>
#include <iostream>
#include <cstdarg>

namespace platform
{
	namespace
	{
		inline const char*
			chk_null(const char* s)
		{
			return s && *s != char() ? s : "<unknown>";
		}

#if WF_Multithread == 1
		std::string
			FetchCurrentThreadID() wnothrow
		{
			TryRet(white::get_this_thread_id())
				// XXX: Nothing more can be done.
				CatchIgnore(...)
				return{};
		}
#endif

#if WFL_Win32
		WB_NONNULL(3) size_t
			WConsoleOutput(wstring& wstr, unsigned long h, const char* str)
		{
			using namespace platform_ex;

			wstr = platform_ex::Windows::UTF8ToWCS(str) + L'\n';
			return platform_ex::Windows::WConsole(h).WriteString(wstr);
		}
#endif

		using namespace Concurrency;

	} // unnamed namespace;
}

using namespace platform;

namespace platform_ex
{
#if WF_Multithread == 1
	void
		LogAssert(const char* expr_str, const char* file, int line,
			const char* msg) wnothrow
	{
		//#	if WFL_Android
				//::__android_log_assert(expr_str, "YFramework",
					//"Assertion failed @ \"%s\":%i:\n %s .\nMessage: \n%s\n", file, line,
					//expr_str, msg);
		//#	else
#		if WFL_Win32
		try
		{
			char prog[MAX_PATH]{ "<unknown>" };

			::GetModuleFileNameA({}, prog, MAX_PATH);

			const auto& errstr(sfmt("Assertion failed @ program %s: "
				"\"%s\":%i:\n %s .\nMessage: \n%s\n", prog, chk_null(file),
				line, chk_null(expr_str), chk_null(msg)));

			::OutputDebugStringA(errstr.c_str());
			// XXX: Not safe in windows procedure, but not used in YFramework.
			// TODO: Use custom windows creation?
			switch (::MessageBoxA({}, errstr.c_str(),
				"LeoEngine Runtime Assertion", MB_ABORTRETRYIGNORE | MB_ICONHAND
				| MB_SETFOREGROUND | MB_TASKMODAL))
			{
			case IDIGNORE:
				return;
			case IDABORT:
				std::raise(SIGABRT);
			default:
				break;
			}
			std::terminate();
		}
		catch (...)
		{
			WF_TraceRaw(Descriptions::Emergent, "Unknown exception found.");
			white::wassert(expr_str, file, line, msg);
		}
#		endif
		TryExpr(FetchCommonLogger().AccessRecord([=] {
			white::wassert(expr_str, file, line, msg);
			}))
			catch (...)
			{
				std::fprintf(stderr, "Fetch logger failed.");
				std::fflush(stderr);
				white::wassert(expr_str, file, line, msg);
			}
	}
#endif

#if WFL_Win32
	const char* sfmt(Descriptions::RecordLevel lv) {
		switch (lv) {
		case Logger::Level::Emergent:
			return "Emergent";
		case Logger::Level::Alert:
			return "Alert";
		case Logger::Level::Critical:
			return "Critical";
		case Logger::Level::Err:
			return "Err";
		case Logger::Level::Warning:
			return "Warning";
		case Logger::Level::Notice:
			return "Notice";
		case Logger::Level::Informative:
			return "Informative";
		case Logger::Level::Debug:
			return "Debug";
		}
		return "Unknown";
	}

	void
		SendDebugString(Descriptions::RecordLevel lv, const char* str) wnothrowv
	{
		try
		{
			const auto& t_id(FetchCurrentThreadID());

			// TODO: Use %::WaitForDebugEventEx if possible. See https://msdn.microsoft.com/en-us/library/windows/desktop/mt171594(v=vs.85).aspx.
			::OutputDebugStringA((!t_id.empty() ? white::sfmt("[%s:%s]: %s\n",
				t_id.c_str(), sfmt(lv), Nonnull(str)) : white::sfmt(
					"[%s]: %s\n", sfmt(lv), Nonnull(str))).c_str());
		}
		CatchIgnore(...)
	}

#endif
}