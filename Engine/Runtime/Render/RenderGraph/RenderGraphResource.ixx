module;
#include "RenderInterface/IFormat.hpp"
#include "WBase/winttype.hpp"
#include "RenderInterface/RenderObject.h"
#include "RenderInterface/IGPUResourceView.h"
#include "RenderInterface/IGraphicsBuffer.hpp"
#include "Runtime/RenderCore/ShaderParametersMetadata.h"

export module RenderGraph:resource;

import :fwd;
import :definition;

import "WBase/cassert.h";

export namespace RenderGraph
{
	class RGView;
	using RGViewHandle = RGHandle<RGView,uint16>;
	using RGViewRegistry = RGHandleRegistry<RGViewHandle, ERGHandleRegistryDestructPolicy::Never>;

	class RGBuffer;
	using RGBufferHandle = RGHandle<RGBuffer, white::uint16>;
	using RGBufferRegistry = RGHandleRegistry<RGBufferHandle, ERGHandleRegistryDestructPolicy::Registry>;

	class RGConstBuffer;
	using RGConstBufferHandle = RGHandle<RGConstBuffer, white::uint16>;
	using RGConstBufferRegistry = RGHandleRegistry<RGConstBufferHandle>;

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

	private:
		RObject* RealObj = nullptr;
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
		uint8 bExternal : 1;

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

		friend RGViewRegistry;
		friend RGBuilder;
	};

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
			:RGView(InName, InType),Flags(InFlag)
		{}
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
			Desc.Usage = EAccessHint::EA_DrawIndirect | EAccessHint::EA_GPUReadWrite | EAccessHint::EA_GPUStructured | EAccessHint::EA_GPUUnordered;
			Desc.BytesPerElement = BytesPerElement;
			Desc.NumElements = NumElements;
			return Desc;
		}

		static RGBufferDesc CreateByteAddressDesc(uint32 NumBytes)
		{
			wassume(NumBytes % 4 == 0);
			RGBufferDesc Desc;
			Desc.Usage = EAccessHint::EA_Raw | EAccessHint::EA_GPUReadWrite | EAccessHint::EA_GPUStructured | EAccessHint::EA_GPUUnordered;
			Desc.BytesPerElement = 4;
			Desc.NumElements = NumBytes / 4;
			return Desc;
		}

		static RGBufferDesc CreateStructuredDesc(uint32 BytesPerElement, uint32 NumElements)
		{
			RGBufferDesc Desc;
			Desc.Usage = EAccessHint::EA_GPUReadWrite | EAccessHint::EA_GPUStructured | EAccessHint::EA_GPUUnordered;
			Desc.BytesPerElement = BytesPerElement;
			Desc.NumElements = NumElements;
			return Desc;
		}

		uint32 GetSize() const
		{
			return NumElements * BytesPerElement;
		}

		auto operator<=>(const RGBufferDesc&) const = default;
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

	private:
		RGBufferHandle Handle;

		friend RGBuilder;
		friend RGBufferRegistry;
		friend RGAllocator;
	};

	using RGBufferRef = RGBuffer*;

	struct RGBufferUAVDesc
	{
		RGBufferRef Buffer = nullptr;
		EFormat Format;
	};

	class RGBufferUAV final :public RGUnorderedAccessView
	{
	public:
		static const ERGViewType StaticType = ERGViewType::BufferUAV;

		const RGBufferUAVDesc Desc;

	private:
		RGBufferUAV(const char* InName, const RGBufferUAVDesc& InDesc, ERGUnorderedAccessViewFlags InFlag)
			:RGUnorderedAccessView(InName, StaticType, InFlag),Desc(InDesc)
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
			:RGResource(InName), ParameterStruct(InParameters, RGParameterStruct::GetStructMetadata<TStruct>())
		{
		}
	private:
		RGConstBufferHandle Handle;
		const RGParameterStruct ParameterStruct;

		friend RGBuilder;
		friend RGConstBufferRegistry;
		friend RGAllocator;
	};

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

	private:
		explicit RGTConstBuffer(const TBufferStruct* InParameters, const char* InName)
			: RGConstBuffer(InParameters, InName)
			, Parameters(InParameters)
		{}
	
		TBufferStruct* Parameters;

		friend RGBuilder;
		friend RGConstBufferRegistry;
		friend RGAllocator;
	};
}