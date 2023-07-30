#pragma once

#include "CoreTypes.h"
#include "RenderInterface/IGraphicsBuffer.hpp"
#include "RenderInterface/ICommandList.h"
#include "ShaderParametersMetadata.h"

namespace platform::Render
{
	ConstantBuffer* CreateConstantBuffer(const void* Contents, Buffer::Usage Usage, const ShaderParametersMetadata& Layout);

	template<typename TBufferStruct>
		requires requires{ TBufferStruct::TypeInfo::GetStructMetadata(); }
	class GraphicsBufferRef
	{
	public:
		static GraphicsBufferRef<TBufferStruct> CreateGraphicsBuffeImmediate(const TBufferStruct& Value, Buffer::Usage Usage)
		{
			return GraphicsBufferRef<TBufferStruct>(CreateConstantBuffer(&Value, Usage, *TBufferStruct::TypeInfo::GetStructMetadata()));
		}

		operator std::shared_ptr<ConstantBuffer>()
		{
			return buffer;
		}

		ConstantBuffer* Get() const
		{
			return buffer.get();
		}
	private:
		GraphicsBufferRef(ConstantBuffer* InBuffer)
			:buffer(InBuffer, RObjectDeleter())
		{
		}

		std::shared_ptr<ConstantBuffer> buffer;
	};

	template<typename TBufferStruct>
		requires requires{ TBufferStruct::TypeInfo::GetStructMetadata(); }
	GraphicsBufferRef<TBufferStruct> CreateGraphicsBuffeImmediate(const TBufferStruct& Value, Buffer::Usage Usage)
	{
		return GraphicsBufferRef<TBufferStruct>::CreateGraphicsBuffeImmediate(Value, Usage);
	}

	struct MemcpyResourceParams
	{
		uint32 Count;
		uint32 SrcOffset;
		uint32 DstOffset;
	};

	template<typename ResourceType>
	void MemcpyResource(Render::CommandList& cmdList, const ResourceType& DstBuffer, const ResourceType& SrcBuffer, const MemcpyResourceParams& Params);
}
