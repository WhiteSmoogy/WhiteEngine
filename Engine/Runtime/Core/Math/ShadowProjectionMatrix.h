#pragma once
#include <WBase/wmath.hpp>


namespace WhiteEngine
{
	namespace wm = white::math;

	class ShadowProjectionMatrix :public wm::float4x4
	{
	public:
		explicit ShadowProjectionMatrix(float MinZ, float MaxZ,wm::float4 WAxis)
			:wm::float4x4(
				wm::float4(1, 0, 0, WAxis.x),
				wm::float4(0, 1, 0, WAxis.y),
				wm::float4(0, 0, (WAxis.z* MaxZ + WAxis.w) / (MaxZ - MinZ), WAxis.z),
				wm::float4(0, 0, -MinZ * (WAxis.z * MaxZ + WAxis.w) / (MaxZ - MinZ), WAxis.w)
			)
		{}
	};
}