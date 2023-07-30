#include "UnifiedBuffer.h"
#include "RenderInterface/IContext.h"
#include "RenderInterface/BuiltinShader.h"

#include "Runtime/RenderCore/ShaderTextureTraits.hpp"

using namespace platform::Render;

ConstantBuffer* platform::Render::CreateConstantBuffer(const void* Contents, Buffer::Usage Usage, const ShaderParametersMetadata& Layout)
{
	auto& Device = Context::Instance().GetDevice();

	Usage = static_cast<Buffer::Usage>(Usage | Buffer::Usage::Static);

	return Device.CreateConstantBuffer(Usage, Layout.GetSize(), Contents);
}

enum class ByteBufferResouceType
{
	Uint_Buffer,
	Float4_Buffer,
	StructuredBuffer,

	MAX
};

enum class ByteBufferStructuredSize
{
	Uint1,
	Uint2,
	Uint4,
	Uint8,
	MAX
};

class ByteBufferShader :public BuiltInShader
{
public:
	EXPORTED_BUILTIN_SHADER(ByteBufferShader);

	class ResourceTypeDim : SHADER_PERMUTATION_ENUM_CLASS("RESOURCE_TYPE", ByteBufferResouceType);
	class StructuredElementSizeDim : SHADER_PERMUTATION_ENUM_CLASS("STRUCTURED_ELEMENT_SIZE", ByteBufferStructuredSize);

	using FPermutationDomain = TShaderPermutationDomain<ResourceTypeDim, StructuredElementSizeDim>;

	static bool ShouldCompilePermutation(const FBuiltInShaderPermutationParameters& Parameters)
	{
		FPermutationDomain PermutationVector(Parameters.PermutationId);

		ByteBufferResouceType ResourceType = PermutationVector.Get<ResourceTypeDim>();

		// Don't compile structured buffer size variations unless we need them
		if (ResourceType != ByteBufferResouceType::StructuredBuffer && static_cast<ByteBufferStructuredSize>(PermutationVector.Get<StructuredElementSizeDim>()) != ByteBufferStructuredSize::Uint4)
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
		SHADER_PARAMETER(uint32, Float4sPerLine)
		SHADER_PARAMETER_UAV(RWByteAddressBuffer, DstByteAddressBuffer)
		END_SHADER_PARAMETER_STRUCT()
};


template<>
void platform::Render::MemcpyResource<GraphicsBuffer>(CommandList& cmdList, const GraphicsBuffer& DstBuffer, const GraphicsBuffer& SrcBuffer, const MemcpyResourceParams& Params)
{
}
