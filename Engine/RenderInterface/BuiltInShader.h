#pragma once

#include "RenderInterface/Shader.h"
#include <shared_mutex>

#define PR_NAMESPACE_BEGIN  namespace platform::Render {
#define PR_NAMESPACE_END }

PR_NAMESPACE_BEGIN
inline namespace Shader
{
	using FBuiltInShaderPermutationParameters = FShaderPermutationParameters;


	class BuiltInShaderMeta : public ShaderMeta
	{
	public:
		using CompiledShaderInitializer = RenderShader::CompiledShaderInitializer;
		typedef RenderShader* (*ConstructCompiledType)(const CompiledShaderInitializer&);

		BuiltInShaderMeta(
			const char* InName,
			const char* InSourceFileName,
			const char* InEntryPoint,
			platform::Render::ShaderType  InFrequency,
			int32 TotalPermutationCount,
			ConstructType InConstructRef,
			ModifyCompilationEnvironmentType InModifyCompilationEnvironmentRef,
			ShouldCompilePermutationType InShouldCompilePermutationRef,
			uint32 InTypeSize,
			const ShaderParametersMetadata* InRootParametersMetadata,
			ConstructCompiledType InConstructCompiledRef
		):
		 ShaderMeta(EShaderMetaForDownCast::BuitlIn,InName,InSourceFileName,InEntryPoint,InFrequency, 
			 TotalPermutationCount,
			 InConstructRef, 
			 InModifyCompilationEnvironmentRef,
			 InShouldCompilePermutationRef,
			 InTypeSize,
			 InRootParametersMetadata
			 ),
			ConstructCompiledRef(InConstructCompiledRef)
		{}

		RenderShader* Construct(const CompiledShaderInitializer& initializer) const
		{
			return (*ConstructCompiledRef)(initializer);
		}

		/**
		* Sets up the environment used to compile an instance of this shader type.
		* @param Platform - Platform to compile for.
		* @param Environment - The shader compile environment that the function modifies.
		*/
		void SetupCompileEnvironment(int32 PermutationId, FShaderCompilerEnvironment& Environment)
		{
			// Allow the shader type to modify its compile environment.
			ModifyCompilationEnvironment(FBuiltInShaderPermutationParameters(PermutationId), Environment);
		}

		/**
		 * Checks if the shader type should be cached for a particular platform.
		* @param Platform - The platform to check.
		* @return True if this shader type should be cached.
		*/
		bool ShouldCompilePermutation(int32 PermutationId) const
		{
			return ShaderMeta::ShouldCompilePermutation(FBuiltInShaderPermutationParameters(PermutationId));
		}

	private:
		ConstructCompiledType ConstructCompiledRef;
	};

	class BuiltInShader :public RenderShader
	{
	public:
		using DerivedType = BuiltInShader;
		using ShaderMetaType = BuiltInShaderMeta;
	public:
		BuiltInShader(const ShaderMetaType::CompiledShaderInitializer& Initializer)
			:RenderShader(Initializer)
		{}

		BuiltInShader(){}
	};

	class BuiltInShaderMapContent :public ShaderMapContent
	{
		friend class BuiltInShaderMap;
		friend class BuiltInShaderMapSection;
	public:
		std::size_t GetSourceFileNameHash() const {
			return HashedSourceFilename;
		}

	private:
		BuiltInShaderMapContent(std::size_t InHashedSourceFilename)
			:ShaderMapContent()
			, HashedSourceFilename(InHashedSourceFilename)
		{}
	private:
		std::size_t HashedSourceFilename;
	};

	class BuiltInShaderMapSection :public ShaderMapBase
	{
	public:
		friend class BuiltInShaderMap;

		void FinalizeContent()
		{
			Content.Finalize(this->GetResourceCode());
			ShaderMapBase::FinalizeContent();
		}
	private:
		BuiltInShaderMapSection(std::size_t InHashedSourceFilename)
			:Content(InHashedSourceFilename)
		{}

		ShaderRef<RenderShader> GetShader(ShaderMeta* ShaderType, int32 PermutationId = 0) const;


		BuiltInShaderMapContent Content;
	};

	class BuiltInShaderMap
	{
	public:
		using FHashedName = uint64_t;

		~BuiltInShaderMap();

		ShaderRef<RenderShader> GetShader(ShaderMeta* ShaderType, int32 PermutationId = 0) const;

		/** Finds the shader with the given type.  Asserts on failure. */
		template<typename ShaderContentType>
		ShaderRef<ShaderContentType> GetShader(int32 PermutationId = 0) const
		{
			auto Shader = GetShader(&ShaderContentType::StaticType, PermutationId);
			WAssert(Shader.IsValid(), white::sfmt("Failed to find shader type %s in Platform %s", ShaderContentType::StaticType.GetTypeName().c_str(), "PCD3D_SM5").c_str());
			return ShaderRef<ShaderContentType>::Cast(Shader);
		}

		/** Finds the shader with the given type.  Asserts on failure. */
		template<typename ShaderContentType>
		ShaderRef<ShaderContentType> GetShader(const typename ShaderContentType::PermutationDomain& PermutationVector) const
		{
			return GetShader<ShaderContentType>(PermutationVector.ToDimensionValueId());
		}

		RenderShader* FindOrAddShader(const ShaderMeta* ShaderType, int32 PermutationId, RenderShader* Shader);

		void AddSection(BuiltInShaderMapSection* InSection);
		BuiltInShaderMapSection* FindSection(const FHashedName& HashedShaderFilename);
		BuiltInShaderMapSection* FindOrAddSection(const ShaderMeta* ShaderType);
	private:
		std::shared_mutex MapMutex;
		std::unordered_map<std::size_t, BuiltInShaderMapSection*> SectionMap;
	};

	BuiltInShaderMap* GetBuiltInShaderMap();


#define EXPORTED_BUILTIN_SHADER(ShaderClass) EXPORTED_SHADER_TYPE(ShaderClass,BuiltIn)

#define IMPLEMENT_BUILTIN_SHADER(ShaderClass,SourceFileName,FunctionName,Frequency) \
	ShaderClass::ShaderMetaType ShaderClass::StaticType( \
		#ShaderClass, \
		SourceFileName, \
		FunctionName, \
		Frequency, \
		SHADER_VTABLE(ShaderClass),\
		sizeof(ShaderClass),\
		ShaderClass::GetRootParametersMetadata(),\
		ShaderClass::ConstructCompiledInstance\
	);\
	ShaderClass::ShaderClass(const ShaderMetaType::CompiledShaderInitializer& Initializer) \
		:DerivedType(Initializer)\
	{\
		if constexpr (RootParameterStruct)\
			platform::Render::BindForRootShaderParameters(this, Initializer.ParameterMap); \
		else\
			platform::Render::BindForLegacyShaderParameters(this, Initializer.ParameterMap); \
	}\
	ShaderClass::ShaderClass() \
	{ }
}
PR_NAMESPACE_END

#undef PR_NAMESPACE_BEGIN
#undef PR_NAMESPACE_END