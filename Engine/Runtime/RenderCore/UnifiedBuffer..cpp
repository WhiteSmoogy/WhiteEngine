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
	MAX
};

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
		END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_BUILTIN_SHADER(MemcpyCS, "ByteBuffer.hlsl", "MemcpyCS", platform::Render::ComputeShader);

void platform::Render::MemcpyResource(RGBuilder& Builder, RGBuffer* DstResource, RGBuffer* SrcResource, const MemcpyResourceParams& Params)
{
	MemcpyResource(Builder, Builder.CreateUAV({ .Buffer = DstResource }, ERGUnorderedAccessViewFlags::SkipBarrier), Builder.CreateSRV({ .Buffer = SrcResource }), Params);
}

void platform::Render::MemcpyResource(RGBuilder& Builder, RGBufferUAV* UAV, RGBufferSRV* SRV, const MemcpyResourceParams& Params)
{
	auto* DstResource = UAV->GetParent();
	auto* SrcResource = SRV->GetParent();

	const uint32 Divisor = white::has_anyflags(DstResource->GetAccess(), EAccessHint::Raw) ? 4 : 1;

	uint32 NumElementsProcessed = 0;

	while (NumElementsProcessed < Params.Count)
	{
		ByteBufferResourceType ResourceTypeEnum;

		const uint32 NumWaves = std::max(std::min<uint32>(Caps.MaxDispatchThreadGroupsPerDimension.x, white::math::DivideAndRoundUp(Params.Count / Divisor, 64u)), 1u);
		const uint32 NumElementsPerDispatch = std::min(std::max(NumWaves, 1u) * Divisor * 64, Params.Count - NumElementsProcessed);

		auto Parameters = Builder.AllocParameters<MemcpyCS::Parameters>();
		Parameters->Common.Size = NumElementsPerDispatch;
		Parameters->Common.SrcOffset = (Params.SrcOffset + NumElementsProcessed);
		Parameters->Common.DstOffset = (Params.DstOffset + NumElementsProcessed);

		if (white::has_anyflags(DstResource->GetAccess(), EAccessHint::Raw))
		{
			ResourceTypeEnum = ByteBufferResourceType::Uint_Buffer;

			Parameters->SrcByteAddressBuffer = SRV;
			Parameters->Common.DstByteAddressBuffer = UAV;
		}
		else
		{
			throw white::unimplemented();
		}

		MemcpyCS::PermutationDomain PermutationVector;
		PermutationVector.Set<MemcpyCS::ResourceTypeDim >(ResourceTypeEnum);
		PermutationVector.Set<MemcpyCS::StructuredElementSizeDim>(ByteBufferStructuredSize::Uint4);

		auto ComputeShader = platform::Render::GetBuiltInShaderMap()->GetShader<MemcpyCS>(PermutationVector);

		ComputeShaderUtils::AddPass(
			Builder, 
			RGEventName("Memcpy(Offset:{} Count:{})",NumElementsProcessed,NumElementsPerDispatch),
			ComputeShader, 
			Parameters, 
			white::math::int3(NumWaves, 1, 1));

		NumElementsProcessed += NumElementsPerDispatch;
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
	auto* DstResource = UAV->GetParent();

	const uint32 Divisor = white::has_anyflags(DstResource->GetAccess(), EAccessHint::Raw) ? 4 : 16;

	uint32 NumElementsProcessed = 0;

	while (NumElementsProcessed < Params.Count)
	{
		ByteBufferResourceType ResourceTypeEnum;

		const uint32 NumWaves = std::max(std::min<uint32>(Caps.MaxDispatchThreadGroupsPerDimension.x, white::math::DivideAndRoundUp(Params.Count / Divisor, 64u)), 1u);
		const uint32 NumElementsPerDispatch = std::min(std::max(NumWaves, 1u) * Divisor * 64, Params.Count - NumElementsProcessed);

		auto Parameters = Builder.AllocParameters<MemsetCS::Parameters>();
		Parameters->Common.Size = NumElementsPerDispatch / Divisor;
		Parameters->Common.DstOffset = (Params.DstOffset + NumElementsProcessed) / Divisor;
		Parameters->Common.Value = Params.Value;

		if (white::has_anyflags(DstResource->GetAccess(), EAccessHint::Raw))
		{
			ResourceTypeEnum = ByteBufferResourceType::Uint_Buffer;

			Parameters->Common.DstByteAddressBuffer = UAV;
		}
		else if (white::has_anyflags(DstResource->GetAccess(), EAccessHint::Structured))
		{
			ResourceTypeEnum = ByteBufferResourceType::StructuredBuffer;
			Parameters->Common.DstStructuredBuffer4x = UAV;
		}
		else
		{
			ResourceTypeEnum = ByteBufferResourceType::Float4_Buffer;
			Parameters->Common.DstBuffer = UAV;
		}

		MemsetCS::PermutationDomain PermutationVector;
		PermutationVector.Set<MemcpyCS::ResourceTypeDim >(ResourceTypeEnum);
		PermutationVector.Set<MemcpyCS::StructuredElementSizeDim>(ByteBufferStructuredSize::Uint4);

		auto ComputeShader = Render::GetBuiltInShaderMap()->GetShader<MemsetCS>(PermutationVector);

		ComputeShaderUtils::AddPass<MemsetCS>(
			Builder,
			RenderGraph::RGEventName{ "Memset(Offset:{} Count:{})",NumElementsProcessed,NumElementsPerDispatch },
			ComputeShader, Parameters, white::math::int3(NumWaves, 1, 1));

		NumElementsProcessed += NumElementsPerDispatch;
	}
}

