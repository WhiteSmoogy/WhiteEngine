#pragma once

#include "ICommandList.h"

namespace platform::Render {
	void SetGraphicsPipelineState(CommandList& cmdlist, const GraphicsPipelineStateInitializer& initializer);
	void SetComputePipelineState(ComputeCommandList& cmdlist, const ComputeHWShader* Shader);


	namespace PipelineStateCache
	{
		class FGraphicsPipelineState;
		class FComputePipelineState;

		FGraphicsPipelineState* GetAndOrCreateGraphicsPipelineState(CommandList& cmdlist, const GraphicsPipelineStateInitializer& initializer);
		FComputePipelineState* GetAndOrCreateComputePipelineState(ComputeCommandList& cmdlist, const ComputeHWShader* Shader);
	}
}