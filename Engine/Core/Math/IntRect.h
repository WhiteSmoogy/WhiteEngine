#pragma once
#include <WBase/wmath.hpp>
#include <WBase/winttype.hpp>

namespace WhiteEngine
{
	namespace wm = white::math;

	using namespace white::inttype;

	using IntPoint = wm::int2;

	struct IntRect
	{
		IntPoint Min;

		IntPoint Max;

		constexpr IntRect()
			:Min(),Max()
		{}

		constexpr IntRect(IntPoint min,IntPoint max)
			:Min(min),Max(max)
		{}

		constexpr IntRect(int32 x0, int32 y0, int32 x1, int32 y1)
			:Min(x0,y0),Max(x1,y1)
		{
		}

		constexpr IntRect(wm::vector4<int> minmax)
			:Min(minmax.xy),Max(minmax.zw)
		{}

		int Width() const
		{
			return Max.x - Min.x;
		}

		int Height() const
		{
			return Max.y - Min.y;
		}
	};
}