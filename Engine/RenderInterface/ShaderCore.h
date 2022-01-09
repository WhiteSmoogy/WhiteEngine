#pragma once
#ifndef WE_RENDER_ShaderCore_h
#define WE_RENDER_ShaderCore_h 1

#include <WBase/winttype.hpp>
#include <WBase/wmathtype.hpp>
#include <WBase/cformat.h>
#include <WFramework/WCLib/Platform.h>
#include <optional>
#include <WBase/span.hpp>
#include "Runtime/Core/Hash/MessageDigest.h"

#if WFL_Win32
#define D3D_RAYTRACING 1
#endif

namespace white::coroutine
{
	template<class T>
	class Task;
}

namespace WhiteEngine
{
	class AsyncArchive;
}

namespace platform::Render {

	class Texture;

	using white::coroutine::Task;

	inline namespace Shader
	{
		using white::uint16;
		using white::uint32;
		using white::int32;
		using white::uint8;

		struct ShaderCodeResourceCounts
		{
			// for FindOptionalData() and AddOptionalData()
			static const uint8 Key = 'p';

			uint16 NumSamplers = 0;
			uint16 NumSRVs = 0;
			uint16 NumUAVs = 0;
			uint16 NumCBs = 0;

			Task<void> Serialize(WhiteEngine::AsyncArchive& v);
		};

		enum ShaderType : white::uint8
		{
			VertexShader,
			PixelShader,
			HullShader,
			DomainShader,
			GeometryShader,
			VisibilityAll = 5,
			ComputeShader = 5,

			NumStandardType = 6,

			RayGen = 6,
			RayMiss,
			RayHitGroup,
			RayCallable
		};


		using ShaderBlob = std::pair<std::unique_ptr<stdex::byte[]>, std::size_t>;

		enum ShaderBaseType
		{
			SBT_INVALID,

			SBT_SRV,
			SBT_UAV,
			SBT_SAMPLER,
			SBT_CBUFFER,

			SBT_BOOL,

			SBT_INT32,
			SBT_UINT32,
			SBT_FLOAT32,

			SBT_ELEMTYPE = 1<<15,
		};

		template<uint8 row, uint8 col>
		constexpr uint8 MakeFormatRowCol = (row << 4) | col;

		template<uint16 ch0,uint8 row,uint8 col,uint8 index>
		constexpr uint32 MakeShaderFormat = (ch0 << 16) | (MakeFormatRowCol<row, col> << 8) | index;

		enum ShaderParamType :uint32
		{
			SPT_texture1D = MakeShaderFormat<SBT_SRV| SBT_ELEMTYPE,1,1,0>,
			SPT_texture2D,
			SPT_texture3D,
			SPT_textureCUBE,
			SPT_texture1DArray,
			SPT_texture2DArray,
			SPT_texture3DArray,
			SPT_textureCUBEArray,
			SPT_ConstantBuffer = MakeShaderFormat<SBT_CBUFFER| SBT_ELEMTYPE, 1, 1, 0>,
			SPT_buffer = MakeShaderFormat<SBT_SRV| SBT_ELEMTYPE, 1, 1, 8>,
			SPT_StructuredBuffer,
			SPT_rwbuffer = MakeShaderFormat<SBT_UAV| SBT_ELEMTYPE, 1, 1, 0>,
			SPT_rwstructured_buffer,
			SPT_rwtexture1D,
			SPT_rwtexture2D,
			SPT_rwtexture3D,
			SPT_rwtexture1DArray,
			SPT_rwtexture2DArray,
			SPT_AppendStructuredBuffer,
			SPT_ConsumeStructuredBuffer = MakeShaderFormat<SBT_SRV| SBT_ELEMTYPE, 1, 1, 10>,
			SPT_byteAddressBuffer = MakeShaderFormat<SBT_SRV , 1, 1, 10>,
			SPT_rwbyteAddressBuffer = MakeShaderFormat<SBT_UAV, 1, 1, 8>,
			SPT_RaytracingAccelerationStructure = MakeShaderFormat<SBT_SRV, 1, 1, 12>,
			SPT_sampler = MakeShaderFormat<SBT_SAMPLER, 1, 1, 0>,
			SPT_shader = MakeShaderFormat<SBT_INVALID, 1, 1, 0>,
			SPT_bool = MakeShaderFormat<SBT_BOOL, 1, 1, 0>,
			SPT_string = MakeShaderFormat<SBT_INVALID, 1, 1, 1>,
			SPT_uint  = MakeShaderFormat<SBT_UINT32, 1, 1, 0>,
			SPT_uint2 = MakeShaderFormat<SBT_UINT32, 1, 2, 0>,
			SPT_uint3 = MakeShaderFormat<SBT_UINT32, 1, 3, 0>,
			SPT_uint4 = MakeShaderFormat<SBT_UINT32, 1, 4, 0>,
			SPT_int   = MakeShaderFormat<SBT_INT32, 1, 1, 0>,
			SPT_int2  = MakeShaderFormat<SBT_INT32, 1, 2, 0>,
			SPT_int3  = MakeShaderFormat<SBT_INT32, 1, 3, 0>,
			SPT_int4  = MakeShaderFormat<SBT_INT32, 1, 4, 0>,
			SPT_float = MakeShaderFormat<SBT_FLOAT32, 1, 1, 0>,
			SPT_float2 = MakeShaderFormat<SBT_FLOAT32, 1, 2, 0>,
			SPT_float2x2 = MakeShaderFormat<SBT_FLOAT32, 2, 2, 0>,
			SPT_float2x3 = MakeShaderFormat<SBT_FLOAT32, 2, 3, 0>,
			SPT_float2x4 = MakeShaderFormat<SBT_FLOAT32, 2, 4, 0>,
			SPT_float3 = MakeShaderFormat<SBT_FLOAT32, 1, 3, 0>,
			SPT_float3x2 = MakeShaderFormat<SBT_FLOAT32, 3, 2, 0>,
			SPT_float3x3 = MakeShaderFormat<SBT_FLOAT32, 3, 3, 0>,
			SPT_float3x4 = MakeShaderFormat<SBT_FLOAT32, 3, 4, 0>,
			SPT_float4 = MakeShaderFormat<SBT_FLOAT32, 1, 4, 0>,
			SPT_float4x2 = MakeShaderFormat<SBT_FLOAT32, 4, 2, 0>,
			SPT_float4x3 = MakeShaderFormat<SBT_FLOAT32, 4, 3, 0>,
			SPT_float4x4 = MakeShaderFormat<SBT_FLOAT32, 4, 4, 0>,

			SPT_ElemEmpty = MakeShaderFormat<SBT_INVALID, 1, 1, 2>,
			SPT_StructInclude,
		};

		constexpr ShaderBaseType GetBaseType(ShaderParamType type)
		{
			return static_cast<ShaderBaseType>((type >> 16) & ~SBT_ELEMTYPE);
		}

		constexpr bool IsElemType(ShaderParamType type)
		{
			return static_cast<ShaderBaseType>(type >> 16) & SBT_ELEMTYPE;
		}

		constexpr bool IsNumberType(ShaderParamType type)
		{
			auto base_type = GetBaseType(type);

			return base_type == SBT_INT32 || base_type == SBT_UINT32 || base_type == SBT_FLOAT32;
		}

		constexpr bool IsTextureReadType(ShaderParamType type)
		{
			auto base_type = GetBaseType(type);

			return base_type == SBT_SRV && type <= SPT_textureCUBEArray;
		}

		constexpr uint8 GetNumRows(ShaderParamType type)
		{
			return ((type >> 8) & 0XF0) >> 4;
		}

		constexpr uint8 GetNumColumns(ShaderParamType type)
		{
			return ((type >> 8) & 0X0F);
		}

		struct RayTracingShaderInfo
		{
			std::string EntryPoint;
			std::string AnyHitEntryPoint;
			std::string IntersectionEntryPoint;


			Task<void> Serialize(WhiteEngine::AsyncArchive& v);
		};

		enum class ShaderParamClass :uint8_t
		{
			LooseData,
			UniformBuffer,
			Sampler,
			SRV,
			UAV,

			Num
		};

		struct ShaderInfo {
			ShaderType Type;

			ShaderInfo(ShaderType t);

			struct ConstantBufferInfo
			{
				struct VariableInfo
				{
					std::string name;
					uint32_t start_offset;
					uint8_t type;
					uint8_t rows;
					uint8_t columns;
					uint16_t elements;
					uint16_t size;

					Task<void> Serialize(WhiteEngine::AsyncArchive& v);
					
				};
				std::vector<VariableInfo> var_desc;

				std::string name;
				size_t name_hash;
				uint16_t size;
				uint32_t bind_point;

				Task<void> Serialize(WhiteEngine::AsyncArchive& v);
				
			};
			std::vector<ConstantBufferInfo> ConstantBufferInfos;

			struct BoundResourceInfo
			{
				std::string name;
				uint8_t type;
				uint8_t dimension;
				uint16_t bind_point;

				Task<void> Serialize(WhiteEngine::AsyncArchive& v);
				
			};
			std::vector<BoundResourceInfo> BoundResourceInfos;

			ShaderCodeResourceCounts ResourceCounts;

			std::optional<size_t> InputSignature = std::nullopt;
			std::optional<white::math::vector3<white::uint16>> CSBlockSize = std::nullopt;

			std::optional<RayTracingShaderInfo> RayTracingInfos = std::nullopt;

			Task<void> Serialize(WhiteEngine::AsyncArchive& v);
			
		};

		using ShaderMacro = std::pair<std::string, std::string>;
		struct ParameterAllocation
		{
			uint16 BufferIndex;
			uint16 BaseIndex;
			uint16 Size;
			ShaderParamClass Class = ShaderParamClass::Num;
		};

		class ShaderParameterMap
		{
		public:
			void AddParameterAllocation(const std::string& ParameterName, uint16 BufferIndex, uint16 BaseIndex, uint16 Size, ShaderParamClass ParameterType);

			bool FindParameterAllocation(const std::string& ParameterName, uint16& OutBufferIndex, uint16& OutBaseIndex, uint16& OutSize) const;

			void UpdateHash(Digest::SHA1& HashState) const;
		private:
			std::unordered_map<std::string, ParameterAllocation> ParameterMap;
		};

		struct FShaderCompilerEnvironment
		{
			void SetDefine(const char* Name, const char* Value)
			{
				Definitions.emplace(Name, Value);
			}
			void SetDefine(const char* Name, bool Value)
			{
				SetDefine(Name, Value ? 1 : 0);
			}
			void SetDefine(const char* Name, int32 Value)
			{
				SetDefine(Name, white::sfmt("%d", Value).c_str());
			}

			const std::unordered_map<std::string, std::string>& GetDefinitions() const
			{
				return Definitions;
			}
		private:
			std::unordered_map<std::string, std::string> Definitions;
		};

		const std::string& GetShaderCompressionFormat();

		#ifndef WB_ATTRIBUTE_UNALIGNED
		// TODO find out if using GCC_ALIGN(1) instead of this new #define break on all kinds of platforms...
		#define WB_ATTRIBUTE_UNALIGNED
		#endif

		typedef int32  WB_ATTRIBUTE_UNALIGNED unaligned_int32;
		typedef uint32 WB_ATTRIBUTE_UNALIGNED unaligned_uint32;

		class ShaderCodeReader
		{
			white::span<const uint8> ShaderCode;

		public:
			ShaderCodeReader(white::span<const uint8> InShaderCode)
				: ShaderCode(InShaderCode)
			{
				wassume(!ShaderCode.empty());
			}

			uint32 GetActualShaderCodeSize() const
			{
				return static_cast<int32>(ShaderCode.size()) - GetOptionalDataSize();
			}

			// for convenience
			template <class T>
			const T* FindOptionalData() const
			{
				return (const T*)FindOptionalData(T::Key, sizeof(T));
			}


			// @param InKey e.g. FShaderCodePackedResourceCounts::Key
			// @return 0 if not found
			const uint8* FindOptionalData(uint8 InKey, uint8 ValueSize) const
			{
				wassume(ValueSize);

				const uint8* End = &ShaderCode[0] + ShaderCode.size();

				int32 LocalOptionalDataSize = GetOptionalDataSize();

				const uint8* Start = End - LocalOptionalDataSize;
				// while searching don't include the optional data size
				End = End - sizeof(LocalOptionalDataSize);
				const uint8* Current = Start;

				while (Current < End)
				{
					uint8 Key = *Current++;
					uint32 Size = *((const unaligned_uint32*)Current);
					Current += sizeof(Size);

					if (Key == InKey && Size == ValueSize)
					{
						return Current;
					}

					Current += Size;
				}

				return 0;
			}

			const char* FindOptionalData(uint8 InKey) const
			{
				wassume(ShaderCode.size() >= 4);

				const uint8* End = &ShaderCode[0] + ShaderCode.size();

				int32 LocalOptionalDataSize = GetOptionalDataSize();

				const uint8* Start = End - LocalOptionalDataSize;
				// while searching don't include the optional data size
				End = End - sizeof(LocalOptionalDataSize);
				const uint8* Current = Start;

				while (Current < End)
				{
					uint8 Key = *Current++;
					uint32 Size = *((const unaligned_uint32*)Current);
					Current += sizeof(Size);

					if (Key == InKey)
					{
						return (char*)Current;
					}

					Current += Size;
				}

				return 0;
			}

			// Returns nullptr and Size -1 if key was not found
			const uint8* FindOptionalDataAndSize(uint8 InKey, int32& OutSize) const
			{
				wassume(ShaderCode.size() >= 4);

				const uint8* End = &ShaderCode[0] + ShaderCode.size();

				int32 LocalOptionalDataSize = GetOptionalDataSize();

				const uint8* Start = End - LocalOptionalDataSize;
				// while searching don't include the optional data size
				End = End - sizeof(LocalOptionalDataSize);
				const uint8* Current = Start;

				while (Current < End)
				{
					uint8 Key = *Current++;
					uint32 Size = *((const unaligned_uint32*)Current);
					Current += sizeof(Size);

					if (Key == InKey)
					{
						OutSize = Size;
						return Current;
					}

					Current += Size;
				}

				OutSize = -1;
				return nullptr;
			}

			int32 GetOptionalDataSize() const
			{
				if (ShaderCode.size() < sizeof(int32))
				{
					return 0;
				}

				const uint8* End = &ShaderCode[0] + ShaderCode.size();

				int32 LocalOptionalDataSize = ((const unaligned_int32*)End)[-1];

				wassume(LocalOptionalDataSize >= 0);
				wassume(ShaderCode.size() >= LocalOptionalDataSize);

				return LocalOptionalDataSize;
			}

			int32 GetShaderCodeSize() const
			{
				return static_cast<int32>(ShaderCode.size()) - GetOptionalDataSize();
			}
		};

		class ShaderCode
		{
			// -1 if ShaderData was finalized
			mutable int32 OptionalDataSize;
			// access through class methods
			mutable std::vector<uint8> ShaderCodeWithOptionalData;

			/** ShaderCode may be compressed in SCWs on demand. If this value isn't null, the shader code is compressed. */
			mutable int32 UncompressedSize;

			/** Compression algo */
			mutable std::string CompressionFormat;

			/** We cannot get the code size after the compression, so store it here */
			mutable int32 ShaderCodeSize;

		public:

			ShaderCode()
				: OptionalDataSize(0)
				, UncompressedSize(0)
				, CompressionFormat()
				, ShaderCodeSize(0)
			{
			}

			// adds CustomData or does nothing if that was already done before
			void FinalizeShaderCode() const
			{
				if (OptionalDataSize != -1)
				{
					WAssert(UncompressedSize == 0, "ShaderCode::FinalizeShaderCode() was called after compressing the code");
					OptionalDataSize += sizeof(OptionalDataSize);
					ShaderCodeWithOptionalData.insert(ShaderCodeWithOptionalData.end(),(const uint8*)&OptionalDataSize, (const uint8*)&OptionalDataSize+ sizeof(OptionalDataSize));
					OptionalDataSize = -1;
				}
			}

			void Compress(const std::string& ShaderCompressionFormat);

			// Write access for regular microcode: Optional Data must be added AFTER regular microcode and BEFORE Finalize
			std::vector<uint8>& GetWriteAccess()
			{
				WAssert(OptionalDataSize != -1, "Tried to add ShaderCode after being finalized!");
				WAssert(OptionalDataSize == 0, "Tried to add ShaderCode after adding Optional data!");
				return ShaderCodeWithOptionalData;
			}

			int32 GetShaderCodeSize() const
			{
				// use the cached size whenever available
				if (ShaderCodeSize != 0)
				{
					return ShaderCodeSize;
				}
				else
				{
					FinalizeShaderCode();

					ShaderCodeReader Wrapper(ShaderCodeWithOptionalData);
					return Wrapper.GetShaderCodeSize();
				}
			}

			// for read access, can have additional data attached to the end. Can also be compressed
			const std::vector<uint8>& GetReadAccess() const
			{
				FinalizeShaderCode();

				return ShaderCodeWithOptionalData;
			}

			bool IsCompressed() const
			{
				return UncompressedSize != 0;
			}

			const std::string& GetCompressionFormat() const
			{
				return CompressionFormat;
			}

			int32 GetUncompressedSize() const
			{
				return UncompressedSize;
			}

			// for convenience
			template <class T>
			void AddOptionalData(const T& In)
			{
				AddOptionalData(T::Key, (uint8*)&In, sizeof(T));
			}

			// Note: we don't hash the optional attachments in GenerateOutputHash() as they would prevent sharing (e.g. many material share the save VS)
			// can be called after the non optional data was stored in ShaderData
			// @param Key uint8 to save memory so max 255, e.g. FShaderCodePackedResourceCounts::Key
			// @param Size >0, only restriction is that sum of all optional data values must be < 4GB
			void AddOptionalData(uint8 Key, const uint8* ValuePtr, uint32 ValueSize)
			{
				wconstraint(ValuePtr);

				// don't add after Finalize happened
				wconstraint(OptionalDataSize >= 0);

				ShaderCodeWithOptionalData.emplace_back(Key);
				ShaderCodeWithOptionalData.insert(ShaderCodeWithOptionalData.end(),(const uint8*)&ValueSize, (const uint8*)&ValueSize+sizeof(ValueSize));
				ShaderCodeWithOptionalData.insert(ShaderCodeWithOptionalData.end(),ValuePtr, ValuePtr+ValueSize);
				OptionalDataSize += sizeof(uint8) + sizeof(ValueSize) + (uint32)ValueSize;
			}

			// Note: we don't hash the optional attachments in GenerateOutputHash() as they would prevent sharing (e.g. many material share the save VS)
			// convenience, silently drops the data if string is too long
			// @param e.g. 'n' for the ShaderSourceFileName
			void AddOptionalData(uint8 Key, const char* InString)
			{
				uint32 Size =static_cast<uint32>(std::char_traits<char>::length(InString)) + 1;
				AddOptionalData(Key, (uint8*)InString, Size);
			}
		};

		struct ShaderCompilerOutput
		{
			ShaderType Type;
			ShaderCode ShaderCode;
			ShaderParameterMap ParameterMap;
			Digest::SHAHash OutputHash;

			void CompressOutput(const std::string& ShaderCompressionFormat);

			void GenerateOutputHash();
		};

		class ShaderInitializer
		{
		public:
			const ShaderBlob* pBlob;
			const ShaderInfo* pInfo;
		};

		void GenerateOuput(ShaderInitializer initializer, ShaderCompilerOutput& Output);

		const uint8* TryUncompressCode(const std::vector<uint8>& Code, int32 UnCompressSize, std::vector<uint8>& UnCompressCode);
	}
}

#endif