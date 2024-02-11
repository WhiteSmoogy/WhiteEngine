export module RenderGraph:builder;

import :fwd;
import :definition;
import :resource;

export namespace RenderGraph
{
	class RGBuilder
	{
	public:
		RGBufferUAVRef CreateUAV(const RGBufferUAVDesc& Desc, ERGUnorderedAccessViewFlags InFlags)
		{
			auto UAV = Views.Allocate<RGBufferUAV>(Allocator, Desc.Buffer->Name, Desc, InFlags);

			return UAV;
		}

		RGBufferSRVRef CreateSRV(const RGBufferSRVDesc& Desc)
		{
			auto SRV = Views.Allocate<RGBufferSRV>(Allocator, Desc.Buffer->Name, Desc);

			return SRV;
		}
	private:
		RGAllocator& Allocator;
		RGViewRegistry Views;
	};
}