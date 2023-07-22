#pragma once

#include <WBase/winttype.hpp>

namespace WhiteEngine
{
	class SpinWait
	{
	public:
		SpinWait() noexcept;

		bool NextSpinWillYield() const noexcept;

		void SpinOne() noexcept;

		void Reset() noexcept;
	private:
		white::uint32 count;
	};
}