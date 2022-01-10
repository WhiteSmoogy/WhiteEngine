#include "Shader.h"
#include "Runtime/Core/Compression.h"
#include "IContext.h"
#include "IDevice.h"
#include "IRayContext.h"
#include "Runtime/Core/Container/vector.hpp"
using namespace platform::Render;
using namespace WhiteEngine;

const uint8* Shader::TryUncompressCode(const std::vector<uint8>& Code, int32 UnCompressSize, std::vector<uint8>& UnCompressCode)
{
	const uint8* ShaderCode = Code.data();

	if (static_cast<int32>(Code.size()) != UnCompressSize)
	{
		UnCompressCode.resize(white::Align(UnCompressSize, 16));
		Compression::UnCompressMemory(GetShaderCompressionFormat(), UnCompressCode.data(), UnCompressSize, ShaderCode, Code.size());

		ShaderCode = UnCompressCode.data();
	}

	return ShaderCode;
}

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
	std::unique_lock lock{ ShaderCriticalSection };
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


	HardwareShader* Shader = nullptr;
	std::vector<uint8> UnCompressCode;
	auto code = white::make_const_span(TryUncompressCode(ShaderEntry.Code,ShaderEntry.UnCompressSize,UnCompressCode), ShaderEntry.UnCompressSize);

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