#pragma once

#ifndef WE_RENDER_DATASTRUCTURES_H
#define WE_RENDER_DATASTRUCTURES_H 1

#include <WBase/winttype.hpp>
#include <WBase/wmath.hpp>

namespace WhiteEngine::Render {
	using namespace white::inttype;
	namespace wm = white::math;

	using LightIndex = uint16;

	enum DirectLightType {
		POINT_LIGHT = 0,
		SPOT_LIGHT =1,
		DIRECTIONAL_LIGHT =2
	};

	struct DirectLight {
		//direction for spot and directional lights (world space).
		wm::float3 direction;
		//range for point and spot lights(Maximum distance of influence)
		float range;
		//position for spot and point lights(world Space)
		wm::float3 position;
		//outer angle for spot light(radian)
		float outerangle;
		//blub size for point light or inneragnle for spot light
		float blub_innerangle;

		//The color of emitted light(linear RGB color)
		wm::float3 color;
		//
		uint32 type;
	};
}

#endif