#include "SpinWait.h"
#include <WFramework/WCLib/Platform.h>
#include <WFramework/WCLib/NativeAPI.h>
#include <thread>

namespace
{
	namespace local
	{
		constexpr std::uint32_t yield_threshold = 10;
	}
}

namespace WhiteEngine
{
	SpinWait::SpinWait() noexcept
	{
		Reset();
	}

	bool SpinWait::NextSpinWillYield() const noexcept
	{
		return count >= local::yield_threshold;
	}

	void SpinWait::Reset() noexcept
	{
		static const std::uint32_t initialCount =
			std::thread::hardware_concurrency() > 1 ? 0 : local::yield_threshold;
		count = initialCount;
	}

	void SpinWait::SpinOne() noexcept
	{
#if WFL_Win32
		// Spin strategy taken from .NET System.SpinWait class.
		// I assume the Microsoft developers knew what they're doing.
		if (!NextSpinWillYield())
		{
			// CPU-level pause
			// Allow other hyper-threads to run while we busy-wait.

			// Make each busy-spin exponentially longer
			const std::uint32_t loopCount = 2u << count;
			for (std::uint32_t i = 0; i < loopCount; ++i)
			{
				::YieldProcessor();
				::YieldProcessor();
			}
		}
		else
		{
			// We've already spun a number of iterations.
			//
			const auto yieldCount = count - local::yield_threshold;
			if (yieldCount % 20 == 19)
			{
				// Yield remainder of time slice to another thread and
				// don't schedule this thread for a little while.
				::SleepEx(1, FALSE);
			}
			else if (yieldCount % 5 == 4)
			{
				// Yield remainder of time slice to another thread
				// that is ready to run (possibly from another processor?).
				::SleepEx(0, FALSE);
			}
			else
			{
				// Yield to another thread that is ready to run on the
				// current processor.
				::SwitchToThread();
			}
		}
#else
		if (next_spin_will_yield())
		{
			std::this_thread::yield();
		}
#endif

		++count;
		if (count == 0)
		{
			// Don't wrap around to zero as this would go back to
			// busy-waiting.
			count = local::yield_threshold;
		}
	}
}