#pragma once
#include "RenderInterface/ICommandList.h"
#include "Core/Math/PlatformMath.h"
#include "ShaderParameterStruct.h"
#include "RenderInterface/PipelineStateUtility.h"
#include "RenderInterface/DeviceCaps.h"

import RenderGraph;

namespace ComputeShaderUtils
{
	using namespace platform;
	using namespace RenderGraph;

	inline white::math::int3 GetGroupCount(const white::math::int2& ThreadCount, int GroupSize)
	{
		return { white::math::DivideAndRoundUp((int)ThreadCount.x ,GroupSize) ,white::math::DivideAndRoundUp((int)ThreadCount.y, GroupSize),1 };
	}

	template<typename TShaderClass>
	inline void Dispatch(Render::ComputeCommandList& CmdList, const Render::ShaderRef<TShaderClass>& ComputeShader, const typename TShaderClass::Parameters& Parameters, white::math::int3 GroupCount)
	{
		auto ShaderRHI = ComputeShader.GetComputeShader();
		SetComputePipelineState(CmdList, ShaderRHI);
		Render::SetShaderParameters(CmdList, ComputeShader, ShaderRHI, Parameters);
		CmdList.DispatchComputeShader(GroupCount.x, GroupCount.y, GroupCount.z);
	}

	inline void ValidateGroupCount(const white::math::int3& GroupCount)
	{
		wassume((uint32)GroupCount.x <= Render::Caps.MaxDispatchThreadGroupsPerDimension.x);
		wassume((uint32)GroupCount.y <= Render::Caps.MaxDispatchThreadGroupsPerDimension.y);
		wassume((uint32)GroupCount.z <= Render::Caps.MaxDispatchThreadGroupsPerDimension.z);
	}

	template<typename TShaderClass>
	inline RGPassRef AddPass(
		RGBuilder& GraphBuilder,
		RGEventName&& PassName,
		ERGPassFlags PassFlags,
		const Render::ShaderRef<TShaderClass>& ComputeShader,
		const Render::ShaderParametersMetadata* ParametersMetadata,
		typename TShaderClass::Parameters* Parameters,
		white::math::int3 GroupCount)
	{
		WAssert(
			white::has_anyflags(PassFlags, white::enum_or(ERGPassFlags::Compute, ERGPassFlags::AsyncCompute)) &&
			!white::has_anyflags(PassFlags, white::enum_or(ERGPassFlags::Copy, ERGPassFlags::Raster)), "AddPass only supports 'Compute' or 'AsyncCompute'.");

		ValidateGroupCount(GroupCount);

		return GraphBuilder.AddPass(
			std::move(PassName),
			ParametersMetadata,
			Parameters,
			PassFlags,
			[ParametersMetadata, Parameters, ComputeShader, GroupCount](platform::Render::ComputeCommandList& CmdList)
			{
				ComputeShaderUtils::Dispatch(CmdList, ComputeShader, *Parameters, GroupCount);
			});
	}

	template <typename TShaderClass>
	inline RGPassRef AddPass(
		RGBuilder& GraphBuilder,
		RGEventName&& PassName,
		const Render::ShaderRef<TShaderClass>& ComputeShader,
		typename TShaderClass::Parameters* Parameters,
		white::math::int3 GroupCount)
	{
		auto* ParametersMetadata = TShaderClass::Parameters::TypeInfo::GetStructMetadata();
		return AddPass(GraphBuilder, std::move(PassName), ERGPassFlags::Compute, ComputeShader, ParametersMetadata, Parameters, GroupCount);
	}
}

