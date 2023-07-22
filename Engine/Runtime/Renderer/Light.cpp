#include "Light.h"

using namespace WhiteEngine;

void Light::SetTransform(const wm::float4x4& light2world)
{
	LightToWorld = light2world;

	WorldToLight = wm::inverse(LightToWorld);
}

wm::float3 Light::GetDirection() const
{
	return wm::float3(
		WorldToLight[0][2],
		WorldToLight[1][2],
		WorldToLight[2][2]);
}