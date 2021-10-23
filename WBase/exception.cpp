#include "exception.h"

namespace white {
	unsupported::~unsupported() = default;


	unimplemented::~unimplemented() = default;

	invalid_construction::invalid_construction()
		: invalid_argument("Violation on construction found.")
	{}

	narrowing_error::~narrowing_error() = default;

	invalid_construction::~invalid_construction() = default;

	void
		throw_invalid_construction()
	{
		throw invalid_construction();
	}

	WB_NORETURN WB_API void white::throw_out_of_range(const char* msg)
	{
		throw std::out_of_range(msg);
	}
}