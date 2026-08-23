#include "UnifiedBuffer.h"
#include "RenderInterface/IContext.h"
#include "RenderInterface/BuiltinShader.h"

#include "Runtime/RenderCore/ShaderTextureTraits.hpp"
#include "Runtime/RenderCore/Dispatch.h"

using namespace platform::Render;
using namespace RenderGraph;

ConstantBuffer* platform::Render::CreateConstantBuffer(const void* Contents, Buffer::Usage Usage, const ShaderParametersMetadata& Layout)
{
	auto& Device = Context::Instance().GetDevice();

	Usage = static_cast<Buffer::Usage>(Usage | Buffer::Usage::Static);

	return Device.CreateConstantBuffer(Layout.GetSize(), Contents, __func__, Usage);
}

enum class ByteBufferResourceType
{
	Float4_Buffer,
	StructuredBuffer,
	Uint_Buffer,

	MAX
};

enum class ByteBufferStructuredSize
{
	Uint1,
	Uint2,
	Uint4,
	Uint5,
	MAX
};

struct ByteBufferUint5
{
	uint32 Values[5];
};

static_assert(sizeof(ByteBufferUint5) == 20);

class ByteBufferShader :public BuiltInShader
{
public:
	using DerivedType = ByteBufferShader;

	ByteBufferShader(const ShaderMetaType::CompiledShaderInitializer& Initializer)
		: BuiltInShader(Initializer)
	{
	}

	ByteBufferShader()
	{}

	class ResourceTypeDim : SHADER_PERMUTATION_ENUM_CLASS("RESOURCE_TYPE", ByteBufferResourceType);
	class StructuredElementSizeDim : SHADER_PERMUTATION_ENUM_CLASS("STRUCTURED_ELEMENT_SIZE", ByteBufferStructuredSize);

	using PermutationDomain = TShaderPermutationDomain<ResourceTypeDim, StructuredElementSizeDim>;

	static bool ShouldCompilePermutation(const FBuiltInShaderPermutationParameters& Parameters)
	{
		PermutationDomain PermutationVector(Parameters.PermutationId);

		ByteBufferResourceType ResourceType = PermutationVector.Get<ResourceTypeDim>();

		// Don't compile structured buffer size variations unless we need them
		if (ResourceType != ByteBufferResourceType::StructuredBuffer && static_cast<ByteBufferStructuredSize>(PermutationVector.Get<StructuredElementSizeDim>()) != ByteBufferStructuredSize::Uint4)
		{
			return false;
		}

		return true;
	}

	BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
		SHADER_PARAMETER(uint32, Value)
		SHADER_PARAMETER(uint32, Size)
		SHADER_PARAMETER(uint32, SrcOffset)
		SHADER_PARAMETER(uint32, DstOffset)
		SHADER_PARAMETER_UAV(RWByteAddressBuffer, DstByteAddressBuffer)
		SHADER_PARAMETER_UAV(RWStructuredBuffer<wm::uint4>, DstStructuredBuffer4x)
		SHADER_PARAMETER_UAV(RWStructuredBuffer<wm::uint2>, DstStructuredBuffer2x)
		SHADER_PARAMETER_UAV(RWStructuredBuffer<uint32>, DstStructuredBuffer1x)
		SHADER_PARAMETER_UAV(RWStructuredBuffer<ByteBufferUint5>, DstStructuredBuffer5x)
		SHADER_PARAMETER_UAV(RWBuffer<wm::float4>, DstBuffer)
		END_SHADER_PARAMETER_STRUCT()
};

class MemcpyCS :public ByteBufferShader
{
public:
	EXPORTED_BUILTIN_SHADER(MemcpyCS);

	BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
		SHADER_PARAMETER_STRUCT_INCLUDE(ByteBufferShader::Parameters, Common)
		SHADER_PARAMETER_SRV(ByteAddressBuffer, SrcByteAddressBuffer)
		SHADER_PARAMETER_SRV(StructuredBuffer<wm::uint4>, SrcStructuredBuffer4x)
		SHADER_PARAMETER_SRV(StructuredBuffer<wm::uint2>, SrcStructuredBuffer2x)
		SHADER_PARAMETER_SRV(StructuredBuffer<uint32>, SrcStructuredBuffer1x)
		SHADER_PARAMETER_SRV(StructuredBuffer<ByteBufferUint5>, SrcStructuredBuffer5x)
		SHADER_PARAMETER_SRV(Buffer<wm::float4>, SrcBuffer)
		END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_BUILTIN_SHADER(MemcpyCS, "ByteBuffer.hlsl", "MemcpyCS", platform::Render::ComputeShader);

void platform::Render::MemcpyResource(RGBuilder& Builder, RGBuffer* DstResource, RGBuffer* SrcResource, const MemcpyResourceParams& Params)
{
	MemcpyResource(Builder, Builder.CreateUAV({ .Buffer = DstResource }, ERGUnorderedAccessViewFlags::SkipBarrier), Builder.CreateSRV({ .Buffer = SrcResource }), Params);
}

void platform::Render::MemcpyResource(RGBuilder& Builder, RGBufferUAV* UAV, RGBufferSRV* SRV, const MemcpyResourceParams& Params)
{
	wassume(UAV != nullptr && SRV != nullptr);
	auto* DstResource = UAV->GetParent();
	auto* SrcResource = SRV->GetParent();
	wassume(DstResource != nullptr && SrcResource != nullptr);
	wassume(uint64(Params.DstOffset) + Params.Count <= DstResource->Desc.GetSize64());
	wassume(uint64(Params.SrcOffset) + Params.Count <= SrcResource->Desc.GetSize64());

	if (Params.Count == 0)
	{
		return;
	}

	ByteBufferResourceType ResourceTypeEnum;
	ByteBufferStructuredSize StructuredSize = ByteBufferStructuredSize::Uint4;
	uint32 BytesPerElement;
	uint32 BytesPerThread;

	if (white::has_anyflags(DstResource->GetAccess(), EAccessHint::Raw))
	{
		wassume(white::has_anyflags(SrcResource->GetAccess(), EAccessHint::Raw));
		ResourceTypeEnum = ByteBufferResourceType::Uint_Buffer;
		BytesPerElement = 4;
		BytesPerThread = 16;
	}
	else if (white::has_anyflags(DstResource->GetAccess(), EAccessHint::Structured))
	{
		wassume(white::has_anyflags(SrcResource->GetAccess(), EAccessHint::Structured));
		wassume(DstResource->Desc.BytesPerElement == SrcResource->Desc.BytesPerElement);
		ResourceTypeEnum = ByteBufferResourceType::StructuredBuffer;
		BytesPerElement = DstResource->Desc.BytesPerElement;
		BytesPerThread = BytesPerElement;

		switch (BytesPerElement)
		{
		case 4: StructuredSize = ByteBufferStructuredSize::Uint1; break;
		case 8: StructuredSize = ByteBufferStructuredSize::Uint2; break;
		case 16: StructuredSize = ByteBufferStructuredSize::Uint4; break;
		case 20: StructuredSize = ByteBufferStructuredSize::Uint5; break;
		default: wassume(false); return;
		}
	}
	else
	{
		ResourceTypeEnum = ByteBufferResourceType::Float4_Buffer;
		BytesPerElement = 16;
		BytesPerThread = 16;
	}

	wassume(Params.Count % BytesPerElement == 0);
	wassume(Params.SrcOffset % BytesPerElement == 0);
	wassume(Params.DstOffset % BytesPerElement == 0);

	uint32 NumElementsProcessed = 0;

	while (NumElementsProcessed < Params.Count)
	{
		const uint32 RemainingBytes = Params.Count - NumElementsProcessed;
		const uint32 ThreadCount = white::math::DivideAndRoundUp(RemainingBytes, BytesPerThread);
		const uint32 NumWaves = std::max(std::min<uint32>(
			Caps.MaxDispatchThreadGroupsPerDimension.x,
			white::math::DivideAndRoundUp(ThreadCount, 64u)), 1u);
		const uint64 DispatchCapacity = uint64(NumWaves) * 64 * BytesPerThread;
		const uint32 NumBytesPerDispatch = static_cast<uint32>(std::min<uint64>(DispatchCapacity, RemainingBytes));

		auto Parameters = Builder.AllocParameters<MemcpyCS::Parameters>();
		Parameters->Common.Size = NumBytesPerDispatch / BytesPerElement;
		Parameters->Common.SrcOffset = (Params.SrcOffset + NumElementsProcessed) / BytesPerElement;
		Parameters->Common.DstOffset = (Params.DstOffset + NumElementsProcessed) / BytesPerElement;

		switch (ResourceTypeEnum)
		{
		case ByteBufferResourceType::Uint_Buffer:
			Parameters->SrcByteAddressBuffer = SRV;
			Parameters->Common.DstByteAddressBuffer = UAV;
			break;
		case ByteBufferResourceType::StructuredBuffer:
			switch (StructuredSize)
			{
			case ByteBufferStructuredSize::Uint1:
				Parameters->SrcStructuredBuffer1x = SRV;
				Parameters->Common.DstStructuredBuffer1x = UAV;
				break;
			case ByteBufferStructuredSize::Uint2:
				Parameters->SrcStructuredBuffer2x = SRV;
				Parameters->Common.DstStructuredBuffer2x = UAV;
				break;
			case ByteBufferStructuredSize::Uint4:
				Parameters->SrcStructuredBuffer4x = SRV;
				Parameters->Common.DstStructuredBuffer4x = UAV;
				break;
			case ByteBufferStructuredSize::Uint5:
				Parameters->SrcStructuredBuffer5x = SRV;
				Parameters->Common.DstStructuredBuffer5x = UAV;
				break;
			default: wassume(false); return;
			}
			break;
		case ByteBufferResourceType::Float4_Buffer:
			Parameters->SrcBuffer = SRV;
			Parameters->Common.DstBuffer = UAV;
			break;
		default: wassume(false); return;
		}

		MemcpyCS::PermutationDomain PermutationVector;
		PermutationVector.Set<MemcpyCS::ResourceTypeDim >(ResourceTypeEnum);
		PermutationVector.Set<MemcpyCS::StructuredElementSizeDim>(StructuredSize);

		auto ComputeShader = platform::Render::GetBuiltInShaderMap()->GetShader<MemcpyCS>(PermutationVector);

		ComputeShaderUtils::AddPass(
			Builder, 
			RGEventName("Memcpy(Offset:{} Count:{})",NumElementsProcessed,NumBytesPerDispatch),
			ComputeShader, 
			Parameters, 
			white::math::int3(NumWaves, 1, 1));

		NumElementsProcessed += NumBytesPerDispatch;
	}
}


class MemsetCS :public ByteBufferShader
{
public:
	EXPORTED_BUILTIN_SHADER(MemsetCS);

	BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
		SHADER_PARAMETER_STRUCT_INCLUDE(ByteBufferShader::Parameters, Common)
		END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_BUILTIN_SHADER(MemsetCS, "ByteBuffer.hlsl", "MemsetCS", platform::Render::ComputeShader);

void  platform::Render::MemsetResource(RenderGraph::RGBuilder& Builder, RenderGraph::RGBufferUAV* UAV, const MemsetResourceParams& Params)
{
	wassume(UAV != nullptr);
	auto* DstResource = UAV->GetParent();
	wassume(DstResource != nullptr);
	wassume(uint64(Params.DstOffset) + Params.Count <= DstResource->Desc.GetSize64());

	if (Params.Count == 0)
	{
		return;
	}

	ByteBufferResourceType ResourceTypeEnum;
	ByteBufferStructuredSize StructuredSize = ByteBufferStructuredSize::Uint4;
	uint32 BytesPerElement;
	uint32 BytesPerThread;

	if (white::has_anyflags(DstResource->GetAccess(), EAccessHint::Raw))
	{
		ResourceTypeEnum = ByteBufferResourceType::Uint_Buffer;
		BytesPerElement = 4;
		BytesPerThread = 16;
	}
	else if (white::has_anyflags(DstResource->GetAccess(), EAccessHint::Structured))
	{
		ResourceTypeEnum = ByteBufferResourceType::StructuredBuffer;
		BytesPerElement = DstResource->Desc.BytesPerElement;
		BytesPerThread = BytesPerElement;
		switch (BytesPerElement)
		{
		case 4: StructuredSize = ByteBufferStructuredSize::Uint1; break;
		case 8: StructuredSize = ByteBufferStructuredSize::Uint2; break;
		case 16: StructuredSize = ByteBufferStructuredSize::Uint4; break;
		case 20: StructuredSize = ByteBufferStructuredSize::Uint5; break;
		default: wassume(false); return;
		}
	}
	else
	{
		ResourceTypeEnum = ByteBufferResourceType::Float4_Buffer;
		BytesPerElement = 16;
		BytesPerThread = 16;
	}

	wassume(Params.Count % BytesPerElement == 0);
	wassume(Params.DstOffset % BytesPerElement == 0);

	uint32 NumElementsProcessed = 0;

	while (NumElementsProcessed < Params.Count)
	{
		const uint32 RemainingBytes = Params.Count - NumElementsProcessed;
		const uint32 ThreadCount = white::math::DivideAndRoundUp(RemainingBytes, BytesPerThread);
		const uint32 NumWaves = std::max(std::min<uint32>(
			Caps.MaxDispatchThreadGroupsPerDimension.x,
			white::math::DivideAndRoundUp(ThreadCount, 64u)), 1u);
		const uint64 DispatchCapacity = uint64(NumWaves) * 64 * BytesPerThread;
		const uint32 NumBytesPerDispatch = static_cast<uint32>(std::min<uint64>(DispatchCapacity, RemainingBytes));

		auto Parameters = Builder.AllocParameters<MemsetCS::Parameters>();
		Parameters->Common.Size = NumBytesPerDispatch / BytesPerElement;
		Parameters->Common.DstOffset = (Params.DstOffset + NumElementsProcessed) / BytesPerElement;
		Parameters->Common.Value = Params.Value;

		switch (ResourceTypeEnum)
		{
		case ByteBufferResourceType::Uint_Buffer:
			Parameters->Common.DstByteAddressBuffer = UAV;
			break;
		case ByteBufferResourceType::StructuredBuffer:
			switch (StructuredSize)
			{
			case ByteBufferStructuredSize::Uint1: Parameters->Common.DstStructuredBuffer1x = UAV; break;
			case ByteBufferStructuredSize::Uint2: Parameters->Common.DstStructuredBuffer2x = UAV; break;
			case ByteBufferStructuredSize::Uint4: Parameters->Common.DstStructuredBuffer4x = UAV; break;
			case ByteBufferStructuredSize::Uint5: Parameters->Common.DstStructuredBuffer5x = UAV; break;
			default: wassume(false); return;
			}
			break;
		case ByteBufferResourceType::Float4_Buffer:
			Parameters->Common.DstBuffer = UAV;
			break;
		default: wassume(false); return;
		}

		MemsetCS::PermutationDomain PermutationVector;
		PermutationVector.Set<MemcpyCS::ResourceTypeDim >(ResourceTypeEnum);
		PermutationVector.Set<MemcpyCS::StructuredElementSizeDim>(StructuredSize);

		auto ComputeShader = Render::GetBuiltInShaderMap()->GetShader<MemsetCS>(PermutationVector);

		ComputeShaderUtils::AddPass<MemsetCS>(
			Builder,
			RenderGraph::RGEventName{ "Memset(Offset:{} Count:{})",NumElementsProcessed,NumBytesPerDispatch },
			ComputeShader, Parameters, white::math::int3(NumWaves, 1, 1));

		NumElementsProcessed += NumBytesPerDispatch;
	}
}

