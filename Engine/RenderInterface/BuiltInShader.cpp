#include "BuiltInShader.h"
#include "Core/Container/map.hpp"
using namespace platform::Render;

ShaderRef<RenderShader> Shader::BuiltInShaderMapSection::GetShader(ShaderMeta* ShaderType, int32 PermutationId) const
{
	return { Content.GetShader(ShaderType, PermutationId),*this };
}

Shader::BuiltInShaderMap::~BuiltInShaderMap()
{
	for (auto& section : SectionMap)
	{
		delete section.second;
	}

	SectionMap.clear();
}

ShaderRef<RenderShader> Shader::BuiltInShaderMap::GetShader(ShaderMeta* ShaderType, int32 PermutationId) const
{
	std::shared_lock lock{ MapMutex };
	auto section_itr = SectionMap.find(ShaderType->GetHashedShaderFilename());

	return section_itr != SectionMap.end() ? section_itr->second->GetShader(ShaderType, PermutationId):ShaderRef<RenderShader>();
}

void Shader::BuiltInShaderMap::AddSection(BuiltInShaderMapSection* InSection)
{
	wassume(InSection);
	const auto &Content = InSection->Content;
	const FHashedName& HashedFilename = Content.HashedSourceFilename;

	std::unique_lock lock{ MapMutex };
	auto [SectionItr, bInserted] = SectionMap.emplace(HashedFilename, InSection);
	if (!bInserted && SectionItr->second != InSection)
	{
		delete InSection;
	}
}

BuiltInShaderMapSection* Shader::BuiltInShaderMap::FindSection(const FHashedName& HashedShaderFilename)
{
	std::shared_lock lock{ MapMutex };
	auto Section = SectionMap.find(HashedShaderFilename);
	return Section != SectionMap.end() ? Section->second : nullptr;
}

BuiltInShaderMapSection* Shader::BuiltInShaderMap::FindOrAddSection(const ShaderMeta* ShaderType)
{
	const FHashedName HashedFilename(ShaderType->GetHashedShaderFilename());
	std::unique_lock lock{ MapMutex };
	auto SectionItr = SectionMap.find(HashedFilename);
	if (SectionItr == SectionMap.end())
	{
		SectionItr = SectionMap.emplace(
			HashedFilename,
			new BuiltInShaderMapSection(HashedFilename)).first;
	}
	return SectionItr->second;
}

RenderShader* Shader::BuiltInShaderMap::FindOrAddShader(const ShaderMeta* ShaderType, int32 PermutationId, RenderShader* Shader)
{
	auto* Section = FindOrAddSection(ShaderType);
	return Section->Content.FindOrAddShader(ShaderType->GetHash(),PermutationId,Shader);
}
