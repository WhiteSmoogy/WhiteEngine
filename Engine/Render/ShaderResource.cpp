#include "Shader.h"
#include "Core/Compression.h"
#include "IContext.h"
#include "IDevice.h"
#include "IRayContext.h"
#include "Core/Container/vector.hpp"
using namespace platform::Render;
using namespace WhiteEngine;

const std::string& Shader::GetShaderCompressionFormat()
{
	return NAME_LZ4;
}

Shader::ShaderMapResource::ShaderMapResource(std::size_t Num)
	:NumHWShaders(Num)
{
	HWShaders = std::make_unique<std::atomic<HardwareShader*>[]>(NumHWShaders);
#if D3D_RAYTRACING
#endif
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

void Shader::ShaderMapResourceCode::AddShaderCompilerOutput(const ShaderCompilerOutput& Output)
{
	AddShaderCode(Output.Type, Output.OutputHash, Output.ShaderCode);
}

int32 Shader::ShaderMapResourceCode::FindShaderIndex(const Digest::SHAHash& InHash) const
{
	auto index = std::distance(ShaderHashes.begin(),std::lower_bound(ShaderHashes.begin(), ShaderHashes.end(), InHash));
	if (index != ShaderHashes.size() && ShaderHashes[index] == InHash)
		return static_cast<int32>(index);
	return white::INDEX_NONE;
}

void Shader::ShaderMapResourceCode::AddShaderCode(ShaderType InType, const Digest::SHAHash& InHash, const ShaderCode& InCode)
{
	auto index = std::distance(ShaderHashes.begin(), std::lower_bound(ShaderHashes.begin(), ShaderHashes.end(), InHash));
	if (index >= ShaderHashes.size() || ShaderHashes[index] != InHash)
	{
		ShaderHashes.insert(ShaderHashes.begin() + index, InHash);

		auto& Entry = *ShaderEntries.emplace(ShaderEntries.begin() + index);

		Entry.Type = InType;

		auto ShaderCompressionFormat = GetShaderCompressionFormat();
		if (!ShaderCompressionFormat.empty())
		{
			Entry.UnCompressSize = InCode.GetUncompressedSize();
			WAssert(ShaderCompressionFormat == InCode.GetCompressionFormat(), "shader compression format mismatch");
		}
		else
		{
			Entry.UnCompressSize =static_cast<int32>(InCode.GetReadAccess().size());
		}
		Entry.Code = InCode.GetReadAccess();
	}
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
	auto code = white::make_const_span(ShaderCode, ShaderEntry.UnCompressSize);

#if D3D_RAYTRACING
	if (ShaderEntry.Type >= RayGen)
	{
		Shader = Context::Instance().GetRayContext().GetDevice().CreateRayTracingSahder(code);
	}
	else
#endif
	{
		auto& Device = Context::Instance().GetDevice();
		
		switch (ShaderEntry.Type)
		{
		case VertexShader:
			Shader = Device.CreateVertexShader(code);
			break;
		case PixelShader:
			Shader = Device.CreatePixelShader(code);
			break;
		case GeometryShader:
			Shader = Device.CreateGeometryShader(code);
			break;
		case ComputeShader:
			Shader = Device.CreateComputeShader(code);
			break;
		}
	}

	if (Shader)
		Shader->SetHash(Code->ShaderHashes[ShaderIndex]);
	

	return Shader;
}