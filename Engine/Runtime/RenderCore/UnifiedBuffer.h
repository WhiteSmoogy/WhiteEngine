#pragma once

#include "CoreTypes.h"
#include "RenderInterface/IGraphicsBuffer.hpp"
#include "RenderInterface/ICommandList.h"
#include "ShaderParametersMetadata.h"

namespace platform::Render
{
	ConstantBuffer* CreateConstantBuffer(const void* Contents, Buffer::Usage Usage, const ShaderParametersMetadata& Layout);

	template<typename TStruct>
	std::shared_ptr<ConstantBuffer> CreateConstantBuffer(const std::vector<TStruct>& Contents, Buffer::Usage Usage)
	{
		static_assert(white::IsAligned(sizeof(TStruct), 16));
		ShaderParametersMetadata Layout(static_cast<uint32>(Contents.size() * sizeof(TStruct)), {});
		return shared_raw_robject(CreateConstantBuffer(Contents.data(), Usage, Layout));
	}


	template<typename TBufferStruct>
	class ConstantBufferRef
	{
	public:
		static ConstantBufferRef<TBufferStruct> CreateGraphicsBuffeImmediate(const TBufferStruct& Value, Buffer::Usage Usage)
		{
			if constexpr (requires { TBufferStruct::TypeInfo::GetStructMetadata; })
				return ConstantBufferRef<TBufferStruct>(CreateConstantBuffer(&Value, Usage, *TBufferStruct::TypeInfo::GetStructMetadata()));
			else
			{
				ShaderParametersMetadata Layout(white::Align<uint32>(sizeof(TBufferStruct), 16), {});

				return ConstantBufferRef<TBufferStruct>(CreateConstantBuffer(&Value, Usage, Layout));
			}
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
		ConstantBufferRef(ConstantBuffer* InBuffer)
			:buffer(InBuffer, RObjectDeleter())
		{
		}

		std::shared_ptr<ConstantBuffer> buffer;
	};

	template<typename TBufferStruct>
	ConstantBufferRef<TBufferStruct> CreateGraphicsBuffeImmediate(const TBufferStruct& Value, Buffer::Usage Usage)
	{
		return ConstantBufferRef<TBufferStruct>::CreateGraphicsBuffeImmediate(Value, Usage);
	}

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

	template<typename ResourceType>
	void MemcpyResource(CommandList& cmdList, const ResourceType& DstBuffer, const ResourceType& SrcBuffer, const MemcpyResourceParams& Params);

	template<typename ResourceType>
	void MemsetResource(CommandList& cmdList, const ResourceType& DstBuffer, const MemsetResourceParams& Params);
}
