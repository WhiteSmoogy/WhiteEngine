module;

#include "WBase/smart_ptr.hpp"

export module RenderGraph;

export import :allocator; 
export import :resource;
export import :resourcepool;
export import :definition; 
export import :builder;

export namespace RenderGraph
{
	white::ref_ptr<RGPooledBuffer> AllocatePooledBuffer(const RGBufferDesc& Desc, const char* Name, ERGPooledBufferAlignment Alignment = ERGPooledBufferAlignment::Page)
	{
		return GRenderGraphResourcePool.FindFreeBuffer(Desc, Name, Alignment);
	}
}