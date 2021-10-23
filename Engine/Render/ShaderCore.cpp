#include "ShaderCore.h"
#include "Core/Serialization/AsyncArchive.h"

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
