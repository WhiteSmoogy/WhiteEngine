#include "Shader.h"
#include "Core/Compression.h"
#include "IContext.h"
#include "IDevice.h"
using namespace platform::Render;
using namespace WhiteEngine;

const std::string& GetShaderCompressionFormat()
{
	return NAME_LZ4;
}

HardwareShader* Shader::ShaderMapResource::CreateShader(int32 ShaderIndex)
{
	wassume(!HWShaders[ShaderIndex].load(std::memory_order_acquire));

	auto HWShader = CreateHWShader(ShaderIndex);

#if D3D_RAYTRACING
	//AddToRayTracingLibrary
#endif

	return HWShader;
}

HardwareShader* ShaderMapResource_InlineCode::CreateHWShader(int32 ShaderIndex)
{
	auto& ShaderEntry = Code->ShaderEntries[ShaderIndex];
	const uint8* ShaderCode = ShaderEntry.Code.data();

	std::vector<uint8> UnCompressCode;
	if (static_cast<int32>(ShaderEntry.Code.size()) != ShaderEntry.UnCompressSize)
	{
		UnCompressCode.resize(white::Align(ShaderEntry.UnCompressSize,16));
		Compression::UnCompressMemory(GetShaderCompressionFormat(), UnCompressCode.data(), ShaderEntry.UnCompressSize, ShaderCode, ShaderEntry.Code.size());

		ShaderCode = UnCompressCode.data();
	}

	HardwareShader* Shader = nullptr;

#if D3D_RAYTRACING
	if (ShaderEntry.Type >= RayGen)
	{
	}
	else
#endif
	{
		Shader = Context::Instance().GetDevice().CreateShader(white::make_const_span(ShaderCode, ShaderEntry.UnCompressSize));
	}

	if (Shader)
		Shader->SetHash(Code->ShaderHashes[ShaderIndex]);
	

	return Shader;
}