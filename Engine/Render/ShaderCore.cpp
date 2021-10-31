#include "ShaderCore.h"
#include "Core/Serialization/AsyncArchive.h"
#include "Core/Compression.h"

using namespace platform::Render;
using namespace platform::Render::Shader;

using namespace WhiteEngine;

WhiteEngine::Task<void> platform::Render::ShaderCodeResourceCounts::Serialize(WhiteEngine::AsyncArchive& v)
{
	co_await(v >> NumSamplers);
	co_await(v >> NumSRVs);
	co_await(v >> NumUAVs);
	co_await(v >> NumCBs);
}

WhiteEngine::Task<void> platform::Render::Shader::ShaderInfo::ConstantBufferInfo::VariableInfo::Serialize(WhiteEngine::AsyncArchive& v)
{
	co_await(v >> name);
	co_await(v >> start_offset);
	co_await(v >> type);
	co_await(v >> rows);
	co_await(v >> columns);
	co_await(v >> elements);
	co_await(v >> size);
}

WhiteEngine::Task<void> platform::Render::Shader::ShaderInfo::ConstantBufferInfo::Serialize(WhiteEngine::AsyncArchive& v)
{
	co_await(v >> var_desc);
	co_await(v >> name);
	co_await(v >> name_hash);
	co_await(v >> size);
	co_await(v >> bind_point);
}

WhiteEngine::Task<void> platform::Render::Shader::RayTracingShaderInfo::Serialize(WhiteEngine::AsyncArchive& v)
{
	co_await(v >> EntryPoint);
	co_await(v >> AnyHitEntryPoint);
	co_await(v >> IntersectionEntryPoint);
}

WhiteEngine::Task<void> platform::Render::Shader::ShaderInfo::BoundResourceInfo::Serialize(WhiteEngine::AsyncArchive& v)
{
	co_await(v >> name);
	co_await(v >> type);
	co_await(v >> dimension);
	co_await(v >> bind_point);
}

WhiteEngine::Task<void> platform::Render::Shader::ShaderInfo::Serialize(WhiteEngine::AsyncArchive& v)
{
	co_await(v >> Type);
	co_await(v >> ConstantBufferInfos);
	co_await(v >> BoundResourceInfos);
	co_await(v >> ResourceCounts);
	co_await(v >> InputSignature);
	co_await(v >> CSBlockSize);
	co_await(v >> RayTracingInfos);
}

void ShaderParameterMap::AddParameterAllocation(const std::string& ParameterName, uint16 BufferIndex, uint16 BaseIndex, uint16 Size, ShaderParamClass ParameterType)
{
	ParameterAllocation Allocation;
	Allocation.BufferIndex = BufferIndex;
	Allocation.BaseIndex = BaseIndex;
	Allocation.Size = Size;
	Allocation.Class = ParameterType;
	ParameterMap.emplace(ParameterName, Allocation);
}

bool ShaderParameterMap::FindParameterAllocation(const std::string& ParameterName, uint16& OutBufferIndex, uint16& OutBaseIndex, uint16& OutSize) const
{
	auto itr = ParameterMap.find(ParameterName);
	if (itr != ParameterMap.end())
	{
		OutBufferIndex = itr->second.BufferIndex;
		OutBaseIndex = itr->second.BaseIndex;
		OutSize = itr->second.Size;

		return true;
	}

	return false;
}

void ShaderParameterMap::UpdateHash(Digest::SHA1& HashState) const
{
	for (auto& ParameterIt :ParameterMap)
	{
		const auto& ParamValue = ParameterIt.second;
		HashState.Update((const uint8*)*ParameterIt.first.data(), ParameterIt.first.length() * sizeof(char));
		HashState.Update((const uint8*)&ParamValue.BufferIndex, sizeof(ParamValue.BufferIndex));
		HashState.Update((const uint8*)&ParamValue.BaseIndex, sizeof(ParamValue.BaseIndex));
		HashState.Update((const uint8*)&ParamValue.Size, sizeof(ParamValue.Size));
	}
}

void ShaderCode::Compress(const std::string& ShaderCompressionFormat)
{
	WAssert(OptionalDataSize == -1, "ShaderCode::Compress() was called before calling ShaderCode::FinalizeShaderCode()");

	std::vector<uint8> Compressed;
	int32 CompressedSize =static_cast<int32>(ShaderCodeWithOptionalData.size());
	Compressed.resize(CompressedSize);

	// there is code that assumes that if CompressedSize == CodeSize, the shader isn't compressed. Because of that, do not accept equal compressed size (very unlikely anyway)
	if (Compression::CompressMemory(ShaderCompressionFormat, Compressed.data(), CompressedSize, ShaderCodeWithOptionalData.data(), ShaderCodeWithOptionalData.size()) && CompressedSize < ShaderCodeWithOptionalData.size())
	{
		// cache the ShaderCodeSize since it will no longer possible to get it as the reader will fail to parse the compressed data
		ShaderCodeReader Wrapper(ShaderCodeWithOptionalData);
		ShaderCodeSize = Wrapper.GetShaderCodeSize();

		// finalize the compression
		CompressionFormat = ShaderCompressionFormat;
		UncompressedSize = ShaderCodeWithOptionalData.size();

		Compressed.resize(CompressedSize);
		ShaderCodeWithOptionalData = Compressed;
	}
}

void ShaderCompilerOutput::GenerateOutputHash()
{
	Digest::SHA1 HashState;

	auto& Code = ShaderCode.GetReadAccess();

	// we don't hash the optional attachments as they would prevent sharing (e.g. many materials share the same VS)
	uint32 ShaderCodeSize = ShaderCode.GetShaderCodeSize();

	// make sure we are not generating the hash on compressed data
	WAssert(!ShaderCode.IsCompressed(), "Attempting to generate the output hash of a compressed code");

	HashState.Update(Code.data(), ShaderCodeSize * sizeof(uint8));
	ParameterMap.UpdateHash(HashState);
	HashState.Final();
	HashState.GetHash(&OutputHash.Hash[0]);
}

void ShaderCompilerOutput::CompressOutput(const std::string& ShaderCompressionFormat)
{
	// make sure the hash has been generated
	WAssert(OutputHash !=Digest::SHAHash(), "Output hash must be generated before compressing the shader code.");
	WAssert(!ShaderCompressionFormat.empty(), "Compression format should be valid");
	ShaderCode.Compress(ShaderCompressionFormat);
}
