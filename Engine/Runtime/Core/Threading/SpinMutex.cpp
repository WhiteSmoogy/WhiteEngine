#include "SpinMutex.h"
#include "SpinWait.h"

namespace WhiteEngine
{
	SpinMutex::SpinMutex() noexcept
		: locked(false)
	{
	}

	bool SpinMutex::try_lock() noexcept
	{
		return !locked.exchange(true, std::memory_order_acquire);
	}

	void SpinMutex::lock() noexcept
	{
		SpinWait wait;
		while (!try_lock())
		{
			while (locked.load(std::memory_order_relaxed))
			{
				wait.SpinOne();
			}
		}
	}

	void SpinMutex::unlock() noexcept
	{
		locked.store(false, std::memory_order_release);
	}
}