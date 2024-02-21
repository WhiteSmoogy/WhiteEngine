export module RenderGraph:fwd;

export namespace RenderGraph
{
	class RGBuilder;
	class RGAllocator;
	class RGPass;

	class RGPooledBuffer;
	class RGBufferPool;
	class RGConstBuffer;

	class RGUnorderedAccessView;
	class RGShaderResourceView;

	class RGBufferUAV;
	using RGBufferUAVRef = RGBufferUAV*;

	class RGBufferSRV;
	using RGBufferSRVRef = RGBufferSRV*;

	class RGTextureUAV;
	using RGTextureUAVRef = RGTextureUAV*;

	class RGTextureSRV;
	using RGTextureSRVRef = RGTextureSRV*;
}