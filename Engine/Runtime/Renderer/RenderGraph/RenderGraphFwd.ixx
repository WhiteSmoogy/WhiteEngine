module;
#include "WBase/winttype.hpp"

export module RenderGraph:fwd;

export namespace RenderGraph
{
	using white::int64;
	using white::int32;
	using white::uint16;
	using white::uint8;

	class RGBuilder;
	class RGAllocator;
	class RGPass;
}