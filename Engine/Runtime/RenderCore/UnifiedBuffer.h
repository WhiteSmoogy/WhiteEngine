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
		uint32 Count;     // Bytes to copy.
		uint32 SrcOffset; // Byte offset in the source buffer.
		uint32 DstOffset; // Byte offset in the destination buffer.
	};

	struct MemsetResourceParams
	{
		uint32 Count;     // Bytes to fill.
		uint32 DstOffset; // Byte offset in the destination buffer.
		uint32 Value;
	};

	void MemcpyResource(RenderGraph::RGBuilder& Builder, RenderGraph::RGBuffer* DstResource, RenderGraph::RGBuffer* SrcResource, const MemcpyResourceParams& Params);
	void MemcpyResource(RenderGraph::RGBuilder& Builder, RenderGraph::RGBufferUAV* DstResource, RenderGraph::RGBufferSRV* SrcResource, const MemcpyResourceParams& Params);

	void MemsetResource(RenderGraph::RGBuilder& Builder, RenderGraph::RGBufferUAV* DstResource, const MemsetResourceParams& Params);
}
