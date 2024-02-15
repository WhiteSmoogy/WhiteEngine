module;
#include "WBase/winttype.hpp"

export module RenderGraphFwd;

export namespace RenderGraph
{
	using white::int64;
	using white::int32;
	using white::uint16;
	using white::uint8;

	class RGBuilder;
	class RGAllocator;
	class RGPass;

	class RGPooledBuffer;
	class RGBufferPool;

	class RGUnorderedAccessView;
	class RGShaderResourceView;
}