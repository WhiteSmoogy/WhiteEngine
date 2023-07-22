#pragma once
/*
\note 该文件实际上约束了左手坐标系的轴向描述
*/
#include <WBase/wmath.hpp>


namespace WhiteEngine
{
	namespace wm = white::math;

	struct Rotator
	{
		/** Rotation around the right axis (around X axis), Looking up and down (0=Straight Ahead, +Up, -Down) */
		float Pitch;

		/** Rotation around the up axis(around Y axis), Running in circles 0=North, +East, -West..*/
		float Yaw;

		/** Rotation around the forward axis (around Z axis), Tilting your head, 0=Straight, +Clockwise, -CCW. */
		float Roll;

		explicit Rotator(wm::float3 normal_direction)
		{
			Roll = 0;

			Yaw =std::atan2(-normal_direction.x,normal_direction.z) * (180.f / wm::PI);

			Pitch = std::atan2(normal_direction.y,std::sqrt(normal_direction.x*normal_direction.x+normal_direction.z*normal_direction.z) ) * (180.f / wm::PI);

		}
	};
}