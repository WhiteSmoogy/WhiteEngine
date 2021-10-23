#pragma once
#include <WBase/wmath.hpp>


namespace WhiteEngine
{
	namespace wm = white::math;

	class ScaleMatrix :public wm::float4x4
	{
	public:
		explicit ScaleMatrix(wm::float3 scale)
			:wm::float4x4(
				wm::float4(scale.x,  0,     0,      0),
				wm::float4(0,     scale.y,  0,      0),
				wm::float4(0,        0,    scale.z, 0),
				wm::float4(0,        0,      0,     1)
			)
		{}
	};
}