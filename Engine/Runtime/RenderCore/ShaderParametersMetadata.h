#pragma once

#include "RenderInterface/ShaderCore.h"
#include <WBase/exception_type.h>

#define PR_NAMESPACE_BEGIN  namespace platform::Render {
#define PR_NAMESPACE_END }

PR_NAMESPACE_BEGIN
inline namespace Shader
{
	using white::uint32;

	class ShaderParametersMetadata
	{
	public:
		static constexpr const char* kRootUniformBufferBindingName = "_RootShaderParameters";


		class Member
		{
		public:
			Member(
				const char* InName,
				uint32 InOffset,
				ShaderParamType InType,
				ShaderBaseType InBaseType,
				uint32 InNumRows,
				uint32 InNumColumns,
				uint32 InNumElements,
				const ShaderParametersMetadata* InStruct
				)
				: Name(InName)
				, Offset(InOffset)
				, Type(InType)
				, BaseType(InBaseType)
				, NumRows(InNumRows)
				, NumColumns(InNumColumns)
				, NumElements(InNumElements)
				, Struct(InStruct)
			{}

			const char* GetName() const { return Name; }

			uint32 GetOffset() const { return Offset; }

			ShaderParamType GetShaderType() const { return Type; }

			/** Returns the number of row in the element. For instance FMatrix would return 4, or FVector would return 1. */
			uint32 GetNumRows() const { return NumRows; }

			/** Returns the number of column in the element. For instance FMatrix would return 4, or FVector would return 3. */
			uint32 GetNumColumns() const { return NumColumns; }

			/** Returns the number of elements in array, or 0 if this is not an array. */
			uint32 GetNumElements() const { return NumElements; }

			/** Returns the metadata of the struct. */
			const ShaderParametersMetadata* GetStructMetadata() const { return Struct; }

			/** Returns the size of the member. */
			uint32 GetMemberSize() const { 
				wassume(BaseType == SBT_INT32 || BaseType == SBT_UINT32 || BaseType == SBT_FLOAT32);

				uint32 ElementSize = sizeof(uint32) * NumRows * NumColumns;

				/** If this an array, the alignment of the element are changed. */
				if (NumElements > 0)
				{
					return white::Align(ElementSize, 16) * NumElements;
				}
				return ElementSize;
			}
		private:
			const char* Name;
			uint32 Offset;
			ShaderParamType Type;
			ShaderBaseType BaseType;

			uint32 NumRows;
			uint32 NumColumns;
			uint32 NumElements;

			const ShaderParametersMetadata* Struct;
		};

		ShaderParametersMetadata(
			uint32 InSize,
			std::vector<Member>&& InMembers);

		const std::vector<Member>& GetMembers() const { return Members; }

		uint32 GetSize() const { return Size; }
		uint32 GetBufferParameterCount() const { return BufferCount; }
		uint32 GetTextureParameterCount() const { return TextureCount; }
		uint32 GetCBufferParameterCount() const { return CBufferCount; }

		template<typename FunctionType>
		void EnumerateMembers(FunctionType Function, uint32 ParentOffset = 0) const
		{
			for (auto& member : GetMembers())
			{
				Function(member, member.GetOffset() + ParentOffset);
				if (auto ChildStruct = member.GetStructMetadata())
				{
					ChildStruct->EnumerateMembers(Function, member.GetOffset() + ParentOffset);
				}
			}
		}
	private:
		/** Size of the entire struct in bytes. */
		const uint32 Size;
		std::vector<Member> Members;

		const uint32 BufferCount;
		const uint32 TextureCount;
		const uint32 CBufferCount;

	};
}
PR_NAMESPACE_END

#undef PR_NAMESPACE_BEGIN
#undef PR_NAMESPACE_END
