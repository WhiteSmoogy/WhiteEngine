#pragma once


#include "RenderInterface/ITexture.hpp"
#include "RenderInterface/IGraphicsBuffer.hpp"
#include "RenderInterface/IGPUResourceView.h"
#include "RenderInterface/TextureSampleDesc.h"

#include "ShaderParametersMetadata.h"

namespace platform::Render
{
	struct RayTracingShaderBindings
	{
		Texture* Textures[32] = {};
		ShaderResourceView* SRVs[32] = {};
		ConstantBuffer* UniformBuffers[32] = {};
		TextureSampleDesc* Samplers[32] = {};
		UnorderedAccessView* UAVs[8] = {};
	};

	struct RayTracingShaderBindingsWriter : RayTracingShaderBindings
	{
		std::shared_ptr<ConstantBuffer> RootUniformBuffer;

		void SetTexture(uint16 BaseIndex, Texture* Value)
		{
			Textures[BaseIndex] = Value;
		}

		void SetShaderSampler(uint16 BaseIndex, TextureSampleDesc* Value)
		{
			Samplers[BaseIndex] = Value;
		}

		void SetUAVParameter(uint16 BaseIndex, UnorderedAccessView* Value)
		{
			UAVs[BaseIndex] = Value;
		}

		void SetGraphicsBuffer(uint16 BaseIndex, ConstantBuffer* Value)
		{
			UniformBuffers[BaseIndex] = Value;
		}
	};

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
}