#pragma once

#include <WBase/wmathtype.hpp>
#include "ShaderParametersMetadata.h"
#include "RenderInterface/IGPUResourceView.h"
#include "RenderGraph/RenderGraphFwd.h"

namespace platform::Render
{
	inline namespace Shader
	{
		constexpr std::size_t VariableBoundary = 16;
	}

	namespace HLSLTraits
	{
		struct RWByteAddressBuffer {};
		struct ByteAddressBuffer {};
		struct RWTexture2D {};

		template<typename Struct>
		struct ConstantBuffer {};

		template<typename Struct>
		struct RWStructuredBuffer {};

		template<typename Struct>
		struct RWBuffer {};

		template<typename Struct>
		struct StructuredBuffer {};

		template<typename _type0, typename _type1>
		struct UnionPointer
		{
			union {
				_type0* v0;
				_type1* v1;
			};

			_type0*& operator=(_type0* value)
			{
				v0 = value;
				return v0;
			}

			_type1*& operator=(_type1* value)
			{
				v1 = value;
				return v1;
			}

			operator _type0* () const
			{
				return v0;
			}

			operator _type1* () const
			{
				return v0;
			}
		};
	}

#define MS_ALIGN(n) __declspec(align(n))
	inline namespace Shader
	{
		template<typename TypeParameter>
		struct TShaderParameterTypeInfo;

		template<ShaderParamType ShaderType, uint32 NumElements = 0>
		struct ShaderTypeInfo
		{
			static constexpr ShaderParamType ShaderType = ShaderType;

			static constexpr ShaderBaseType BaseType = GetBaseType(ShaderType);
			static constexpr uint32 NumRows = GetNumRows(ShaderType);
			static constexpr uint32 NumColumns = GetNumColumns(ShaderType);
			static constexpr uint32 NumElements = NumElements;

			static inline const platform::Render::ShaderParametersMetadata* GetStructMetadata() { return nullptr; }
		};

		template<>
		struct TShaderParameterTypeInfo<float> : ShaderTypeInfo<SPT_float>
		{
			template<std::size_t Boundary = 0>
			static constexpr std::size_t Alignement = 4;

			using DeclType = float;
		};

		template<>
		struct TShaderParameterTypeInfo<white::math::float2> : ShaderTypeInfo<SPT_float2>
		{
			template<std::size_t Boundary = 0>
			static constexpr std::size_t Alignement = Boundary >= 8 ? 4 : 16;

			using DeclType = white::math::float2;
		};

		template<>
		struct TShaderParameterTypeInfo<white::math::float3> : ShaderTypeInfo<SPT_float3>
		{
			template<std::size_t Boundary = 0>
			static constexpr std::size_t Alignement = Boundary >= 12 ? 4 : 16;

			using DeclType = white::math::float3;
		};

		template<>
		struct TShaderParameterTypeInfo<white::math::float4> : ShaderTypeInfo<SPT_float4>
		{
			template<std::size_t Boundary = 0>
			static constexpr std::size_t Alignement = 16;

			using DeclType = white::math::float4;
		};

		template<>
		struct TShaderParameterTypeInfo<white::math::float4x4> : ShaderTypeInfo<SPT_float4x4>
		{
			template<std::size_t Boundary = 0>
			static constexpr std::size_t Alignement = 16;

			using DeclType = white::math::float4x4;
		};

		template<>
		struct TShaderParameterTypeInfo<int> : ShaderTypeInfo<SPT_int>
		{
			template<std::size_t Boundary = 0>
			static constexpr std::size_t Alignement = 4;

			using DeclType = white::int32;
		};

		template<>
		struct TShaderParameterTypeInfo<unsigned int> : ShaderTypeInfo<SPT_uint>
		{
			template<std::size_t Boundary = 0>
			static constexpr std::size_t Alignement = 4;

			using DeclType = white::uint32;
		};

		template<>
		struct TShaderParameterTypeInfo<white::math::uint2> : ShaderTypeInfo<SPT_uint2>
		{
			template<std::size_t Boundary = 0>
			static constexpr std::size_t Alignement = Boundary >= 8 ? 4 : 16;

			using DeclType = white::math::uint2;
		};

		template<ShaderParamType ShaderType>
		struct TShaderParameterUAVType : ShaderTypeInfo<ShaderType>
		{
			using DeclType = HLSLTraits::UnionPointer<platform::Render::UnorderedAccessView, RenderGraph::RGUnorderedAccessView>;

			template<std::size_t Boundary = 0>
			static constexpr std::size_t Alignement = sizeof(DeclType);
		};

		template<ShaderParamType ShaderType>
		struct TShaderParameterSRVType : ShaderTypeInfo<ShaderType>
		{
			using DeclType = HLSLTraits::UnionPointer<platform::Render::ShaderResourceView, RenderGraph::RGShaderResourceView>;

			template<std::size_t Boundary = 0>
			static constexpr std::size_t Alignement = sizeof(DeclType);
		};

		template<>
		struct TShaderParameterTypeInfo<HLSLTraits::RWByteAddressBuffer> : TShaderParameterUAVType<SPT_rwbyteAddressBuffer>
		{
		};

		template<>
		struct TShaderParameterTypeInfo<HLSLTraits::ByteAddressBuffer> : TShaderParameterSRVType<SPT_byteAddressBuffer>
		{
		};

		template<>
		struct TShaderParameterTypeInfo<HLSLTraits::RWTexture2D> : TShaderParameterUAVType<SPT_rwtexture2D>
		{
		};

		template<typename Struct>
		struct TShaderParameterTypeInfo<HLSLTraits::ConstantBuffer<Struct>> : ShaderTypeInfo<SPT_ConstantBuffer>
		{
			using DeclType = HLSLTraits::UnionPointer<platform::Render::ConstantBuffer, RenderGraph::RGConstBuffer>;

			template<std::size_t Boundary = 0>
			static constexpr std::size_t Alignement = sizeof(DeclType);
		};

		template<typename Struct>
		struct TShaderParameterTypeInfo<HLSLTraits::RWStructuredBuffer<Struct>> : TShaderParameterUAVType<SPT_rwstructured_buffer>
		{
		};

		template<typename Struct>
		struct TShaderParameterTypeInfo<HLSLTraits::RWBuffer<Struct>> : TShaderParameterUAVType<SPT_rwbuffer>
		{
		};

		template<typename Struct>
		struct TShaderParameterTypeInfo<HLSLTraits::StructuredBuffer<Struct>> : TShaderParameterSRVType<SPT_StructuredBuffer>
		{
		};

		template<typename TypeParameter>
		struct TShaderTextureTypeInfo;
	}
}


#define INTERNAL_LOCAL_SHADER_PARAMETER_GET_STRUCT_METADATA(StructTypeName) \
	static platform::Render::ShaderParametersMetadata StaticStructMetadata(sizeof(StructTypeName),\
		StructTypeName::zzGetMembers()); \
	return &StaticStructMetadata;

#define INTERNAL_SHADER_PARAMETER_STRUCT_BEGIN(StructTypeName,GetStructMetadataScope) \
	MS_ALIGN(platform::Render::VariableBoundary) class StructTypeName \
	{ \
	public: \
		struct TypeInfo{\
			static constexpr platform::Render::ShaderBaseType BaseType = platform::Render::SBT_INVALID;\
			static constexpr white::uint32 NumRows = 1;\
			static constexpr white::uint32 NumColumns =1;\
			static constexpr white::uint32 NumElements = 0;\
			template<std::size_t Boundary = 0>\
			static constexpr std::size_t Alignement = 16;\
			using DeclType = StructTypeName;\
			static inline const platform::Render::ShaderParametersMetadata* GetStructMetadata() { GetStructMetadataScope } \
		};\
	private: \
		typedef StructTypeName zzThisStruct; \
		struct zzFirstMemberId { enum { Boundary = 16 }; }; \
		typedef void* zzFuncPtr; \
		typedef zzFuncPtr(*zzMemberFunc)(zzFirstMemberId, std::vector<platform::Render::ShaderParametersMetadata::Member>*); \
		static zzFuncPtr zzAppendMemberGetPrev(zzFirstMemberId, std::vector<platform::Render::ShaderParametersMetadata::Member>*) \
		{ \
			return nullptr; \
		} \
		typedef zzFirstMemberId

#define INTERNAL_SHADER_PARAMETER_EXPLICIT(ShaderType,TypeInfo,MemberType,MemberName) \
	zzMemberId##MemberName; \
	public: \
		alignas(TypeInfo::Alignement<zzMemberId##MemberName::Boundary>) TypeInfo::DeclType MemberName {}; \
	private: \
		struct zzNextMemberId##MemberName \
			{enum { Boundary =zzMemberId##MemberName::Boundary <sizeof(MemberName) ? \
				platform::Render::VariableBoundary:\
				zzMemberId##MemberName::Boundary-sizeof(MemberName) }; \
			}; \
		static zzFuncPtr zzAppendMemberGetPrev(zzNextMemberId##MemberName, std::vector<platform::Render::ShaderParametersMetadata::Member>* Members) \
		{ \
			Members->emplace_back(\
				#MemberName,\
				static_cast<white::uint32>(woffsetof(zzThisStruct,MemberName)),\
				ShaderType, \
				TypeInfo::BaseType,\
				TypeInfo::NumRows,\
				TypeInfo::NumColumns,\
				TypeInfo::NumElements,\
				TypeInfo::GetStructMetadata()\
			);\
			zzFuncPtr(*PrevFunc)(zzMemberId##MemberName, std::vector<platform::Render::ShaderParametersMetadata::Member>*); \
			PrevFunc = zzAppendMemberGetPrev; \
			return (zzFuncPtr)PrevFunc; \
		}\
		typedef zzNextMemberId##MemberName

#define END_SHADER_PARAMETER_STRUCT() \
	zzLastMemberId; \
	static std::vector<platform::Render::ShaderParametersMetadata::Member> zzGetMembers() { \
			std::vector<platform::Render::ShaderParametersMetadata::Member> Members; \
			zzFuncPtr(*LastFunc)(zzLastMemberId, std::vector<platform::Render::ShaderParametersMetadata::Member>*); \
			LastFunc = zzAppendMemberGetPrev; \
			zzFuncPtr Ptr = (zzFuncPtr)LastFunc; \
			do \
			{ \
				Ptr = reinterpret_cast<zzMemberFunc>(Ptr)(zzFirstMemberId(), &Members); \
			} while (Ptr); \
			return Members; \
		} \
	};

#define BEGIN_SHADER_PARAMETER_STRUCT(StructTypeName) \
		INTERNAL_SHADER_PARAMETER_STRUCT_BEGIN(StructTypeName,INTERNAL_LOCAL_SHADER_PARAMETER_GET_STRUCT_METADATA(StructTypeName))

/** Adds a constant-buffer stored value.
 *
 * Example:
 *	SHADER_PARAMETER(float, MyScalar)
 *	SHADER_PARAMETER(float4x4, MyMatrix)
 */
#define SHADER_PARAMETER(MemberType,MemberName) \
		SHADER_PARAMETER_EX(MemberType,MemberName)

#define SHADER_PARAMETER_EX(MemberType,MemberName) \
		INTERNAL_SHADER_PARAMETER_EXPLICIT(platform::Render::TShaderParameterTypeInfo<MemberType>::ShaderType, platform::Render::TShaderParameterTypeInfo<MemberType>,MemberType,MemberName)

 /** Adds a texture.
  *
  * Example:
  * SHADER_PARAMETER_TEXTURE(Texture2D, MyTexture)
  */
#define SHADER_PARAMETER_TEXTURE(MemberType,MemberName) \
	INTERNAL_SHADER_PARAMETER_EXPLICIT(platform::Render::TShaderTextureTypeInfo<MemberType>::ShaderType, platform::Render::TShaderTextureTypeInfo<MemberType>, ShaderType*,MemberName)

  /** Adds a sampler.
   *
   * Example:
   * SHADER_PARAMETER_SAMPLER(SamplerState, MySampler)
   */
#define SHADER_PARAMETER_SAMPLER(MemberType,MemberName) SHADER_PARAMETER(MemberType,MemberName)

   /** Adds an unordered access view.
	*
	* Example:
	*	SHADER_PARAMETER_UAV(RWTexture2D, MyUAV)
	*/
#define SHADER_PARAMETER_UAV(MemberType,MemberName) \
	INTERNAL_SHADER_PARAMETER_EXPLICIT(platform::Render::TShaderParameterTypeInfo<platform::Render::HLSLTraits::MemberType>::ShaderType, platform::Render::TShaderParameterTypeInfo<platform::Render::HLSLTraits::MemberType>,MemberType,MemberName)

	/** Adds a shader resource view.
	 *
	 * Example:
	 *	SHADER_PARAMETER_SRV(Texture2D, MySRV)
	 */
#define SHADER_PARAMETER_SRV(MemberType,MemberName) \
	INTERNAL_SHADER_PARAMETER_EXPLICIT(platform::Render::TShaderParameterTypeInfo<platform::Render::HLSLTraits::MemberType>::ShaderType, platform::Render::TShaderParameterTypeInfo<platform::Render::HLSLTraits::MemberType>,MemberType,MemberName)

	 /** Adds a shader constant buffer.
		*
		* Example:
		*	SHADER_PARAMETER_CBUFFER(Struct, MyStruct)
		*/
#define SHADER_PARAMETER_CBUFFER(MemberType,MemberName) \
	INTERNAL_SHADER_PARAMETER_EXPLICIT(platform::Render::SPT_ConstantBuffer, platform::Render::TShaderParameterTypeInfo<platform::Render::HLSLTraits::ConstantBuffer<MemberType>>,MemberType,MemberName)


		/** Include a shader parameter structure into another one in shader code.
		*
		* Example:
		*	BEGIN_SHADER_PARAMETER_STRUCT(FMyNestedStruct,)
		*		SHADER_PARAMETER(float, MyScalar)
		*		// ...
		*	END_SHADER_PARAMETER_STRUCT()
		*
		*	BEGIN_SHADER_PARAMETER_STRUCT(FOtherStruct)
		*		SHADER_PARAMETER_STRUCT_INCLUDE(FMyNestedStruct, MyStruct)
		*
		*
		*/
#define SHADER_PARAMETER_STRUCT_INCLUDE(StructType,MemberName) \
	INTERNAL_SHADER_PARAMETER_EXPLICIT(platform::Render::SPT_StructInclude, StructType::TypeInfo, StructType, MemberName)