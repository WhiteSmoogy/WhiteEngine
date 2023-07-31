#pragma once

#include <RenderInterface/ICommandList.h>
#include "Runtime/RenderCore/ShaderParamterTraits.hpp"
#include "Runtime/RenderCore/ShaderParameterStruct.h"
#include "Core/Math/PlatformMath.h"

namespace platform
{
	class ScreenSpaceDenoiser
	{
	public:
		struct ShadowVisibilityInput
		{
			Render::Texture2D* Mask;
			Render::Texture* SceneDepth;
			Render::Texture2D* WorldNormal;

			float LightHalfRadians;
		};

		struct ShadowVisibilityOutput
		{
			Render::Texture2D* Mask;
			Render::UnorderedAccessView* MaskUAV;
		};

		struct ShadowViewInfo
		{
			unsigned StateFrameIndex;
			white::math::float4x4 ScreenToTranslatedWorld;
			white::math::float4x4 ViewToClip;
			white::math::float4x4 TranslatedWorldToView;
			white::math::float4x4 InvProjectionMatrix;
			white::math::float4 InvDeviceZToWorldZTransform;
		};

		static void DenoiseShadowVisibilityMasks(
			Render::CommandList& CmdList,
			const ShadowViewInfo& ViewInfo,
			const ShadowVisibilityInput& InputParameters,
			const ShadowVisibilityOutput& Output
		);
	};
}