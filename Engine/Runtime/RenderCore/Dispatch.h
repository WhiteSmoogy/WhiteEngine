#pragma once
#include <RenderInterface/ICommandList.h>
#include "Core/Math/PlatformMath.h"
#include "ShaderParameterStruct.h"

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
}