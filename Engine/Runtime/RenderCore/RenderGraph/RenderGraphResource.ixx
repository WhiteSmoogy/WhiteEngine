module;
#include "RenderInterface/IFormat.hpp"
#include "WBase/winttype.hpp"
#include "RenderInterface/RenderObject.h"
#include "RenderInterface/IGPUResourceView.h"
#include "RenderInterface/IGraphicsBuffer.hpp"
#include "Runtime/RenderCore/ShaderParametersMetadata.h"
#include "RenderInterface/ICommandList.h"

export module RenderGraph:resource;

import RenderGraphFwd;
import :definition;

import "WBase/cassert.h";

export namespace RenderGraph
{
	class RGView;
	using RGViewHandle = RGHandle<RGView, uint16>;
	using RGViewRegistry = RGHandleRegistry<RGViewHandle, ERGHandleRegistryDestructPolicy::Never>;

	class RGBuffer;
	using RGBufferHandle = RGHandle<RGBuffer, white::uint16>;
	using RGBufferRegistry = RGHandleRegistry<RGBufferHandle, ERGHandleRegistryDestructPolicy::Registry>;

	class RGConstBuffer;
	using RGConstBufferHandle = RGHandle<RGConstBuffer, white::uint16>;
	using RGConstBufferRegistry = RGHandleRegistry<RGConstBufferHandle>;

	class RGPass;
	using RGPassHandle = RGHandle<RGPass, uint16>;
	using RGPassRegistry = RGHandleRegistry<RGPassHandle>;

	using namespace platform::Render;
	class RGResource
	{
	public:
		RGResource(const RGResource&) = delete;

		virtual ~RGResource() = default;

		// Name of the resource for debugging purpose.
		const char* const Name = nullptr;

		RObject* GetRObject() const
		{
			return RealObj;
		}
	protected:
		RGResource(const char* InName)
			:Name(InName)
		{}

		bool HasRObject() const
		{
			return RealObj != nullptr;
		}
	private:
		RObject* RealObj = nullptr;

		friend RGBuilder;
	};

	enum class ERGViewableResourceType : uint8
	{
		Texture,
		Buffer,
		MAX
	};

	class RGViewableResource :public RGResource
	{
	public:
		const ERGViewableResourceType Type;

	protected:
		/** Whether this is an externally registered resource. */
		uint8 bExternal : 1 = 0;

		uint8 bForceNonTransient : 1 = 0;

		uint8 bTransient : 1 = 0;

		uint8 bQueuedForUpload : 1 = 0;

		RGViewableResource(const char* Name, ERGViewableResourceType InType)
			:RGResource(Name), Type(InType)
		{}

	private:
		friend RGBuilder;
	};

	enum class ERGViewType : uint8
	{
		TextureUAV,
		TextureSRV,
		BufferUAV,
		BufferSRV,
		MAX
	};

	class RGView :public RGResource
	{
	protected:
		RGView(const char* InName, ERGViewType InType)
			:RGResource(InName), Type(InType)
		{}
	private:
		ERGViewType Type;
		RGViewHandle Handle;

		RGPassHandle LastPass;

		friend RGViewRegistry;
		friend RGBuilder;
	};

	using RGViewRef = RGView*;

	enum class ERGUnorderedAccessViewFlags : white::uint8
	{
		None = 0,

		SkipBarrier = 1 << 0
	};

	enum class ERGBufferFlags : uint8
	{
		None = 0,

		MultiFrame = 1 << 0,

		SkipTracking = 1 << 1,

		ForceImmediateFirstBarrier = 1 << 2,
	};

	enum class ERGPooledBufferAlignment : uint8
	{
		None,

		Page,

		PowerOfTwo
	};

	class RGUnorderedAccessView : public RGView
	{
	public:
		const ERGUnorderedAccessViewFlags Flags;

		UnorderedAccessView* GetRObject() const
		{
			return static_cast<UnorderedAccessView*>(GetRObject());
		}

	protected:
		RGUnorderedAccessView(const char* InName, ERGViewType InType, ERGUnorderedAccessViewFlags InFlag)
			:RGView(InName, InType), Flags(InFlag)
		{}

		bool bExternal : 1;

		friend RGBuilder;
	};

	class RGShaderResourceView : public RGView
	{
	public:
		ShaderResourceView* GetRObject() const
		{
			return static_cast<ShaderResourceView*>(GetRObject());
		}
	protected:
		RGShaderResourceView(const char* InName, ERGViewType InType)
			:RGView(InName, InType)
		{}
	};

	struct RGBufferDesc
	{
		uint32 Usage = 0;
		uint32 BytesPerElement = 1;
		uint32 NumElements = 1;

		static RGBufferDesc CreateStructIndirectDesc(uint32 BytesPerElement, uint32 NumElements)
		{
			RGBufferDesc Desc;
			Desc.Usage = EAccessHint::DrawIndirect | EAccessHint::GPUReadWrite | EAccessHint::Structured | EAccessHint::UAV;
			Desc.BytesPerElement = BytesPerElement;
			Desc.NumElements = NumElements;
			return Desc;
		}

		static RGBufferDesc CreateByteAddressDesc(uint32 NumBytes)
		{
			wassume(NumBytes % 4 == 0);
			RGBufferDesc Desc;
			Desc.Usage = EAccessHint::Raw | EAccessHint::GPUReadWrite | EAccessHint::Structured | EAccessHint::UAV;
			Desc.BytesPerElement = 4;
			Desc.NumElements = NumBytes / 4;
			return Desc;
		}

		static RGBufferDesc CreateStructuredDesc(uint32 BytesPerElement, uint32 NumElements)
		{
			RGBufferDesc Desc;
			Desc.Usage = EAccessHint::GPUReadWrite | EAccessHint::Structured | EAccessHint::UAV;
			Desc.BytesPerElement = BytesPerElement;
			Desc.NumElements = NumElements;
			return Desc;
		}

		uint32 GetSize() const
		{
			return NumElements * BytesPerElement;
		}

		uint32 HashCode() const
		{
			auto Hash64 = white::hash_combine_seq(0, BytesPerElement, NumElements, Usage);

			return (uint32)Hash64 + ((uint32)(Hash64 >> 32) * 23);
		}

		auto operator<=>(const RGBufferDesc&) const = default;

		friend RGBufferDesc operator |(RGBufferDesc lhs, platform::Render::EAccessHint access)
		{
			lhs.Usage |= static_cast<uint32>(access);

			return lhs;
		}
	};

	class RGPooledBuffer final :public white::ref_count_base
	{
	public:
		RGPooledBuffer(CommandList& CmdList, GraphicsBufferRef InBuffer, const RGBufferDesc& InDesc, uint32 InAlignNumElements, const char* InName)
			:Desc(InDesc)
			, AlignNumElements(InAlignNumElements)
			, Buffer(InBuffer)
			, Name(InName)
		{}

		GraphicsBuffer* GetRObject() const
		{
			return Buffer.get();
		}

		uint32 GetSize() const
		{
			return Desc.GetSize();
		}

	private:
		RGBufferDesc GetAlignDesc() const
		{
			auto AlignDesc = Desc;
			AlignDesc.NumElements = AlignNumElements;
			return AlignDesc;
		}

		const RGBufferDesc Desc;
		uint32 AlignNumElements;

		GraphicsBufferRef Buffer;
		const char* Name = nullptr;

		uint32 LastUsedFrame = 0;


		friend RGBuilder;
		friend RGAllocator;

		friend RGBufferPool;
	};

	class RGBuffer final :public RGViewableResource
	{
	public:
		static const ERGViewableResourceType StaticType = ERGViewableResourceType::Buffer;

		RGBufferDesc Desc;
		const ERGBufferFlags Flags;

		GraphicsBuffer* GetRObject()
		{
			return static_cast<GraphicsBuffer*>(RGViewableResource::GetRObject());
		}

		uint32 GetAccess() const
		{
			return Desc.Usage;
		}

	private:
		RGBuffer(const char* Name, const RGBufferDesc& InDesc, ERGBufferFlags InFlags)
			:RGViewableResource(Name, StaticType), Desc(InDesc), Flags(InFlags)
		{}

		RGBufferHandle Handle;

		white::ref_ptr<RGPooledBuffer> Allocation;

		RGPassHandle FirstPass;

		friend RGBuilder;
		friend RGBufferRegistry;
		friend RGAllocator;
	};

	using RGBufferRef = RGBuffer*;

	struct RGBufferUAVDesc
	{
		RGBufferRef Buffer = nullptr;
		EFormat Format = EFormat::EF_Unknown;
	};

	class RGBufferUAV final :public RGUnorderedAccessView
	{
	public:
		static const ERGViewType StaticType = ERGViewType::BufferUAV;

		const RGBufferUAVDesc Desc;

		RGBufferRef GetParent() const
		{
			return Desc.Buffer;
		}

	private:
		RGBufferUAV(const char* InName, const RGBufferUAVDesc& InDesc, ERGUnorderedAccessViewFlags InFlag)
			:RGUnorderedAccessView(InName, StaticType, InFlag), Desc(InDesc)
		{
		}

		friend RGBuilder;
		friend RGViewRegistry;
		friend RGAllocator;
	};

	using RGBufferUAVRef = RGBufferUAV*;

	using RGBufferSRVDesc = RGBufferUAVDesc;

	class RGBufferSRV final :public RGShaderResourceView
	{
	public:
		static const ERGViewType StaticType = ERGViewType::BufferSRV;

		const RGBufferSRVDesc Desc;

		RGBufferRef GetParent() const
		{
			return Desc.Buffer;
		}

	private:
		RGBufferSRV(const char* InName, const RGBufferSRVDesc& InDesc)
			:RGShaderResourceView(InName, StaticType), Desc(InDesc)
		{
		}

		friend RGBuilder;
		friend RGViewRegistry;
		friend RGAllocator;
	};

	using RGBufferSRVRef = RGBufferSRV*;

	class RGConstBuffer : public RGResource
	{
	public:
		ConstantBuffer* GetRObject() const
		{
			return static_cast<ConstantBuffer*>(RGResource::GetRObject());
		}

		const RGParameterStruct& GetParameters() const
		{
			return ParameterStruct;
		}
	protected:
		template<typename TStruct>
		explicit RGConstBuffer(const TStruct* InParameters, const char* InName)
			:RGResource(InName), ParameterStruct(InParameters, RGParameterStruct::GetStructMetadata<TStruct>()) , Size(ParameterStruct.GetSize())
		{
		}

		template<typename TStruct>
		explicit RGConstBuffer(const TStruct* InParameters, uint32 InSize, const char* InName)
			:RGResource(InName), ParameterStruct(InParameters, RGParameterStruct::GetStructMetadata<TStruct>()) , Size(InSize)
		{
		}

		uint32 GetSize() const { return Size; }

	private:
		RGConstBufferHandle Handle;
		const RGParameterStruct ParameterStruct;
		uint32 Size;

		friend RGBuilder;
		friend RGConstBufferRegistry;
		friend RGAllocator;
	};

	using RGConstBufferRef = RGConstBuffer*;

	template<typename TBufferStruct>
	class RGTConstBuffer : public RGConstBuffer
	{
	public:
		const TBufferStruct* Contents() const
		{
			return Parameters;
		}

		const TBufferStruct* operator->() const
		{
			return Parameters;
		}

		TBufferStruct* operator->()
		{
			return Parameters;
		}

	private:
		explicit RGTConstBuffer(TBufferStruct* InParameters, const char* InName)
			: RGConstBuffer(InParameters, InName)
			, Parameters(InParameters)
		{}

		TBufferStruct* Parameters;

		friend RGBuilder;
		friend RGConstBufferRegistry;
		friend RGAllocator;
	};

	template<typename TBufferStruct>
	struct RGTConstBufferRef
	{
		RGTConstBuffer<TBufferStruct>* CBuffer;

		const TBufferStruct* operator->() const
		{
			return CBuffer->operator->();
		}

		TBufferStruct* operator->()
		{
			return CBuffer->operator->();
		}

		const TBufferStruct* Contents() const
		{
			return CBuffer->Contents();
		}

		RGConstBuffer* Get() const
		{
			return CBuffer;
		}
	};
}