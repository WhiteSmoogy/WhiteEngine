#pragma once
#include <WBase/wmath.hpp>
#include "PlatformMath.h"

namespace WhiteEngine
{
	namespace wm = white::math;

	struct BoxSphereBounds
	{
		BoxSphereBounds()
			:Origin(),
			Extent(),
			Radius(0)
		{
		}

		wm::float3 Origin;
		
		wm::float3 Extent;

		float Radius;

		BoxSphereBounds(wm::float3 origin,wm::float3 extent, float r)
			:Origin(origin),
			Extent(extent),
			Radius(r)
		{
		}
	};
}

namespace white::math
{
	/**
 * Util to calculate distance from a point to a bounding box
 *
 * @param Mins 3D Point defining the lower values of the axis of the bound box
 * @param Max 3D Point defining the lower values of the axis of the bound box
 * @param Point 3D position of interest
 * @return the distance from the Point to the bounding box.
 */
	inline float ComputeSquaredDistanceFromBoxToPoint(const float3& Mins, const float3& Maxs, const float3& Point)
	{
		// Accumulates the distance as we iterate axis
		float DistSquared = 0.f;

		// Check each axis for min/max and add the distance accordingly
		// NOTE: Loop manually unrolled for > 2x speed up
		if (Point.x < Mins.x)
		{
			DistSquared += Square(Point.x - Mins.x);
		}
		else if (Point.x > Maxs.x)
		{
			DistSquared += Square(Point.x - Maxs.x);
		}

		if (Point.y < Mins.y)
		{
			DistSquared += Square(Point.y - Mins.y);
		}
		else if (Point.y > Maxs.y)
		{
			DistSquared += Square(Point.y - Maxs.y);
		}

		if (Point.z < Mins.z)
		{
			DistSquared += Square(Point.z - Mins.z);
		}
		else if (Point.z > Maxs.z)
		{
			DistSquared += Square(Point.z - Maxs.z);
		}

		return DistSquared;
	}
}