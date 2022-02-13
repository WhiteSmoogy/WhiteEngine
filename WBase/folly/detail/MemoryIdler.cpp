#include "MemoryIdler.h"

namespace folly::detail
{
	AtomicStruct<std::chrono::steady_clock::duration>
		MemoryIdler::defaultIdleTimeout(std::chrono::seconds(5));
}