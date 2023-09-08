#include "HardwareShader.h"

using namespace platform_ex::Windows::D3D12;
using namespace platform::Render::Shader;

D3D12HardwareShader::D3D12HardwareShader(const white::span<const uint8>& Code)
	:bGlobalUniformBufferUsed(false)
{
	ShaderCodeReader ShaderCode(Code);

	int32 Offset = 0;

	auto CodePtr = Code.data();
	auto CodeSize = ShaderCode.GetActualShaderCodeSize() - Offset;

	ShaderByteCode.first = std::make_unique<byte[]>(CodeSize);
	ShaderByteCode.second = CodeSize;
	std::memcpy(ShaderByteCode.first.get(),CodePtr,CodeSize);

	auto PackedResourceCounts = ShaderCode.FindOptionalData<ShaderCodeResourceCounts>();
	wassume(PackedResourceCounts);
	ResourceCounts = *PackedResourceCounts;

	auto GlobalUniformBufferUsed = reinterpret_cast<const bool*>(ShaderCode.FindOptionalData('b'));
	wconstraint(GlobalUniformBufferUsed);

	bGlobalUniformBufferUsed = *GlobalUniformBufferUsed;

	HashShader();
}

D3D12HardwareShader::D3D12HardwareShader()
	:bGlobalUniformBufferUsed(false)
{}
