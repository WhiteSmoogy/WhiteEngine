#pragma once

#include <WBase/smart_ptr.hpp>
#include "emacro.h"
#include "ShaderCore.h"
#include "ShaderPermutation.h"
#include <concepts>
#include <shared_mutex>

#define PR_NAMESPACE_BEGIN  namespace platform::Render {
#define PR_NAMESPACE_END }

PR_NAMESPACE_BEGIN
class HardwareShader;
class VertexHWShader;
class PixelHWShader;
class GeometryHWShader;
class ComputeHWShader;

inline namespace Shader
{
	using white::int32;
	using white::uint16;

	class BuiltInShaderMeta;
	class ShaderMeta;
	class RenderShader;
	class ShaderParametersMetadata;

	/** Define a shader permutation uniquely according to its type, and permutation id.*/
	template<typename MetaShaderType>
	struct ShaderTypePermutation
	{
		MetaShaderType* const Type;
		const int32 PermutationId;

		ShaderTypePermutation(MetaShaderType* InType, int32 InPermutationId)
			: Type(InType)
			, PermutationId(InPermutationId)
		{
		}

		bool operator==(const ShaderTypePermutation& Other)const
		{
			return Type == Other.Type && PermutationId == Other.PermutationId;
		}

		bool operator!=(const ShaderTypePermutation& Other)const
		{
			return !(*this == Other);
		}
	};

	using ShaderPermutation = ShaderTypePermutation<ShaderMeta>;

	//shader parameter bindings and their offset and size in the shader's parameters struct.
	class RenderShaderParameterBindings
	{
	public:
		struct Parameter
		{
			uint16 BufferIndex;
			uint16 BaseIndex;
			//shader's parameters struct
			uint16 ByteOffset;
			uint16 ByteSize;
		};

		struct ResourceParameter
		{
			uint16 BaseIndex;
			//shader's parameters struct
			uint16 ByteOffset;
		};

		std::vector<Parameter> Parameters;
		//TODO:RaytracingAccelerationStructure
		std::vector<ResourceParameter> Textures;
		std::vector<ResourceParameter> Samplers;
		std::vector<ResourceParameter> UAVs;

		static constexpr uint16 kInvalidBufferIndex = 0xFFFF;

		uint16 RootParameterBufferIndex = kInvalidBufferIndex;



		void BindForLegacyShaderParameters(const RenderShader* Shader, const ShaderParameterMap& ParameterMaps, const ShaderParametersMetadata& StructMetaData);
		void BindForRootShaderParameters(const RenderShader* Shader, const ShaderParameterMap& ParameterMaps);
	};

	struct FShaderPermutationParameters
	{
		/** Unique permutation identifier of the material shader type. */
		const int32 PermutationId;

		explicit FShaderPermutationParameters(int32 InPermutationId = 0)
			:PermutationId(InPermutationId)
		{}
	};

	class ShaderMapResource
	{
	public:
		ShaderMapResource(std::size_t Num);

		int32 GetNumShaders() const
		{
			return NumHWShaders;
		}

		inline bool IsValidShaderIndex(int32 ShaderIndex) const
		{
			return ShaderIndex >= 0 && ShaderIndex < NumHWShaders;
		}

		inline bool HasShader(int32 ShaderIndex) const
		{
			return HWShaders[ShaderIndex].load(std::memory_order_acquire) != nullptr;
		}

		inline HardwareShader* GetShader(int32 ShaderIndex)
		{
			// This is a double checked locking. This trickery arises from the fact that we're
			// synchronizing two threads: one that takes a lock and another that doesn't.
			// Without fences, there is a race between storing the shader pointer and accessing it
			// on the other (lockless) thread.

			auto Shader = HWShaders[ShaderIndex].load(std::memory_order_acquire);
			if (Shader == nullptr)
			{
				// most shadermaps have <100 shaders, and less than a half of them can be created. One lock
				// for all creation seems sufficient, but if this function is often contended, per-shader
				// locks are easily possible.
				std::unique_lock lock{ ShadersCriticalSection };

				Shader = HWShaders[ShaderIndex].load(std::memory_order_relaxed);
				if (Shader == nullptr)
				{
					Shader = CreateShader(ShaderIndex);
					HWShaders[ShaderIndex].store(Shader, std::memory_order_release);
				}
			}
			return Shader;
		}

	protected:
		/** Addrefs the reference, passing the responsibility to the caller to Release() it. */
		HardwareShader* CreateShader(int32 ShaderIndex);

		virtual HardwareShader* CreateHWShader(int32 ShaderIndex) = 0;
	private:
		std::mutex ShadersCriticalSection;

		/** An array of shader pointers . */
		std::unique_ptr<std::atomic<HardwareShader*>[]> HWShaders;

		/** Since the shaders are no longer a vector, this is their count (the size of the HWShaders). */
		int32 NumHWShaders;
	};

	class ShaderMapResourceCode
	{
	public:
		struct ShaderEntry
		{
			std::vector<uint8> Code;
			int32 UnCompressSize;
			ShaderType Type;

			//Archive
		};

		void AddShaderCompilerOutput(const ShaderCompilerOutput& Output);

		int32 FindShaderIndex(const Digest::SHAHash& InHash) const;

		void AddShaderCode(ShaderType InType, const Digest::SHAHash& InHash, const ShaderCode& InCode);

		std::vector<Digest::SHAHash> ShaderHashes;
		std::vector<ShaderEntry> ShaderEntries;
	};

	class ShaderMapResource_InlineCode : public ShaderMapResource
	{
	public:
		ShaderMapResource_InlineCode(ShaderMapResourceCode* InCode)
			: ShaderMapResource(InCode->ShaderEntries.size())
			, Code(InCode)
		{}

		virtual HardwareShader* CreateHWShader(int32 ShaderIndex) override;

		std::unique_ptr<ShaderMapResourceCode> Code;
	};
	
	class RenderShader
	{
	public:
		using FPermutationDomain = FShaderPermutationNone;
		using FPermutationParameters = FShaderPermutationParameters;
		static constexpr bool RootParameterStruct = false;

		struct CompiledShaderInitializer
		{
			ShaderMeta* Meta;
			const std::vector<uint8>& Code;
			const ShaderParameterMap& ParameterMap;
			const Digest::SHAHash& OutputHash;

			CompiledShaderInitializer(ShaderMeta* InMeta,
				const ShaderCompilerOutput& CompilerOutput
			);
		};

		RenderShader();

		virtual ~RenderShader();

		RenderShader(const CompiledShaderInitializer& initializer);

		void Finalize(const ShaderMapResourceCode* Code);

		inline ShaderMeta* GetMeta() const { return Meta; }
		ShaderType GetShaderType() const;

		int32 GetResourceIndex() const { return ResourceIndex; }

		/** Returns the meta data for the root shader parameter struct. */
		static inline const ShaderParametersMetadata* GetRootParametersMetadata()
		{
			return nullptr;
		}

		/** Can be overridden by FShader subclasses to modify their compile environment just before compilation occurs. */
		static void ModifyCompilationEnvironment(const FShaderPermutationParameters&, FShaderCompilerEnvironment&) {}
		/** Can be overridden by FShader subclasses to determine whether a specific permutation should be compiled. */
		static bool ShouldCompilePermutation(const FShaderPermutationParameters&) { return true; }
	public:
		RenderShaderParameterBindings Bindings;
		ShaderMeta* Meta;
		int32 ResourceIndex;

#if WE_EDITOR
		/**
		* Hash of the compiled output from this shader and the resulting parameter map.
		* This is used to find a matching resource.
		*/
		Digest::SHAHash OutputHash;
#endif

	};

	class BuiltInShaderMeta;

	class ShaderMeta
	{
	public:
		enum class EShaderMetaForDownCast :uint32
		{
			Reserve,
			BuitlIn,
		};

		typedef class RenderShader* (*ConstructType)();
		typedef void (*ModifyCompilationEnvironmentType)(const FShaderPermutationParameters&, FShaderCompilerEnvironment&);
		typedef bool (*ShouldCompilePermutationType)(const FShaderPermutationParameters&);


		static std::list<ShaderMeta*>& GetTypeList();
	public:
		ShaderMeta(
			EShaderMetaForDownCast InShaderMetaForDownCast,
			const char* InName,
			const char* InSourceFileName,
			const char* InEntryPoint,
			platform::Render::ShaderType  InFrequency,
			int32 TotalPermutationCount,
			ConstructType InConstructRef,//InConstructCompiledRef
			ModifyCompilationEnvironmentType InModifyCompilationEnvironmentRef,
			ShouldCompilePermutationType InShouldCompilePermutationRef,
			uint32 InTypeSize,
			const ShaderParametersMetadata* InRootParametersMetadata
		);

		const std::string& GetTypeName() const { return TypeName; }
		std::size_t GetHash() const { return Hash; }
		const std::string& GetSourceFileName() const { return SourceFileName; }
		std::size_t GetHashedShaderFilename() const { return HashedShaderFilename; }
		const std::string& GetEntryPoint() const { return EntryPoint; }
		platform::Render::ShaderType GetShaderType() const { return Frequency; }

		inline int32 GetPermutationCount() const
		{
			return TotalPermutationCount;
		}

		RenderShader* Construct() const;

		bool ShouldCompilePermutation(const FShaderPermutationParameters& Parameters) const;
		void ModifyCompilationEnvironment(const FShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment) const;


		BuiltInShaderMeta* GetBuiltInShaderType()
		{
			return ShaderMetaForDownCast == EShaderMetaForDownCast::BuitlIn ? (BuiltInShaderMeta*)(this) : nullptr;
		}

		/** Returns the meta data for the root shader parameter struct. */
		inline const ShaderParametersMetadata* GetRootParametersMetadata() const
		{
			return RootParametersMetadata;
		}
	private:
		EShaderMetaForDownCast ShaderMetaForDownCast;

		std::string TypeName;
		std::size_t Hash;
		std::string SourceFileName;
		std::size_t HashedShaderFilename;
		std::string EntryPoint;
		platform::Render::ShaderType Frequency;
		int32 TotalPermutationCount;

		ConstructType ConstructRef;
		ModifyCompilationEnvironmentType ModifyCompilationEnvironmentRef;
		ShouldCompilePermutationType ShouldCompilePermutationRef;

		const ShaderParametersMetadata* const RootParametersMetadata;
	};

	template <typename ParameterStruct>
	inline void BindForLegacyShaderParameters(RenderShader* Shader, const ShaderParameterMap& ParameterMap)
	{
		Shader->Bindings.BindForLegacyShaderParameters(Shader, ParameterMap, *ParameterStruct::TypeInfo::GetStructMetadata());
	}

	template<>
	inline void BindForLegacyShaderParameters<void>(RenderShader* Shader, const ShaderParameterMap& ParameterMap)
	{
	}

	inline void BindForRootShaderParameters(RenderShader* Shader, const ShaderParameterMap& ParameterMap)
	{
		Shader->Bindings.BindForRootShaderParameters(Shader, ParameterMap);
	}

	template<class>
	struct ShaderParametersType
	{
		static constexpr bool HasParameters = false;
		using type = void;
	};

	template<class ShaderClass>
		requires requires{typename ShaderClass::Parameters; }
	struct ShaderParametersType<ShaderClass>
	{
		static constexpr bool HasParameters = true;

		using type = typename ShaderClass::Parameters;
	};

	template<class ShaderClass>
	using ShaderParametersType_t = typename ShaderParametersType<ShaderClass>::type;

	template <typename ShaderClass>
	requires requires{typename ShaderClass::Parameters; }
	inline const ShaderParametersMetadata* ParametersMetadata()
	{
		return ShaderClass::Parameters::TypeInfo::GetStructMetadata();
	}

	template<typename ShaderClass>
	inline const ShaderParametersMetadata* ParametersMetadata()
	{
		return nullptr;
	}


	class ShaderMapBase
	{
	public:
		virtual ~ShaderMapBase();

		ShaderMapResourceCode* GetResourceCode()
		{
			if (!Code)
			{
				Code = std::make_shared<ShaderMapResourceCode>();
			}
			return Code.get();
		}

		inline ShaderMapResource* GetResource() const { return Resource.get(); }

		void InitResource();

		void FinalizeContent();
	private:
		std::shared_ptr<ShaderMapResource> Resource;
		std::shared_ptr<ShaderMapResourceCode> Code;
	};

	template<typename ShaderContentType>
	class ShaderRefBase
	{
	public:
		ShaderRefBase():ShaderContent(nullptr)
		{}

		ShaderRefBase(ShaderContentType* InShader,const ShaderMapBase& InShaderMap) :ShaderContent(InShader),ShaderMap(&InShaderMap)
		{}

		template<std::derived_from<ShaderContentType> OtherShaderType>
		ShaderRefBase(const ShaderRefBase<OtherShaderType>& Rhs) 
		: ShaderContent(Rhs.GetShader()),ShaderMap(Rhs.GetShaderMap())
		{
		}

		inline ShaderContentType* operator->() const { return ShaderContent; }

		inline ShaderContentType* GetShader() const { return ShaderContent; }
		inline const ShaderMapBase* GetShaderMap() const { return ShaderMap; }
		ShaderMapResource* GetResource() const
		{
			return ShaderMap->GetResource();
		}

		inline VertexHWShader* GetVertexShader() const
		{
			return GetShaderBase<VertexHWShader>(VertexShader);
		}
		inline GeometryHWShader* GetGeometryShader() const
		{
			return GetShaderBase<GeometryHWShader>(GeometryShader);
		}
		inline PixelHWShader*GetPixelShader() const
		{
			return GetShaderBase<PixelHWShader>(PixelShader);
		}

		inline ComputeHWShader* GetComputeShader() const
		{
			return GetShaderBase<ComputeHWShader>(ComputeShader);
		}

		template<typename TargetShader>
		inline TargetShader* GetShaderBase(ShaderType Type) const
		{
			HardwareShader* HWShader = nullptr;
			if (ShaderContent)
			{
				WAssert(ShaderContent->GetShaderType() == Type, "mismatch shader type");
				HWShader = GetResource()->GetShader(ShaderContent->GetResourceIndex());
			}
			return static_cast<TargetShader*>(HWShader);
		}

		template<typename OtherShaderType>
		static ShaderRefBase<ShaderContentType> Cast(const ShaderRefBase<OtherShaderType>& Rhs)
		{
			return ShaderRefBase<ShaderContentType>(static_cast<ShaderContentType*>(Rhs.GetShader()),Rhs.GetShaderMap());
		}

		inline bool IsValid() const { return ShaderContent != nullptr; }

		inline explicit operator bool() const { return IsValid(); }
	protected:
		ShaderRefBase(ShaderContentType* InShader, const ShaderMapBase* InShaderMap) :ShaderContent(InShader), ShaderMap(InShaderMap)
		{}

		ShaderContentType* ShaderContent;
		const ShaderMapBase* ShaderMap;
	};

	template<typename ShaderContentType>
	using ShaderRef = ShaderRefBase< ShaderContentType>;

	

#define EXPORTED_SHADER_VTABLE(ShaderClass) \
	static RenderShader* ConstructInstance() { return new ShaderClass();} \
	static RenderShader* ConstructCompiledInstance(const ShaderMetaType::CompiledShaderInitializer& Initializer) { return new ShaderClass(Initializer);} \
	static constexpr bool HasParameters =  platform::Render::ShaderParametersType<ShaderClass>::HasParameters;\
	static void ModifyCompilationEnvironmentImpl(const platform::Render::FShaderPermutationParameters& Parameters, platform::Render::FShaderCompilerEnvironment& OutEnvironment) \
	{ \
		const typename ShaderClass::FPermutationDomain PermutationVector(Parameters.PermutationId); \
		PermutationVector.ModifyCompilationEnvironment(OutEnvironment); \
		ShaderClass::ModifyCompilationEnvironment(static_cast<const typename ShaderClass::FPermutationParameters&>(Parameters), OutEnvironment); \
	} \
	static bool ShouldCompilePermutationImpl(const platform::Render::FShaderPermutationParameters& Parameters) \
	{ return ShaderClass::ShouldCompilePermutation(static_cast<const typename ShaderClass::FPermutationParameters&>(Parameters)); }

#define EXPORTED_SHADER_TYPE(ShaderClass,ShaderMetaTypeShortcut) \
public:\
	using ShaderMetaType = platform::Render::##ShaderMetaTypeShortcut##ShaderMeta;\
	using ShaderMapType = platform::Render::##ShaderMetaTypeShortcut##ShaderMap;\
	static ShaderMetaType StaticType; \
	EXPORTED_SHADER_VTABLE(ShaderClass)\
	ShaderClass(const ShaderMetaType::CompiledShaderInitializer& Initializer) \
		:DerivedType(Initializer)\
	{\
		if constexpr(ShaderClass::RootParameterStruct)\
			platform::Render::BindForRootShaderParameters(this, Initializer.ParameterMap); \
		else if constexpr(platform::Render::ShaderParametersType<ShaderClass>::HasParameters)\
			platform::Render::BindForLegacyShaderParameters<platform::Render::ShaderParametersType_t<ShaderClass>>(this,Initializer.ParameterMap);\
	}\
	ShaderClass() \
	{ }

#define SHADER_VTABLE(ShaderClass) \
	ShaderClass::FPermutationDomain::PermutationCount,\
	ShaderClass::ConstructInstance, \
	ShaderClass::ModifyCompilationEnvironmentImpl,\
	ShaderClass::ShouldCompilePermutationImpl

#define IMPLEMENT_SHADER(ShaderClass,SourceFileName,FunctionName,Frequency) \
	ShaderClass::ShaderMetaType ShaderClass::StaticType( \
		ShaderClass::ShaderMetaType::EShaderMetaForDownCast::Reserve,\
		#ShaderClass, \
		SourceFileName, \
		FunctionName, \
		Frequency, \
		SHADER_VTABLE(ShaderClass),\
		sizeof(ShaderClass),\
		ShaderClass::GetRootParametersMetadata()\
	)

	/** A reference which is initialized with the requested shader type from a shader map. */
	template<typename ShaderType>
	class ShaderMapRef : public ShaderRef<ShaderType>
	{
	public:
		ShaderMapRef(const typename ShaderType::ShaderMapType* ShaderIndex)
			: ShaderRef<ShaderType>(ShaderIndex->template GetShader<ShaderType>(/* PermutationId = */ 0)) // gcc3 needs the template quantifier so it knows the < is not a less-than
		{
			static_assert(
				std::is_same_v<typename ShaderType::FPermutationDomain, FShaderPermutationNone>,
				"Missing permutation vector argument for shader that have a permutation domain.");
		}

		ShaderMapRef(
			const typename ShaderType::ShaderMapType* ShaderIndex,
			const typename ShaderType::FPermutationDomain& PermutationVector)
			: ShaderRef<ShaderType>(ShaderIndex->template GetShader<ShaderType>(PermutationVector.ToDimensionValueId())) // gcc3 needs the template quantifier so it knows the < is not a less-than
		{ }
	};

	class ShaderMapContent
	{
	public:
		~ShaderMapContent()
		{
			Clear();
		}

		/** Finds the shader with the given type.  Asserts on failure. */
		template<typename ShaderType>
		ShaderType* GetShader(int32 PermutationId = 0) const
		{
			auto Shader = GetShader(&ShaderType::StaticType, PermutationId);
			WAssert(Shader != nullptr, white::sfmt("Failed to find shader type %s in Platform %s", ShaderType::StaticType.GetName(),"PCD3D_SM5").c_str());
			return static_cast<ShaderType*>(Shader);
		}

		/** Finds the shader with the given type.  Asserts on failure. */
		template<typename ShaderType>
		ShaderType* GetShader(const typename ShaderType::FPermutationDomain& PermutationVector) const
		{
			return GetShader<ShaderType>(PermutationVector.ToDimensionValueId());
		}

		/** Finds the shader with the given type.  May return NULL. */
		RenderShader* GetShader(ShaderMeta* ShaderType, int32 PermutationId = 0) const
		{
			return GetShader(ShaderType->GetHash(), PermutationId);
		}

		/** Finds the shader with the given type name.  May return NULL. */
		RenderShader* GetShader(size_t TypeNameHash, int32 PermutationId = 0) const;

		/** Finds the shader with the given type. */
		bool HasShader(size_t TypeNameHash, int32 PermutationId) const
		{
			auto Shader = GetShader(TypeNameHash, PermutationId);
			return Shader != nullptr;
		}

		bool HasShader(const ShaderMeta* Type, int32 PermutationId) const
		{
			return HasShader(Type->GetHash(), PermutationId);
		}

		RenderShader* FindOrAddShader(size_t TypeNameHash, int32 PermutationId, RenderShader* Shader);

		/** clears out all shaders and deletes shader pipelines held in the map */
		void Clear();

		/** @return true if the map is empty */
		inline bool IsEmpty() const
		{
			return Shaders.empty();
		}

		/** @return The number of shaders in the map. */
		uint32 GetNumShaders() const;

		void Finalize(const ShaderMapResourceCode* Code);
	private:
		std::vector<RenderShader*> Shaders;
		std::vector<size_t> ShaderTypes;
		std::vector<int32> ShaderPermutations;
		std::unordered_multimap<white::uint16,white::uint32> ShaderHash;
		std::shared_mutex ShaderMutex;
	};

	void GenerateOuput(ShaderInitializer initializer, ShaderCompilerOutput& Output);

	void CompileShaderMap();

}

PR_NAMESPACE_END

#undef PR_NAMESPACE_BEGIN
#undef PR_NAMESPACE_END
