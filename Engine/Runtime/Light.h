#pragma once

#include <WBase/wmath.hpp>


namespace WhiteEngine
{
	namespace wm = white::math;

	class Light
	{
	private:
		/** A transform from light space into world space. */
		wm::float4x4 LightToWorld;
		/** A transform from world space into light space. */
		wm::float4x4 WorldToLight;
	public:
		void SetTransform(const wm::float4x4& light2world);

		wm::float3 GetDirection() const;
	};
}