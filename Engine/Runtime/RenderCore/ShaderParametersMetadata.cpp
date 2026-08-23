#include "ShaderParametersMetadata.h"

using namespace platform::Render;

uint32 GetBufferCount(const std::vector<ShaderParametersMetadata::Member>& Members)
{
	uint32 Count = 0;
	for (auto& member : Members)
	{
		if (IsBufferType(member.GetShaderType()))
			++Count;
		if (auto* Struct = member.GetStructMetadata())
			Count += GetBufferCount(Struct->GetMembers());
	}

	return Count;
}

uint32 GetTextureCount(const std::vector<ShaderParametersMetadata::Member>& Members)
{
	uint32 Count = 0;
	for (auto& member : Members)
	{
		if (IsTextureType(member.GetShaderType()))
			++Count;
		if (auto* Struct = member.GetStructMetadata())
			Count += GetTextureCount(Struct->GetMembers());
	}

	return Count;
}

uint32 GetCBufferCount(const std::vector<ShaderParametersMetadata::Member>& Members)
{
	uint32 Count = 0;
	for (auto& member : Members)
	{
		if (member.GetShaderType() == SPT_ConstantBuffer)
			++Count;
		if (auto* Struct = member.GetStructMetadata())
			Count += GetCBufferCount(Struct->GetMembers());
	}

	return Count;
}

ShaderParametersMetadata::ShaderParametersMetadata(
	uint32 InSize, 
	std::vector<Member>&& InMembers)
	:Size(InSize)
	,Members(std::move(InMembers))
	, BufferCount(GetBufferCount(Members))
	, TextureCount(GetTextureCount(Members))
	, CBufferCount(GetCBufferCount(Members))
{
}
