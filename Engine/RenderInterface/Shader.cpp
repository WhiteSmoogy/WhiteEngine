#include "Shader.h"
#include "BuiltInRayTracingShader.h"
#include "IContext.h"
#include "IRayContext.h"
#include "../Asset/D3DShaderCompiler.h"
#include "WFramework/Helper/ShellHelper.h"
#include "Runtime/Core/Coroutine/Task.h"
#include "Runtime/Core/Coroutine/ThreadScheduler.h"
#include "Runtime/Core/Coroutine/SyncWait.h"
#include "Runtime/Core/Coroutine/WhenAllReady.h"
#include "../System/SystemEnvironment.h"
#include "../Asset/ShaderAsset.h"
#include "Runtime/Core/Hash/CityHash.h"
#include "Runtime/Core/Serialization/MemoryWriter.h"
#include "BuiltInShader.h"
#include "ShaderParametersMetadata.h"
#include "ShaderDB.h"
#include "spdlog/spdlog.h"
#include "Runtime/Core/Path.h"
#include "Runtime/Core/Serialization/AsyncArchive.h"
#include "Runtime/Core/Container/vector.hpp"
#include "spdlog/stopwatch.h"
#include <format>

using namespace platform::Render;
using namespace WhiteEngine;

namespace platform::Render::Shader
{
	std::list<ShaderMeta*>* GShaderTypeList;

	bool IsRayTracingShader(ShaderType type)
	{
		switch (type)
		{
		case ShaderType::RayGen:
		case ShaderType::RayMiss:
		case ShaderType::RayHitGroup:
		case ShaderType::RayCallable:
			return true;
		default:
			return false;
		}
	}

	std::list<ShaderMeta*>& ShaderMeta::GetTypeList()
	{
		if (GShaderTypeList == nullptr)
			GShaderTypeList = new std::list<ShaderMeta*>();
		return *GShaderTypeList;
	}

	RenderShader* ShaderMeta::Construct() const
	{
		return (*ConstructRef)();
	}

	bool Shader::ShaderMeta::ShouldCompilePermutation(const FShaderPermutationParameters& Parameters) const
	{
		return (*ShouldCompilePermutationRef)(Parameters);
	}

	void ShaderMeta::ModifyCompilationEnvironment(const FShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment) const
	{
		(*ModifyCompilationEnvironmentRef)(Parameters, OutEnvironment);
	}

	ShaderMeta::ShaderMeta(EShaderMetaForDownCast InShaderMetaForDownCast, const char* InName, const char* InSourceFileName, const char* InEntryPoint, platform::Render::ShaderType InFrequency, int32 InTotalPermutationCount,
		ConstructType InConstructRef, 
		ModifyCompilationEnvironmentType InModifyCompilationEnvironmentRef,
		ShouldCompilePermutationType InShouldCompilePermutationRef,
		uint32 InTypeSize,
		const ShaderParametersMetadata* InRootParametersMetadata
	)
		:
		ShaderMetaForDownCast(InShaderMetaForDownCast),
		TypeName(InName), 
		Hash(std::hash<std::string>()(TypeName)),
		SourceFileName(InSourceFileName),
		HashedShaderFilename(std::hash<std::string>()(SourceFileName)),
		EntryPoint(InEntryPoint), 
		Frequency(InFrequency), 
		TotalPermutationCount(InTotalPermutationCount),
		ConstructRef(InConstructRef),
		ModifyCompilationEnvironmentRef(InModifyCompilationEnvironmentRef),
		ShouldCompilePermutationRef(InShouldCompilePermutationRef),
		RootParametersMetadata(InRootParametersMetadata)
	{
		GetTypeList().emplace_front(this);
	}

	Shader::ShaderMapBase::~ShaderMapBase()
	{
	}

	void Shader::ShaderMapBase::InitResource()
	{
		Resource.reset();
		if (Code)
		{
			//Code->Finalize();
			Resource = std::make_shared<ShaderMapResource_InlineCode>(Code.get());
			//BeginInitResource(Resource);
		}
	}

	void Shader::ShaderMapBase::FinalizeContent()
	{
		InitResource();
	}

	ImplDeCtor(RenderShader);
	ImplDeDtor(RenderShader);

	RenderShader::RenderShader(const CompiledShaderInitializer& Initializer)
		:
		ResourceIndex(white::INDEX_NONE),
		Meta(Initializer.Meta)
	{
#if WE_EDITOR
		wassume(Initializer.OutputHash != Digest::SHAHash());

		OutputHash = Initializer.OutputHash;
#endif
	}

	Shader::RenderShader::CompiledShaderInitializer::CompiledShaderInitializer(ShaderMeta* InMeta, const ShaderCompilerOutput& CompilerOutput)
		:Meta(InMeta),Code(CompilerOutput.ShaderCode.GetReadAccess()),ParameterMap(CompilerOutput.ParameterMap),OutputHash(CompilerOutput.OutputHash)
	{
	}

	ShaderType RenderShader::GetShaderType() const
	{
		return Meta->GetShaderType();
	}

	void RenderShader::Finalize(const ShaderMapResourceCode* Code)
	{
		Digest::SHAHash Hash{};
#if WE_EDITOR
		Hash = OutputHash;
#endif
		const int32 NewResourceIndex = Code->FindShaderIndex(Hash);
		WAssert(NewResourceIndex != white::INDEX_NONE, "Missing shader code");
		ResourceIndex = NewResourceIndex;
	}

	static void FillParameterMapByShaderInfo(ShaderParameterMap& target, const ShaderInfo& src);

	BuiltInShaderMap GGlobalBuiltInShaderMap;

	std::string ParseAndMoveShaderParametersToRootConstantBuffer(std::string code,const asset::X::Shader::ShaderCompilerInput& input);
	void PullRootShaderParametersLayout(asset::X::Shader::ShaderCompilerInput& Input, const ShaderParametersMetadata& ParameterMeta);

	white::coroutine::Task<void> operator>>(AsyncArchive& archive, platform::Render::ShaderInitializer& initializer)
	{
		auto pBlob = const_cast<ShaderBlob*>(initializer.pBlob);

		co_await (archive >> pBlob->second);

		if (archive.IsLoading())
		{
			pBlob->first = std::make_unique<stdex::byte[]>(initializer.pBlob->second);
		}
		co_await archive.Serialize(pBlob->first.get(), pBlob->second);

		auto pInfo = const_cast<ShaderInfo*>(initializer.pInfo);
		co_await pInfo->Serialize(archive);
	}

	fs::path RedirectPath(const std::string& path)
	{
		static fs::path shaders_path = WhiteEngine::PathSet::EngineDir() / "Shaders";
		return shaders_path / path;
	}

	fs::file_time_type ReidrectFileTime(const std::string& path)
	{
		auto local_path = RedirectPath(path);

		std::error_code ec;
		auto last_write_time = fs::last_write_time(local_path, ec);

		if (ec)
		{
			last_write_time = fs::last_write_time(path, ec);
		}

		wconstraint(!ec);
		return last_write_time;
	}

	class FileTimeCacheContext
	{
	public:
		bool CheckFileTime(const std::string& key,bool redirect = true)
		{
			{
				std::shared_lock lock{ mutex };

				auto itr = caches.find(key);

				if (itr != caches.end())
					return itr->second;
			}

			fs::path path_key = key;
			if (redirect)
			{
				fs::path local_path = RedirectPath(key);
				if (fs::exists(local_path))
					path_key = local_path;
			}
			std::error_code ec;
			auto last_write_time = fs::last_write_time(path_key,ec);
			if (ec)
			{
				spdlog::warn("CheckFileTime error:{}", ec.message());
				return false;
			}

			auto db_time = WhiteEngine::ShaderDB::QueryTime(key);
			if (!db_time.has_value() || db_time.value() != last_write_time)
			{
				std::unique_lock lock{ mutex };
				caches.emplace(key, false);
				return false;
			}

			std::unique_lock lock{ mutex };
			caches.emplace(key, true);
			return true;
		}

		bool CheckDependentTime(const std::string& key,bool updatedb)
		{
			{
				std::shared_lock lock{ mutex };

				auto itr = caches.find(key);

				if (itr != caches.end())
					return itr->second;
			}


			auto local_path = RedirectPath(key);

			if (fs::exists(local_path))
			{
				auto fs_key = local_path.string();

				if (!CheckFileTime(fs_key,false))
				{
					if (updatedb)
					{
						auto last_write_time = fs::last_write_time(local_path);
						white::coroutine::SyncWait(WhiteEngine::ShaderDB::UpdateTime(fs_key, last_write_time));
					}
					return false;
				}
				return true;
			}

			std::unique_lock lock{ mutex };
			caches.emplace(key, false);
			return false;
		}

		std::shared_mutex mutex;
		std::unordered_map<std::string, bool> caches;
	};

	white::coroutine::Task<bool> IsShaderCache(BuiltInShaderMeta* meta,const fs::path& path, FileTimeCacheContext& contenxt);

	white::coroutine::Task<void> WriteShaderCache(BuiltInShaderMeta* meta, std::set<std::string>&& sets);

	void GenerateOuput(platform::Render::ShaderInitializer initializer, ShaderCompilerOutput& Output)
	{
		MemoryWriter Ar(Output.ShaderCode.GetWriteAccess());

		Output.Type = initializer.pInfo->Type;
		if (IsRayTracingShader(initializer.pInfo->Type))
		{
			auto& RayTracingInfos =const_cast<RayTracingShaderInfo&>(initializer.pInfo->RayTracingInfos.value());
			Ar >> RayTracingInfos.EntryPoint;
			Ar >> RayTracingInfos.AnyHitEntryPoint;
			Ar >> RayTracingInfos.IntersectionEntryPoint;
		}

		Ar.Serialize(initializer.pBlob->first.get(), initializer.pBlob->second);

		// Append data that is generate from the shader code and assist the usage, mostly needed for DX12 
		{
			Output.ShaderCode.AddOptionalData(initializer.pInfo->ResourceCounts);

			bool bGlobalUniformBufferUsed = false;
			std::vector<std::string> UniformBufferNames;
			for (auto& cb : initializer.pInfo->ConstantBufferInfos)
			{
				UniformBufferNames.emplace_back(cb.name);
				if (cb.name == "$Globals")
					bGlobalUniformBufferUsed = true;
			}

			std::vector<uint8> UniformBufferNameBytes;
			MemoryWriter UniformBufferNameWriter(UniformBufferNameBytes);
			UniformBufferNameWriter >> UniformBufferNames;

			Output.ShaderCode.AddOptionalData('u', UniformBufferNameBytes.data(), static_cast<uint32>(UniformBufferNameBytes.size()));
			Output.ShaderCode.AddOptionalData('b', reinterpret_cast<uint8*>(&bGlobalUniformBufferUsed), sizeof(bool));
			Output.ShaderCode.AddOptionalData('t',reinterpret_cast<const uint8*>(&initializer.pInfo->Type), sizeof(initializer.pInfo->Type));
		}

		FillParameterMapByShaderInfo(Output.ParameterMap, *initializer.pInfo);

		Output.GenerateOutputHash();
		Output.CompressOutput(GetShaderCompressionFormat());
	}

	void InsertCompileOuput(BuiltInShaderMeta* meta, platform::Render::ShaderInitializer initializer, int32 PermutationId)
	{
		auto pBuiltInMeta = meta->GetBuiltInShaderType();
		if (!pBuiltInMeta)
			return;

		if (IsRayTracingShader(meta->GetShaderType()))
		{
			ShaderCompilerOutput Output;
			GenerateOuput(initializer, Output);

			auto Section = GGlobalBuiltInShaderMap.FindOrAddSection(meta);

			Section->GetResourceCode()->AddShaderCompilerOutput(Output);

			RenderShader::CompiledShaderInitializer compileOuput {meta,Output };

			auto pShader = static_cast<BuiltInRayTracingShader*>(pBuiltInMeta->Construct(compileOuput));

			GGlobalBuiltInShaderMap.FindOrAddShader(meta, PermutationId, pShader);
		}
		else {
			ShaderCompilerOutput Output;
			GenerateOuput(initializer,Output);

			auto Section = GGlobalBuiltInShaderMap.FindOrAddSection(meta);

			Section->GetResourceCode()->AddShaderCompilerOutput(Output);

			RenderShader::CompiledShaderInitializer compileOuput{ meta,Output };

			auto pShader = pBuiltInMeta->Construct(compileOuput);

			GGlobalBuiltInShaderMap.FindOrAddShader(meta, PermutationId, pShader);
		}
	}

	white::coroutine::Task<void> CompileBuiltInShader(BuiltInShaderMeta* meta,int32 PermutationId,AsyncArchive& archive,std::vector<std::string>& dependents)
	{
		WFL_DEBUG_DECL_TIMER(Commpile, std::format("CompileBuiltInShader {} Entry:{} Permutation={} ", meta->GetSourceFileName(), meta->GetEntryPoint(), PermutationId));

		//shader build system
		asset::X::Shader::ShaderCompilerInput input;

		input.EntryPoint = meta->GetEntryPoint();
		input.Type = meta->GetShaderType();
		input.SourceName = meta->GetSourceFileName();

		fs::path local_path = RedirectPath(meta->GetSourceFileName());
		if(!fs::exists(local_path))
			local_path = meta->GetSourceFileName();

		auto last_write_time = fs::last_write_time(local_path);

		//meta_write_time

		if (meta->GetRootParametersMetadata())
		{
			PullRootShaderParametersLayout(input, *meta->GetRootParametersMetadata());
			std::sort(input.RootParameterBindings.begin(),input.RootParameterBindings.end());
		}

		// Allow the shader type to modify the compile environment.
		meta->SetupCompileEnvironment(PermutationId, input.Environment);
		asset::X::Shader::AppendCompilerEnvironment(input.Environment, input.Type);

		//loop sm_version in future,and reduce by check preprocessShader hash
		input.Environment.SetDefine("SM_VERSION", "66");

		auto Code = co_await platform::X::GenHlslShaderAsync(local_path);

		auto PreprocessRet = asset::X::Shader::PreprocessShader(Code, input);
		Code = PreprocessRet.Code;

		if (IsRayTracingShader(meta->GetShaderType()))
			Code = ParseAndMoveShaderParametersToRootConstantBuffer(Code, input);

		input.Code = Code;

		ShaderInfo Info{ input.Type };

		uint32 Flags = D3DFlags::D3DCOMPILE_HLSL_2021;

#ifndef NDEBUG
		Flags |= D3DFlags::D3DCOMPILE_DEBUG;
#else
		Flags |= D3DFlags::D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

		auto blob = asset::X::Shader::CompileAndReflect(input,
			Flags,
			&Info
		);

		platform::Render::ShaderInitializer initializer{
			.pBlob = &blob,
			.pInfo = &Info
		};
		InsertCompileOuput(meta, initializer, PermutationId);

		co_await(archive >> PermutationId);
		co_await(archive >> initializer);

		dependents = std::move(PreprocessRet.Dependent.files);
		co_return;
	}

	fs::path FormatShaderSavePath(BuiltInShaderMeta* meta);

	void CompileShaderMap()
	{
		spdlog::stopwatch sw;

		std::vector< white::coroutine::Task<void>> tasks;
		FileTimeCacheContext context;
		std::set<std::string> ShadeFileNames;
		for (auto meta : ShaderMeta::GetTypeList())
		{
			//TODO:dispatch type
			if (auto pBuiltInMeta = meta->GetBuiltInShaderType())
			{
				ShadeFileNames.emplace(meta->GetSourceFileName());
				tasks.emplace_back([&context](BuiltInShaderMeta* meta) -> white::coroutine::Task<void>
				{
					std::vector< white::coroutine::Task<void>> tasks;

					int32 PermutationCountToCompile = 0;

					auto localcachepath = FormatShaderSavePath(meta);

					if (co_await IsShaderCache(meta, localcachepath, context))
						co_return;

					auto pArchive = white::unique_raw(WhiteEngine::CreateFileWriter(localcachepath));

					auto hash = std::hash<std::string>()(meta->GetSourceFileName());
					//Serialize meta
					co_await pArchive->Serialize(&hash,sizeof(hash));

					std::set<std::string> Dependents;
					for (int32 PermutationId = 0; PermutationId < meta->GetPermutationCount(); PermutationId++)
					{
						if (!meta->ShouldCompilePermutation(PermutationId))
							continue;

						std::vector<std::string> dependents;
						co_await CompileBuiltInShader(meta, PermutationId,*pArchive, dependents);
						Dependents.insert(dependents.begin(), dependents.end());
					}
					for (auto& dependent : Dependents)
					{
						context.CheckDependentTime(dependent,true);
					}
					co_await WriteShaderCache(meta,std::move(Dependents));
				}(pBuiltInMeta));
			}
		}

		white::coroutine::SyncWait(white::coroutine::WhenAllReady(std::move(tasks)));
		for (auto& key : ShadeFileNames)
		{
			auto Section = GGlobalBuiltInShaderMap.FindSection(std::hash<std::string>()(key));
			if (Section)
			{
				Section->FinalizeContent();
			}
		}

		spdlog::info("CompileShaderMap: {} seconds", sw);
	}

	void FillParameterMapByShaderInfo(ShaderParameterMap& target, const ShaderInfo& src)
	{
		for (auto& cb : src.ConstantBufferInfos)
		{
			auto& name = cb.name;

			bool bGlobalCB = name == "$Globals";
			if (bGlobalCB)
			{
				for (auto& var : cb.var_desc)
				{
					target.AddParameterAllocation(var.name, cb.bind_point,
						var.start_offset, var.size, ShaderParamClass::LooseData);
				}
			}
			else
			{
				target.AddParameterAllocation(name, 0,cb.bind_point,
					 cb.size, ShaderParamClass::UniformBuffer);
			}
		}
		for (auto& br : src.BoundResourceInfos)
		{
			ShaderParamClass Class = static_cast<ShaderParamClass>(br.type);

			target.AddParameterAllocation(br.name, 0, br.bind_point,
				 1, Class);
		}
	}

	BuiltInShaderMap* Shader::GetBuiltInShaderMap()
	{
		return &GGlobalBuiltInShaderMap;
	}

	namespace fs = std::filesystem;

	white::coroutine::Task<bool> IsShaderCache(BuiltInShaderMeta* meta, const fs::path& path, FileTimeCacheContext& context) 
	{
		if (!fs::exists(path))
			co_return false;

		if (!context.CheckFileTime(meta->GetSourceFileName()))
			co_return false;

		//read manifest
		auto dependents = WhiteEngine::ShaderDB::QueryDependent(meta->GetSourceFileName());
		if (!dependents.has_value())
			co_return false;

		for(auto& dependent : *dependents)
		{
			if (!context.CheckDependentTime(dependent,false))
				co_return false;
		}

		//load cache
		auto pArchive = white::unique_raw(WhiteEngine::CreateFileReader(path));

		auto hash = std::hash<std::string>()(meta->GetSourceFileName());
		//Serialize meta
		co_await pArchive->Serialize(&hash, sizeof(hash));

		for (int32 PermutationId = 0; PermutationId < meta->GetPermutationCount(); PermutationId++)
		{
			//cause error when impl change
			if (!meta->ShouldCompilePermutation(PermutationId))
				continue;

			ShaderInfo Info{ meta->GetShaderType() };
			ShaderBlob Blob;

			platform::Render::ShaderInitializer initializer{
				.pBlob = &Blob,
				.pInfo = &Info
			};

			int32 CheckId;
			co_await (*pArchive >> CheckId);
			WAssert(CheckId == PermutationId, "invalid shader achive");
			co_await(*pArchive >> initializer);

			InsertCompileOuput(meta, initializer, PermutationId);
		}

		co_return true;
	}

	fs::path FormatShaderSavePath(BuiltInShaderMeta* meta) {
		static auto save_dir = [] {
			auto directory = (WhiteEngine::PathSet::EngineIntermediateDir() / "Shaders");
			fs::create_directories(directory);
			return directory.string();
		}();

		std::string cache_filename;

		auto hash = std::hash<std::string>()(meta->GetSourceFileName());
		auto entry_hash = std::hash<std::string>()(meta->GetEntryPoint());
		std::format_to(std::back_inserter(cache_filename), "{}/{:X}{:X}", save_dir, hash,entry_hash);

		std::array<char, 4> key = {};
		auto itr = meta->GetSourceFileName().rbegin();
		for (int i = 0; itr != meta->GetSourceFileName().rend() && i < 8; ++i, ++itr)
		{
			if (i > 3)
				key[i - 4] = *itr;
		}

		std::format_to(std::back_inserter(cache_filename), "_{:X}", *reinterpret_cast<unsigned*>(key.data()));

		return cache_filename;
	}

	white::coroutine::Task<void> WriteShaderCache(BuiltInShaderMeta* meta,std::set<std::string>&& sets) {
		auto last_write_time = ReidrectFileTime(meta->GetSourceFileName());

		std::vector<std::string> dependents(sets.begin(), sets.end());
		co_await WhiteEngine::ShaderDB::UpdateTime(meta->GetSourceFileName(), last_write_time);
		co_await WhiteEngine::ShaderDB::UpdateDependent(meta->GetSourceFileName(), dependents);

		co_return;
	}

}

RenderShader* Shader::ShaderMapContent::GetShader(size_t TypeNameHash, int32 PermutationId) const
{
	auto Hash = (uint16)CityHash128to64({ TypeNameHash,(uint64)PermutationId });

	auto range = ShaderHash.equal_range(Hash);
	for (auto itr = range.first; itr != range.second; ++itr)
	{
		auto Index = itr->second;
		if (ShaderTypes[Index] == TypeNameHash && ShaderPermutations[Index] == PermutationId)
		{
			return Shaders[Index];
		}
	}
	return nullptr;
}

RenderShader* Shader::ShaderMapContent::FindOrAddShader(size_t TypeNameHash, int32 PermutationId, RenderShader* Shader)
{
	{
		std::shared_lock lock{ ShaderMutex };
		auto FindShader = GetShader(TypeNameHash, PermutationId);

		if (FindShader != nullptr)
			return FindShader;
	}

	std::unique_lock lock{ ShaderMutex };

	auto Hash = (uint16)CityHash128to64({ TypeNameHash,(uint64)PermutationId });

	const int32 Index = GetNumShaders();

	ShaderHash.emplace(Hash, Index);
	Shaders.emplace_back(Shader);
	ShaderTypes.emplace_back(TypeNameHash);
	ShaderPermutations.emplace_back(PermutationId);

	return Shader;
}

void Shader::ShaderMapContent::Clear()
{
	for (auto Shader : Shaders)
	{
		delete Shader;
	}

	Shaders.clear();
	ShaderTypes.clear();
	ShaderPermutations.clear();
	ShaderHash.clear();
}

uint32 Shader::ShaderMapContent::GetNumShaders() const
{
	return static_cast<int32>(Shaders.size());
}

void Shader::ShaderMapContent::Finalize(const ShaderMapResourceCode* Code)
{
	for (auto Shader : Shaders)
	{
		Shader->Finalize(Code);
	}
}

void Shader::PullRootShaderParametersLayout(asset::X::Shader::ShaderCompilerInput& Input, const ShaderParametersMetadata& ParameterMeta)
{
	for (auto& member : ParameterMeta.GetMembers())
	{
		auto ShaderType = member.GetShaderType();

		const bool bIsVariableNativeType = (
			ShaderType >= SPT_uint &&
			ShaderType <= SPT_float4x4
			);

		if (bIsVariableNativeType)
		{
			ShaderCompilerInput::RootParameterBinding RootParameterBinding;
			RootParameterBinding.Name = member.GetName();
			RootParameterBinding.ExpectedShaderType = asset::ShadersAsset::GetTypeName(member.GetShaderType());
			RootParameterBinding.ByteOffset = member.GetOffset();
			Input.RootParameterBindings.emplace_back(RootParameterBinding);
		}
	}
}

std::string Shader::ParseAndMoveShaderParametersToRootConstantBuffer(std::string code, const asset::X::Shader::ShaderCompilerInput& input)
{
	if (input.RootParameterBindings.empty())
		return code;

	struct ParsedShaderParameter
	{
		std::string Type;

		bool IsFound() const
		{
			return !Type.empty();
		}
	};
	std::map<std::string, ParsedShaderParameter> ParsedParameters;

	for (auto& Member : input.RootParameterBindings)
	{
		ParsedParameters.emplace(Member.Name, ParsedShaderParameter{});
	}


	// Browse the code for global shader parameter, Save their type and erase them white spaces.
	{
		enum class EState
		{
			// When to look for something to scan.
			Scanning,

			// When going to next ; in the global scope and reset.
			GoToNextSemicolonAndReset,

			// Parsing what might be a type of the parameter.
			ParsingPotentialType,
			FinishedPotentialType,

			// Parsing what might be a name of the parameter.
			ParsingPotentialName,
			FinishedPotentialName,

			// Parsing what looks like array of the parameter.
			ParsingPotentialArraySize,
			FinishedArraySize,

			// Found a parameter, just finish to it's semi colon.
			FoundParameter,
		};

		const int32 ShaderSourceLen =static_cast<int32>(code.size());

		int32 CurrentPragamLineoffset = -1;
		int32 CurrentLineoffset = 0;

		int32 TypeStartPos = -1;
		int32 TypeEndPos = -1;
		int32 NameStartPos = -1;
		int32 NameEndPos = -1;
		int32 ScopeIndent = 0;

		EState State = EState::Scanning;
		bool bGoToNextLine = false;

		auto ResetState = [&]()
		{
			TypeStartPos = -1;
			TypeEndPos = -1;
			NameStartPos = -1;
			NameEndPos = -1;
			State = EState::Scanning;
		};

		auto EmitError = [&](const std::string& ErrorMessage)
		{
			WE_LogError(ErrorMessage.c_str());
		};

		auto EmitUnpextectedHLSLSyntaxError = [&]()
		{
			EmitError("Unexpected syntax when parsing shader parameters from shader code.");
			State = EState::GoToNextSemicolonAndReset;
		};

		for (int32 Cursor = 0; Cursor < ShaderSourceLen; Cursor++)
		{
			const auto Char = code[Cursor];

			auto FoundShaderParameter = [&]()
			{
				wconstraint(Char == ';');
				wconstraint(TypeStartPos != -1);
				wconstraint(TypeEndPos != -1);
				wconstraint(NameStartPos != -1);
				wconstraint(NameEndPos != -1);

				auto Type = code.substr(TypeStartPos, TypeEndPos - TypeStartPos + 1);
				auto Name = code.substr(NameStartPos, NameEndPos - NameStartPos + 1);

				if (ParsedParameters.contains(Name))
				{
					if (ParsedParameters[Name].IsFound())
					{
						// If it has already been found, it means it is duplicated. Do nothing and let the shader compiler throw the error.
					}
					else
					{
						ParsedParameters[Name] = ParsedShaderParameter{.Type = Type};

						// Erases this shader parameter conserving the same line numbers.
						if (true)
						{
							for (int32 j = TypeStartPos; j <= Cursor; j++)
							{
								if (code[j] != '\r' && code[j] != '\n')
									code[j] = ' ';
							}
						}
					}
				}

				ResetState();
			};

			const bool bIsWhiteSpace = Char == ' ' || Char == '\t' || Char == '\r' || Char == '\n';
			const bool bIsLetter = (Char >= 'a' && Char <= 'z') || (Char >= 'A' && Char <= 'Z');
			const bool bIsNumber = Char >= '0' && Char <= '9';

			const auto UpComing = (code.c_str()) + Cursor;
			const int32 RemainingSize = ShaderSourceLen - Cursor;

			CurrentLineoffset += Char == '\n';

			// Go to the next line if this is a preprocessor macro.
			if (bGoToNextLine)
			{
				if (Char == '\n')
				{
					bGoToNextLine = false;
				}
				continue;
			}
			else if (Char == '#')
			{
				if (RemainingSize > 6 && std::strncmp(UpComing, "#line ", 6) == 0)
				{
					CurrentPragamLineoffset = Cursor;
					CurrentLineoffset = -1; // that will be incremented to 0 when reaching the \n at the end of the #line
				}

				bGoToNextLine = true;
				continue;
			}

			// If within a scope, just carry on until outside the scope.
			if (ScopeIndent > 0 || Char == '{')
			{
				if (Char == '{')
				{
					ScopeIndent++;
				}
				else if (Char == '}')
				{
					ScopeIndent--;
					if (ScopeIndent == 0)
					{
						ResetState();
					}
				}
				continue;
			}

			if (State == EState::Scanning)
			{
				if (bIsLetter)
				{
					static const char* KeywordTable[] = {
						"enum",
						"class",
						"const",
						"struct",
						"static",
					};
					static int32 KeywordTableSize[] = { 4, 5, 5, 6, 6 };

					int32 RecognisedKeywordId = -1;
					for (int32 KeywordId = 0; KeywordId < white::arrlen(KeywordTable); KeywordId++)
					{
						const char* Keyword = KeywordTable[KeywordId];
						const int32 KeywordSize = KeywordTableSize[KeywordId];

						if (RemainingSize > KeywordSize)
						{
							char KeywordEndTestChar = UpComing[KeywordSize];

							if ((KeywordEndTestChar == ' ' || KeywordEndTestChar == '\r' || KeywordEndTestChar == '\n' || KeywordEndTestChar == '\t') &&
								std::strncmp(UpComing, Keyword, KeywordSize) == 0)
							{
								RecognisedKeywordId = KeywordId;
								break;
							}
						}
					}

					if (RecognisedKeywordId == -1)
					{
						// Might have found beginning of the type of a parameter.
						State = EState::ParsingPotentialType;
						TypeStartPos = Cursor;
					}
					else if (RecognisedKeywordId == 2)
					{
						// Ignore the const keywords, but still parse given it might still be a shader parameter.
						Cursor += KeywordTableSize[RecognisedKeywordId];
					}
					else
					{
						// Purposefully ignore enum, class, struct, static
						State = EState::GoToNextSemicolonAndReset;
					}
				}
				else if (bIsWhiteSpace)
				{
					// Keep parsing void.
				}
				else if (Char == ';')
				{
					// Looks like redundant semicolon, just ignore and keep scanning.
				}
				else
				{
					// No idea what this is, just go to next semi colon.
					State = EState::GoToNextSemicolonAndReset;
				}
			}
			else if (State == EState::GoToNextSemicolonAndReset)
			{
				// If need to go to next global semicolon and reach it. Resume browsing.
				if (Char == ';')
				{
					ResetState();
				}
			}
			else if (State == EState::ParsingPotentialType)
			{
				// Found character legal for a type...
				if (bIsLetter ||
					bIsNumber ||
					Char == '<' || Char == '>' || Char == '_')
				{
					// Keep browsing what might be type of the parameter.
				}
				else if (bIsWhiteSpace)
				{
					// Might have found a type.
					State = EState::FinishedPotentialType;
					TypeEndPos = Cursor - 1;
				}
				else
				{
					// Found unexpected character in the type.
					State = EState::GoToNextSemicolonAndReset;
				}
			}
			else if (State == EState::FinishedPotentialType)
			{
				if (bIsLetter)
				{
					// Might have found beginning of the name of a parameter.
					State = EState::ParsingPotentialName;
					NameStartPos = Cursor;
				}
				else if (bIsWhiteSpace)
				{
					// Keep parsing void.
				}
				else
				{
					// No idea what this is, just go to next semi colon.
					State = EState::GoToNextSemicolonAndReset;
				}
			}
			else if (State == EState::ParsingPotentialName)
			{
				// Found character legal for a name...
				if (bIsLetter ||
					bIsNumber ||
					Char == '_')
				{
					// Keep browsing what might be name of the parameter.
				}
				else if (Char == ':' || Char == '=')
				{
					// Found a parameter with syntax:
					// uint MyParameter : <whatever>;
					// uint MyParameter = <DefaultValue>;
					NameEndPos = Cursor - 1;
					State = EState::FoundParameter;
				}
				else if (Char == ';')
				{
					// Found a parameter with syntax:
					// uint MyParameter;
					NameEndPos = Cursor - 1;
					FoundShaderParameter();
				}
				else if (Char == '[')
				{
					// Syntax:
					//  uint MyArray[
					NameEndPos = Cursor - 1;
					State = EState::ParsingPotentialArraySize;
				}
				else if (bIsWhiteSpace)
				{
					// Might have found a name.
					// uint MyParameter <Still need to know what is after>;
					NameEndPos = Cursor - 1;
					State = EState::FinishedPotentialName;
				}
				else
				{
					// Found unexpected character in the name.
					// syntax:
					// uint MyFunction(<Don't care what is after>
					State = EState::GoToNextSemicolonAndReset;
				}
			}
			else if (
				State == EState::FinishedPotentialName ||
				State == EState::FinishedArraySize)
			{
				if (Char == ';')
				{
					// Found a parameter with syntax:
					// uint MyParameter <a bit of OK stuf>;
					FoundShaderParameter();
				}
				else if (Char == ':')
				{
					// Found a parameter with syntax:
					// uint MyParameter <a bit of OK stuf> : <Ignore all this crap>;
					State = EState::FoundParameter;
				}
				else if (Char == '=')
				{
					// Found syntax that doesn't make any sens:
					// uint MyParameter <a bit of OK stuf> = <Ignore all this crap>;
					State = EState::FoundParameter;
					// TDOO: should error out that this is useless.
				}
				else if (Char == '[')
				{
					if (State == EState::FinishedPotentialName)
					{
						// Syntax:
						//  uint MyArray [
						State = EState::ParsingPotentialArraySize;
					}
					else
					{
						EmitError("Shader parameters can only support one dimensional array");
					}
				}
				else if (bIsWhiteSpace)
				{
					// Keep parsing void.
				}
				else
				{
					// Found unexpected stuff.
					State = EState::GoToNextSemicolonAndReset;
				}
			}
			else if (State == EState::ParsingPotentialArraySize)
			{
				if (Char == ']')
				{
					State = EState::FinishedArraySize;
				}
				else if (Char == ';')
				{
					EmitUnpextectedHLSLSyntaxError();
				}
				else
				{
					// Keep going through the array size that might be a complex expression.
				}
			}
			else if (State == EState::FoundParameter)
			{
				if (Char == ';')
				{
					FoundShaderParameter();
				}
				else
				{
					// Cary on skipping all crap we don't care about shader parameter until we find it's semi colon.
				}
			}
			else
			{
				throw white::unimplemented();
			}
		} // for (int32 Cursor = 0; Cursor < PreprocessedShaderSource.Len(); Cursor++)
	}

	std::string rootcbuffer;
	for (auto& Member : input.RootParameterBindings)
	{
		std::string register_alloc = white::to_string(Member.ByteOffset/16);

		switch (Member.ByteOffset % 16)
		{
		case 0:
			break;
		case 4:
			register_alloc += ".y";
			break;
		case 8:
			register_alloc += ".z";
			break;
		case 12:
			register_alloc += ".w";
			break;
		default:
			wassume(false);//not alignment to 4 bytes
		}

		const auto& ParsedParameter = ParsedParameters[Member.Name];

		rootcbuffer += white::sfmt("%s %s : packoffset(c%s);\r\n", ParsedParameter.Type.c_str(), Member.Name.c_str(), register_alloc.c_str());
	}

	std::string new_code = white::sfmt(
		"%s %s\r\n"
		"{\r\n"
		"%s"
		"}\r\n\r\n%s",
		"cbuffer",
		ShaderParametersMetadata::kRootUniformBufferBindingName,
		rootcbuffer.c_str(),
		code.c_str()
	);

	return new_code;
}