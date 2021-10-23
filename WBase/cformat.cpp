#include "cformat.h"

namespace white
{
	size_t
		vfmtlen(const char* fmt, std::va_list args) wnothrow
	{
		wconstraint(fmt);

		const int l(std::vsnprintf({}, 0, fmt, args));

		return size_t(l < 0 ? -1 : l);
	}
	size_t
		vfmtlen(const wchar_t* fmt, std::va_list args) wnothrow
	{
		wconstraint(fmt);

		const int l(std::vswprintf({}, 0, fmt, args));

		return size_t(l < 0 ? -1 : l);
	}
}