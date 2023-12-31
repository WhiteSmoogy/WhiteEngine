#ifndef WE_RENDER_Indirect_h
#define WE_RENDER_Indirect_h 1

#include "WBase/wdef.h"
#include "WBase/winttype.hpp"
#include "WBase/span.hpp"

#include "IGraphicsPipelineState.h"

namespace platform::Render 
{
	enum class IndirectArgumentType
	{
		INDIRECT_DRAW,
		INDIRECT_DRAW_INDEX,
		INDIRECT_DISPATCH,
		INDIRECT_VERTEX_BUFFER,
		INDIRECT_INDEX_BUFFER,
		INDIRECT_CONSTANT,
	};

	struct IndirectArgumentDescriptor
	{
		IndirectArgumentType	mType;
		white::uint32           mIndex;
		white::uint32           mByteSize;
	};

	struct CommandSignatureDesc
	{
		const RootSignature* pRootSignature;
		white::span<const IndirectArgumentDescriptor> ArgDescs;
		/// Set to true if indirect argument struct should not be aligned to 16 bytes
		bool mPacked = true;
	};

	class CommandSignature
	{
	public:
		virtual ~CommandSignature();
	};

	using uint = white::uint32;


	struct DrawArguments
	{
		uint VertexCountPerInstance;
		uint InstanceCount;
		uint StartVertexLocation;
		uint StartInstanceLocation;
	};

	struct DrawIndexArguments
	{
		uint IndexCountPerInstance;
		uint InstanceCount;
		uint StartIndexLocation;
		int  BaseVertexLocation;
		uint StartInstanceLocation;
	};
}

namespace platform_ex::Windows::D3D12
{
	enum class IndirectArgumentType
	{
		INDIRECT_CONSTANT_BUFFER_VIEW = 8192,   
		INDIRECT_SHADER_RESOURCE_VIEW,     
		INDIRECT_UNORDERED_ACCESS_VIEW, 
	};
}

#endif
