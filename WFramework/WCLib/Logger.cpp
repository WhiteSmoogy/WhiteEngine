#include "Logger.h"
#include <WFramework/WCLib/Host.h>
#if WFL_Win32
#include <csignal>
#include <WFramework/WCLib/NativeAPI.h>
#include <WFramework/Win32/WCLib/Consoles.h>
#include <WFramework/Win32/WCLib/NLS.h>
#endif
#if WF_Multithread == 1
#include <WBase/concurrency.h>
#endif
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

	bool
		Echo(string_view sv) wnoexcept(false)
	{
#if WFL_Win32
		wstring wstr;
		size_t n(0);

		TryExpr(n = WConsoleOutput(wstr, STD_OUTPUT_HANDLE, sv.data()))
			CatchIgnore(platform_ex::Windows::Win32Exception&)
			if (n < wstr.length())
				std::wcout << &wstr[n];
		return bool(std::wcout.flush());
#elif WF_Hosted
		return bool(std::cout << platform_ex::EncodeArg(sv) << std::endl);
#else
		return bool(std::cout << sv << std::endl);
#endif
	}

	Logger::Logger(const Logger& logger)
		: FilterLevel(logger.FilterLevel), filter(logger.filter),
		sender(logger.sender)
	{}
	Logger::Logger(Logger&& logger) wnothrow
		: Logger()
	{
		swap(logger, *this);
	}

	void
		Logger::SetFilter(Filter f)
	{
		if (f)
			filter = std::move(f);
	}
	void
		Logger::SetSender(Sender s)
	{
		if (s)
			sender = std::move(s);
	}

	bool
		Logger::DefaultFilter(Level lv, Logger& logger) wnothrow
	{
		return lv <= logger.FilterLevel;
	}

	void
		Logger::DefaultSendLog(Level lv, Logger& logger, const char* str) wnothrowv
	{
		SendLog(std::cerr, lv, logger, str);
	}

	void
		Logger::DefaultSendLogToFile(Level lv, Logger& logger, const char* str)
		wnothrowv
	{
		SendLogToFile(stderr, lv, logger, str);
	}

	void
		Logger::DoLog(Level level, const char* str)
	{
		if (str)
		{
			lock_guard<recursive_mutex> lck(record_mutex);

			DoLogRaw(level, str);
		}
	}

	void
		Logger::DoLogRaw(Level level, const char* str)
	{
		sender(level, *this, Nonnull(str));
	}

	void
		Logger::DoLogException(Level lv, const std::exception& e) wnothrow
	{
		const auto do_log_excetpion_raw([this](const char* msg) WB_NONNULL(1)
			wnothrow {
			try
			{
				DoLogRaw(Descriptions::Emergent,
					"Another exception thrown when handling exception.");
				DoLogRaw(Descriptions::Emergent, msg);
			}
			CatchExpr(...,
				white::wtrace(stderr, Descriptions::Emergent, Descriptions::Notice,
					__FILE__, __LINE__, "Logging error: unhandled exception."))
		});
		const auto& msg(e.what());
		lock_guard<recursive_mutex> lck(record_mutex);

		try
		{
			// XXX: Provide no throw guarantee and put it out of the critical
			//	section?
			// XXX: Log demangled type name.
			DoLogRaw(lv, sfmt("<%s>: %s", typeid(e).name(), msg));
		}
		CatchExpr(std::exception& ex, do_log_excetpion_raw(ex.what()))
			CatchExpr(..., do_log_excetpion_raw({}))
	}

	Logger::Sender
		Logger::FetchDefaultSender(string_view tag)
	{
		WAssertNonnull(tag.data());
#if WFL_Win32
		return [](Level lv, Logger& logger, const char* str) {
			// TODO: Avoid throwing of %WriteString here for better performance?
			// FIXME: Output may be partially updated?
			try
			{
				wstring wstr;

				WConsoleOutput(wstr, STD_ERROR_HANDLE, str);
			}
			CatchExpr(platform_ex::Windows::Win32Exception&, DefaultSendLog(lv, logger, str))
		};
#endif
		return DefaultSendLog;
	}

	void
		Logger::SendLog(std::ostream& os, Level lv, Logger&, const char* str)
		wnothrowv
	{
		try
		{
#if WF_Multithread == 1
			const auto& t_id(FetchCurrentThreadID());

			if (!t_id.empty())
				os << white::sfmt("[%s:%#X]: %s\n", t_id.c_str(), unsigned(lv),
					Nonnull(str));
			else
#endif
				os << white::sfmt("[%#X]: %s\n", unsigned(lv), Nonnull(str));
			os.flush();
		}
		CatchIgnore(...)
	}

	void
		Logger::SendLogToFile(std::FILE* stream, Level lv, Logger&, const char* str)
		wnothrowv
	{
		WAssertNonnull(stream);
#if WF_Multithread == 1
		const auto& t_id(FetchCurrentThreadID());

		if (!t_id.empty())
			std::fprintf(stream, "[%s:%#X]: %s\n", t_id.c_str(), unsigned(lv),
				Nonnull(str));
		else
#endif
			std::fprintf(stream, "[%#X]: %s\n", unsigned(lv), Nonnull(str));
		std::fflush(stream);
	}

	void
		swap(Logger& x, Logger& y) wnothrow
	{
		// TODO: Wait for C++17.
		// XXX: See discussion in LWG 2062.
#if !__GLIBCXX__
		wnoexcept_assert("Unsupported luanguage implementation found.",
			x.filter.swap(y.filter));
		wnoexcept_assert("Unsupported luanguage implementation found.",
			x.sender.swap(y.sender));
#endif

		std::swap(x.FilterLevel, y.FilterLevel);
		x.filter.swap(y.filter);
		x.sender.swap(y.sender);
	}


	Logger&
		FetchCommonLogger()
	{
		static Logger logger;

		return logger;
	}


	string
		LogWithSource(const char* file, int line, const char* fmt, ...) wnothrow
	{
		try
		{
			std::va_list args;

			va_start(args, fmt);

			string str(vsfmt(fmt, args));

			va_end(args);
			return sfmt("\"%s\":%i:", chk_null(file), line) + std::move(str);
		}
		CatchExpr(...,
			white::wtrace(stderr, Descriptions::Emergent, Descriptions::Notice,
				chk_null(file), line, "LogWithSource error: unhandled exception."))
			return{};
	}

} // namespace platform;

