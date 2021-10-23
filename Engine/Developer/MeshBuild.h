#pragma once

#include "CoreTypes.h"

namespace WhiteEngine {
	constexpr float THRESH_POINTS_ARE_SAME = (0.00002f);	/* Two points are same if within this distance */

	constexpr float THRESH_NORMALS_ARE_SAME = (0.00002f);	/* Two normal points are same if within this distance */

	constexpr float THRESH_UVS_ARE_SAME = (0.0009765625f);/* Two UV are same if within this threshold (1.0f/1024f) */


	/**
	 * Returns true if the specified points are about equal
	*/
	inline bool PointsEqual(const wm::float3& V1, const wm::float3& V2, bool bUseEpsilonCompare = true)
	{
		const float Epsilon = bUseEpsilonCompare ? THRESH_POINTS_ARE_SAME : 0.0f;
		return std::abs(V1.x - V2.x) <= Epsilon && std::abs(V1.y - V2.y) <= Epsilon && std::abs(V1.z - V2.z) <= Epsilon;
	}

	/**
	 * Returns true if the specified normal vectors are about equal
	*/
	inline bool NormalsEqual(const wm::float3& V1, const wm::float3& V2)
	{
		const float Epsilon = THRESH_NORMALS_ARE_SAME;
		return std::abs(V1.x - V2.x) <= Epsilon && std::abs(V1.y - V2.y) <= Epsilon && std::abs(V1.z - V2.z) <= Epsilon;
	}

	inline bool UVsEqual(const wm::float2& V1, const wm::float2& V2)
	{
		const float Epsilon = THRESH_UVS_ARE_SAME;
		return std::abs(V1.x - V2.x) <= Epsilon && std::abs(V1.y - V2.y) <= Epsilon;
	}

	inline bool ColorsEqual(const LinearColor& ColorA, const LinearColor& ColorB,float Tolerance= wm::KindaEpsilon)
	{
		return std::abs(ColorA.r - ColorB.r) < Tolerance && std::abs(ColorA.g - ColorB.g) < Tolerance && std::abs(ColorA.b - ColorB.b) < Tolerance && std::abs(ColorA.a - ColorB.a) < Tolerance;
	}
}
