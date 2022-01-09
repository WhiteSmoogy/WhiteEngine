#pragma once

#include "Runtime/Core/Math/BoxSphereBounds.h"
#include <WBase/winttype.hpp>

namespace WhiteEngine
{
	using namespace white::inttype;

	constexpr float SCENE_MAX = 2097152.0;

	class ShadowCascadeSettings
	{
	public:
		float SplitNear = 0;

		float SplitFar = SCENE_MAX;

		int32 ShadowSplitIndex = -1;

		float ShadowCascadeBiasDistribution = 1.0f;

		float FadePlaneOffset = SplitFar;

		float FadePlaneLength = SplitFar-FadePlaneOffset;
	};

	class ProjectedShadowInitializer
	{
	public:
		wm::float4x4 WorldToLight;

		wm::float3 ShadowTranslation;

		wm::float3 Scales;

		wm::float4 WAxis;

		BoxSphereBounds SubjectBounds;
	};

	class WholeSceneProjectedShadowInitializer :public ProjectedShadowInitializer
	{
	public:
		ShadowCascadeSettings CascadeSettings;
	};
}