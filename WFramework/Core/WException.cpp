#include <WBase/wdef.h>
#include "WException.h"
#include <WBase/exception.h>

namespace white {

	LoggedEvent::LoggedEvent(const char* str, RecordLevel lv)
		: GeneralEvent(Nonnull(str)),
		level(lv)
	{}
	LoggedEvent::LoggedEvent(string_view sv, RecordLevel lv)
		: GeneralEvent(std::string(sv)),
		level(lv)
	{}
	LoggedEvent::LoggedEvent(const GeneralEvent& e, RecordLevel lv)
		: GeneralEvent(e),
		level(lv)
	{}
	ImplDeDtor(LoggedEvent)

		RecordLevel LoggedEvent::GetLevel() const wnothrow
	{
		return level;
	}

		FatalError::FatalError(const char* t, string_view c)
		: GeneralEvent(Nonnull(t)),
		content((Nonnull(c.data()), make_shared<string>(string(c))))
	{}
	ImplDeDtor(FatalError)

		string_view	FatalError::GetContent() const wnothrow
	{
		return Deref(content);
	}

	const char* FatalError::GetTitle() const wnothrow
	{
		return what();
	}


		void
		TraceException(const char* str, RecordLevel lv, size_t level) wnothrow
	{
		TryExpr(
			WF_TraceRaw(lv, "%s%s", std::string(level, ' ').c_str(), Nonnull(str)))
			CatchExpr(..., WF_TraceRaw(Critical, "Failure @ TraceException."))
	}

	void
		TraceExceptionType(const std::exception& e, RecordLevel lv) wnothrow
	{
		WFL_Log(lv, [](const std::exception& e) {return white::sfmt("Caught std::exception[%s].", typeid(e).name()); },e);
	}

	void
		ExtractAndTrace(const std::exception& e, RecordLevel lv) wnothrow
	{
		TraceExceptionType(e, lv);
		ExtractException(TraceException, e, lv);
	}

	void
		ExtractException(const ExtractedLevelPrinter& print, const std::exception& e,
			RecordLevel lv, size_t level) wnothrow
	{
		TryExpr(print(e.what(), lv, level))
			CatchExpr(..., print("Exception occurred when printing @ ExtractException.",
				Critical, level))
			// FIXME: Following code only tested OK for %YCL_Win32.
			TryExpr(white::handle_nested(e,
				[&, lv, level](std::exception& ex) wnothrow{
			ExtractException(print, ex, lv, level + 1);
		}))
			CatchExpr(..., print("Unknown nested exception found nested on calling"
				" white::handle_nested @ ExtractException.", Critical, level))
	}

	bool
		TryExecute(std::function<void()> f, const char* desc, RecordLevel lv,
			ExceptionTracer trace)
	{
		try
		{
			TryExpr(f())
				catch (...)
			{
				if (desc)
					WF_TraceRaw(Notice, "Exception filtered: %s.", desc);
				throw;
			}
			return{};
		}
		CatchExpr(std::exception& e, trace(e, lv))
			return true;
	}
}