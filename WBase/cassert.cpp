/*!	\file cassert.cpp
\ingroup WBase
\brief ISO C ¶ÏÑÔ/µ÷ÊÔ¸ú×ÙÀ©Õ¹¡£
*/

#include "cassert.h"
#include <exception> // for std::terminate;
#include <cstdarg>

//todo add platform.h
#if _WIN32
#include <Windows.h>
#endif

#undef wdebug

namespace platform {
	void native_debug(const char* msg) {
#if _WIN32
		OutputDebugStringA(msg);
#else
		std::fprintf(stderr, msg);
#endif
	}

	void wdebug(const char* msg, va_list args) {
		const size_t count = 4096 / sizeof(char);
		char strBuffer[count] = { 0 };
		vsnprintf(strBuffer, count - 1, msg, args);
		native_debug(strBuffer);
	}

	void wdebug(const char* msg, ...) wnothrow {
		const size_t count = 4096 / sizeof(char);
		char strBuffer[count] = { 0 };
		char *p = strBuffer;
		va_list vlArgs;
		va_start(vlArgs, msg);
		vsnprintf(strBuffer, count - 1, msg, vlArgs);
		va_end(vlArgs);
		native_debug(strBuffer);
	}
}

namespace white
{
	void
		wassert(const char* expr_str, const char* file, int line, const char* msg)
		wnothrow
	{
		const auto chk_null([](const char* s) wnothrow{
			return s && *s != char() ? s : "<unknown>";
		});

		platform::wdebug("Assertion failed @ \"%s\":%i:\n"
			" %s .\nMessage: \n%s\n", chk_null(file), line, chk_null(expr_str),
			chk_null(msg));
		std::terminate();
	}

#if WB_Use_WTrace
	void
		wtrace(std::FILE* stream, std::uint8_t lv, std::uint8_t t, const char* file,
			int line, const char* msg, ...) wnothrow
	{
		if (lv < t)
		{
#if _WIN32
			if WB_LIKELY(stream == stderr) {
				platform::wdebug("Trace[%#X] @ \"%s\":%i:\n", unsigned(lv), file,
					line);

				std::va_list args;
				va_start(args, msg);
				platform::wdebug(msg, args);
				va_end(args);
				return;
			}
#endif
			std::fprintf(stream, "Trace[%#X] @ \"%s\":%i:\n", unsigned(lv), file,
				line);

			std::va_list args;
			va_start(args, msg);
			std::vfprintf(stream, msg, args);
			va_end(args);
		}
	}
#endif

} // namespace white;

