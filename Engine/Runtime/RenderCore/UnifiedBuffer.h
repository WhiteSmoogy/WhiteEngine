#pragma once

#include "CoreTypes.h"
#include "RenderInterface/IGraphicsBuffer.hpp"
#include "RenderInterface/ICommandList.h"
#include "ShaderParametersMetadata.h"

import RenderGraph;

namespace platform::Render
{
	struct MemcpyResourceParams
	{
		uint32 Count;
		uint32 SrcOffset;
		uint32 DstOffset;
	};

	struct MemsetResourceParams
	{
		uint32 Count;
		uint32 DstOffset;
		uint32 Value;
	};

	void MemcpyResource(RenderGraph::RGBuilder& Builder, RenderGraph::RGBuffer* DstResource, RenderGraph::RGBuffer* SrcResource, const MemcpyResourceParams& Params);
	void MemcpyResource(RenderGraph::RGBuilder& Builder, RenderGraph::RGBufferUAV* DstResource, RenderGraph::RGBufferSRV* SrcResource, const MemcpyResourceParams& Params);

	void MemsetResource(RenderGraph::RGBuilder& Builder, RenderGraph::RGBufferUAV* DstResource, const MemsetResourceParams& Params);
}
