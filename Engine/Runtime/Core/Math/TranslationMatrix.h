#pragma once
#include <WBase/wmath.hpp>


namespace WhiteEngine
{
	namespace wm = white::math;

	class TranslationMatrix :public wm::float4x4
	{
	public:
		explicit TranslationMatrix(wm::float3 delta)
			:
			wm::float4x4(
				wm::float4(1, 0, 0, 0),
				wm::float4(0, 1, 0, 0),
				wm::float4(0, 0, 1, 0),
				wm::float4(delta,   1)
			)
		{
		}
	};
}