#include "UnifiedBuffer.h"
#include "RenderInterface/IContext.h"
#include "RenderInterface/BuiltinShader.h"

#include "Runtime/RenderCore/ShaderTextureTraits.hpp"
#include "Runtime/RenderCore/Dispatch.h"

using namespace platform::Render;

ConstantBuffer* platform::Render::CreateConstantBuffer(const void* Contents, Buffer::Usage Usage, const ShaderParametersMetadata& Layout)
{
	auto& Device = Context::Instance().GetDevice();

	Usage = static_cast<Buffer::Usage>(Usage | Buffer::Usage::Static);

	return Device.CreateConstantBuffer(Usage, Layout.GetSize(), Contents);
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

IMPLEMENT_BUILTIN_SHADER(MemcpyCS, "ByteBuffer.wsl", "MemcpyCS", platform::Render::ComputeShader);

template<>
void platform::Render::MemcpyResource<std::shared_ptr<GraphicsBuffer>>(CommandList& cmdList, const std::shared_ptr<GraphicsBuffer>& DstBuffer, const std::shared_ptr<GraphicsBuffer>& SrcBuffer, const MemcpyResourceParams& Params)
{
	const uint32 Divisor = white::has_anyflags(DstBuffer->GetAccess(), EAccessHint::EA_Raw) ? 4 : 1;

	uint32 NumElementsProcessed = 0;

	while (NumElementsProcessed < Params.Count)
	{
		ByteBufferResourceType ResourceTypeEnum;

		const uint32 NumWaves = std::max(std::min<uint32>(Caps.MaxDispatchThreadGroupsPerDimension.x, white::math::DivideAndRoundUp(Params.Count / Divisor, 64u)), 1u);
		const uint32 NumElementsPerDispatch = std::min(std::max(NumWaves, 1u) * Divisor * 64, Params.Count - NumElementsProcessed);

		MemcpyCS::Parameters Parameters;
		Parameters.Common.Size = NumElementsPerDispatch;
		Parameters.Common.SrcOffset = (Params.SrcOffset + NumElementsProcessed);
		Parameters.Common.DstOffset = (Params.DstOffset + NumElementsProcessed);

		if (white::has_anyflags(DstBuffer->GetAccess(), EAccessHint::EA_Raw))
		{
			ResourceTypeEnum = ByteBufferResourceType::Uint_Buffer;

			Parameters.SrcByteAddressBuffer = cmdList.CreateShaderResourceView(SrcBuffer.get()).get();
			Parameters.Common.DstByteAddressBuffer = cmdList.CreateUnorderedAccessView(DstBuffer.get()).get();;
		}

		MemcpyCS::PermutationDomain PermutationVector;
		PermutationVector.Set<MemcpyCS::ResourceTypeDim >(ResourceTypeEnum);
		PermutationVector.Set<MemcpyCS::StructuredElementSizeDim>(ByteBufferStructuredSize::Uint4);

		auto ComputeShader = Render::GetBuiltInShaderMap()->GetShader<MemcpyCS>(PermutationVector);

		ComputeShaderUtils::Dispatch(cmdList, ComputeShader, Parameters, white::math::int3(NumWaves, 1, 1));

		NumElementsProcessed += NumElementsPerDispatch;
	}
}

template void platform::Render::MemcpyResource<std::shared_ptr<GraphicsBuffer>>(CommandList& cmdList, const std::shared_ptr<GraphicsBuffer>& DstBuffer, const std::shared_ptr<GraphicsBuffer>& SrcBuffer, const MemcpyResourceParams& Params);