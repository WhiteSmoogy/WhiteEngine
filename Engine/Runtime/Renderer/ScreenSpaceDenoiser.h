#pragma once

#include <RenderInterface/ICommandList.h>
#include "RenderInterface/ShaderParamterTraits.hpp"
#include "RenderInterface/ShaderParameterStruct.h"
#include "Core/Math/PlatformMath.h"

namespace platform
{
	class ComputeShaderUtils
	{
	public:
		

		static white::math::int3 GetGroupCount(const white::math::int2& ThreadCount, int GroupSize)
		{
			return { white::math::DivideAndRoundUp((int)ThreadCount.x ,GroupSize) ,white::math::DivideAndRoundUp((int)ThreadCount.y, GroupSize),1 };
		}

		template<typename TShaderClass>
		static void Dispatch(Render::CommandList& CmdList, const Render::ShaderRef<TShaderClass>& ComputeShader, const typename TShaderClass::Parameters& Parameters, white::math::int3 GroupCount)
		{
			auto ShaderRHI = ComputeShader.GetComputeShader();
			CmdList.SetComputeShader(ShaderRHI);
			Render::SetShaderParameters(CmdList, ComputeShader, ShaderRHI, Parameters);
			CmdList.DispatchComputeShader(GroupCount.x, GroupCount.y, GroupCount.z);
		}
	};

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