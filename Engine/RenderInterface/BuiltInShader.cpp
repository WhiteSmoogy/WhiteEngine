#include "BuiltInShader.h"
#include "Runtime/Core/Container/map.hpp"
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
	auto section_itr = SectionMap.find(ShaderType->GetHashedShaderFilename());

	return section_itr != SectionMap.end() ? section_itr->second->GetShader(ShaderType, PermutationId):ShaderRef<RenderShader>();
}

void Shader::BuiltInShaderMap::AddSection(BuiltInShaderMapSection* InSection)
{
	wassume(InSection);
	const auto &Content = InSection->Content;
	const FHashedName& HashedFilename = Content.HashedSourceFilename;

	SectionMap.emplace(HashedFilename, InSection);
}

BuiltInShaderMapSection* Shader::BuiltInShaderMap::FindSection(const FHashedName& HashedShaderFilename)
{
	auto Section =white::find(SectionMap,HashedShaderFilename);
	return Section?*Section:nullptr;
}

BuiltInShaderMapSection* Shader::BuiltInShaderMap::FindOrAddSection(const ShaderMeta* ShaderType)
{
	const FHashedName HashedFilename(ShaderType->GetHashedShaderFilename());
	auto Section = FindSection(HashedFilename);
	if (!Section)
	{
		Section = new BuiltInShaderMapSection(HashedFilename);
		AddSection(Section);
	}
	return Section;
}

RenderShader* Shader::BuiltInShaderMap::FindOrAddShader(const ShaderMeta* ShaderType, int32 PermutationId, RenderShader* Shader)
{
	MapMutex.lock_shared();
	auto key = ShaderType->GetHashedShaderFilename();
	auto section_itr = SectionMap.find(key);
	MapMutex.unlock_shared();
	if (section_itr == SectionMap.end())
	{
		MapMutex.lock();
		section_itr = SectionMap.emplace(key, new BuiltInShaderMapSection(key)).first;
		MapMutex.unlock();
	}

	return section_itr->second->Content.FindOrAddShader(ShaderType->GetHash(),PermutationId,Shader);
}
