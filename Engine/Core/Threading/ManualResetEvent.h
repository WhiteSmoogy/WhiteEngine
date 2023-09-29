#pragma once

#include <atomic>
#include <cstdint>

namespace white::threading {
	class manual_reset_event
	{
	public:
		manual_reset_event(bool initiallySet = false);

		~manual_reset_event();

		void set() noexcept;

		void reset() noexcept;

		void wait() noexcept;

		bool ready() noexcept { return value.load(std::memory_order_acquire) == 1; }

	private:
		std::atomic<std::uint8_t> value;
	};
}