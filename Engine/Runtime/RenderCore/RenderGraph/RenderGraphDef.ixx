module;
#include "Runtime/RenderCore/ShaderParametersMetadata.h"
#include "Runtime/RenderCore/ShaderParameterResource.h"
#include "RenderInterface/IFormat.hpp"

#include <algorithm>
#include <compare>
#include <concepts>
#include <cstring>
#include <format>
#include <iterator>
#include <limits>
#include <type_traits>
#include <utility>
#include <vector>
export module RenderGraph:definition;

import "WBase/cassert.h";

import :fwd;
import :allocator;

export namespace RenderGraph
{
	using platform::Render::ShaderParametersMetadata;
	using platform::Render::ShaderBaseType;
	using platform::Render::ShaderParamType;
	using platform::Render::EAccessHint;

	template <typename LocalObjectType, typename LocalIndexType>
	class RGHandle
	{
	public:
		using ObjectType = LocalObjectType;
		using IndexType = LocalIndexType;

		static const RGHandle Null;

		constexpr RGHandle() = default;

		explicit constexpr RGHandle(white::int32 InIndex)
		{
			wassume(InIndex >= 0 && static_cast<white::uint64>(InIndex) < kNullIndex);
			Index = (IndexType)InIndex;
		}

		constexpr IndexType GetIndex() const { wassume(IsValid()); return Index; }
		constexpr IndexType GetIndexUnchecked() const { return Index; }
		constexpr bool IsNull()  const { return Index == kNullIndex; }
		constexpr bool IsValid() const { return Index != kNullIndex; }
		explicit constexpr operator bool() const { return IsValid(); }
		constexpr bool operator==(RGHandle Other) const { return Index == Other.Index; }
		constexpr bool operator!=(RGHandle Other) const { return Index != Other.Index; }
		bool operator<=(RGHandle Other) const { wassume(IsValid() && Other.IsValid()); return Index <= Other.Index; }
		bool operator>=(RGHandle Other) const { wassume(IsValid() && Other.IsValid()); return Index >= Other.Index; }
		bool operator< (RGHandle Other) const { wassume(IsValid() && Other.IsValid()); return Index < Other.Index; }
		bool operator> (RGHandle Other) const { wassume(IsValid() && Other.IsValid()); return Index > Other.Index; }

		RGHandle& operator+=(white::int32 Increment)
		{
			wassume(IsValid());
			const auto NewIndex = white::int64(Index) + Increment;
			wassume(NewIndex >= 0 && NewIndex < int64(kNullIndex));
			Index = static_cast<IndexType>(NewIndex);
			return *this;
		}

		RGHandle& operator-=(white::int32 Decrement)
		{
			wassume(IsValid());
			const auto NewIndex = white::int64(Index) - Decrement;
			wassume(NewIndex >= 0 && NewIndex < int64(kNullIndex));
			Index = static_cast<IndexType>(NewIndex);
			return *this;
		}

		RGHandle operator-(white::int32 Subtract) const
		{
			RGHandle Handle = *this;
			Handle -= Subtract;
			return Handle;
		}

		RGHandle operator+(white::int32 Add) const
		{
			RGHandle Handle = *this;
			Handle += Add;
			return Handle;
		}

		RGHandle& operator++()
		{
			wassume(IsValid());
			++Index;
			return *this;
		}

		RGHandle& operator--()
		{
			wassume(IsValid());
			--Index;
			return *this;
		}

		// Returns the min of two pass handles. Returns null if both are null; returns the valid handle if one is null.
		static RGHandle Min(RGHandle A, RGHandle B)
		{
			if (A.IsNull()) return B;
			if (B.IsNull()) return A;
			return A.Index < B.Index ? A : B;
		}

		// Returns the max of two pass handles. Returns null if both are null; returns the valid handle if one is null.
		static RGHandle Max(RGHandle A, RGHandle B)
		{
			if (A.IsNull()) return B;
			if (B.IsNull()) return A;
			return A.Index > B.Index ? A : B;
		}

	private:
		static const IndexType kNullIndex = std::numeric_limits<IndexType>::max();
		IndexType Index = kNullIndex;

		friend white::uint32 GetTypeHash(RGHandle Handle)
		{
			return Handle.GetIndexUnchecked();
		}
	};

	template <typename LocalObjectType, typename LocalIndexType>
	inline const RGHandle<LocalObjectType, LocalIndexType> RGHandle<LocalObjectType, LocalIndexType>::Null{};

	enum class ERGHandleRegistryDestructPolicy
	{
		Registry,
		Allocator,
		Never
	};

	template <typename LocalHandleType, ERGHandleRegistryDestructPolicy DestructPolicy = ERGHandleRegistryDestructPolicy::Registry>
	class RGHandleRegistry
	{
	public:
		using HandleType = LocalHandleType;
		using ObjectType = typename HandleType::ObjectType;
		using IndexType = typename HandleType::IndexType;

		RGHandleRegistry() = default;
		RGHandleRegistry(const RGHandleRegistry&) = delete;
		RGHandleRegistry(RGHandleRegistry&&) = default;
		RGHandleRegistry& operator=(RGHandleRegistry&&) = default;
		RGHandleRegistry& operator=(const RGHandleRegistry&) = delete;

		~RGHandleRegistry()
		{
			Clear();
		}

		void Insert(ObjectType* Object)
		{
			wassume(Object != nullptr);
			wassume(Array.size() < std::numeric_limits<IndexType>::max());
			Array.emplace_back(Object);
			Object->Handle = Last();
		}

		template<typename DerivedType = ObjectType, class ...TArgs>
		DerivedType* Allocate(RGAllocator& Allocator, TArgs&&... Args)
		{
			static_assert(std::derived_from<DerivedType, ObjectType>, "You must specify a type that derives from ObjectType");
			DerivedType* Object;
			if constexpr (DestructPolicy == ERGHandleRegistryDestructPolicy::Allocator)
			{
				Object = Allocator.Alloc<DerivedType>(std::forward<TArgs>(Args)...);
			}
			else
			{
				Object = Allocator.AllocNoDestruct<DerivedType>(std::forward<TArgs>(Args)...);
			}
			Insert(Object);
			return Object;
		}

		void Clear()
		{
			if constexpr (DestructPolicy == ERGHandleRegistryDestructPolicy::Registry)
			{
				for (auto itr = Array.rbegin();itr != Array.rend();++itr)
				{
					(*itr)->~ObjectType();
				}
			}
			Array.clear();
		}

		template <typename FunctionType>
		void Enumerate(FunctionType Function)
		{
			for (ObjectType* Object : Array)
			{
				Function(Object);
			}
		}

		template <typename FunctionType>
		void Enumerate(FunctionType Function) const
		{
			for (const ObjectType* Object : Array)
			{
				Function(Object);
			}
		}

		const ObjectType* Get(HandleType Handle) const
		{
			wassume(Handle.GetIndex() < Array.size());
			return Array[Handle.GetIndex()];
		}

		ObjectType* Get(HandleType Handle)
		{
			wassume(Handle.GetIndex() < Array.size());
			return Array[Handle.GetIndex()];
		}

		const ObjectType* operator[] (HandleType Handle) const
		{
			return Get(Handle);
		}

		ObjectType* operator[] (HandleType Handle)
		{
			return Get(Handle);
		}

		HandleType Begin() const
		{
			return HandleType(0);
		}

		HandleType End() const
		{
			return HandleType(Array.size());
		}

		HandleType Last() const
		{
			return HandleType(Num() - 1);
		}

		int32 Num() const
		{
			return static_cast<int32>(Array.size());
		}

	private:
		std::vector<ObjectType*, RGSTLAllocator<ObjectType*>> Array;
	};

	template <typename HandleType>
	class RGTHandleUniqueFilter
	{
	public:
		RGTHandleUniqueFilter() = default;

		RGTHandleUniqueFilter(HandleType InHandle)
		{
			AddHandle(InHandle);
		}

		void Reset()
		{
			Handle = HandleType::Null;
			bUnique = false;
		}

		void AddHandle(HandleType InHandle)
		{
			if (Handle != InHandle && InHandle.IsValid())
			{
				bUnique = Handle.IsNull();
				Handle = InHandle;
			}
		}

		HandleType GetUniqueHandle() const
		{
			return bUnique ? Handle : HandleType::Null;
		}

	private:
		HandleType Handle;
		bool bUnique = false;
	};

	template <typename TStruct>
	concept IsRGParameterStruct = requires
	{
		{ TStruct::TypeInfo::GetStructMetadata() } -> std::convertible_to<const ShaderParametersMetadata*>;
	};

	struct RGBufferAccess
	{
		RGBufferAccess() = default;

		RGBufferAccess(RGBuffer* InBuffer, EAccessHint InAccess)
			: Buffer(InBuffer)
			, Access(InAccess)
		{
		}

		RGBuffer* GetBuffer() const
		{
			return Buffer;
		}

		EAccessHint GetAccess() const
		{
			return Access;
		}

		operator bool() const
		{
			return Buffer != nullptr;
		}

		operator RGBuffer* () const
		{
			return Buffer;
		}

		RGBuffer* operator->() const
		{
			return Buffer;
		}

		auto operator<=>(const RGBufferAccess&) const = default;

		RGBuffer* Buffer = nullptr;
		EAccessHint Access = EAccessHint::None;
	};


	class RGParameter
	{
	public:
		RGParameter(const ShaderParametersMetadata::Member* InMember, const uint8* InMemberPtr)
			:Member(InMember), MemberPtr(InMemberPtr)
		{}

		RGTextureSRVRef GetAsTextureSRV() const
		{
			return GetAsRenderGraphResource<RGTextureSRVRef>();
		}

		RGBufferSRVRef GetAsBufferSRV() const
		{
			return GetAsRenderGraphResource<RGBufferSRVRef>();
		}

		RGTextureUAVRef GetAsTextureUAV() const
		{
			return GetAsRenderGraphResource<RGTextureUAVRef>();
		}

		RGBufferUAVRef GetAsBufferUAV() const
		{
			return GetAsRenderGraphResource<RGBufferUAVRef>();
		}

		ShaderBaseType GetShaderBaseType() const
		{
			return platform::Render::GetShaderBaseType(Member->GetShaderType());
		}

		ShaderParamType GetShaderType() const
		{
			return Member->GetShaderType();
		}

		RGBufferAccess GetAsBufferAccess() const
		{
			return *GetAs<RGBufferAccess>();
		}

	protected:
		template <typename T>
		T GetAsRenderGraphResource() const
		{
			const auto* Resource = reinterpret_cast<const platform::Render::HLSLTraits::UnionPointerBase*>(MemberPtr);
			return Resource->IsRenderGraph() ? static_cast<T>(Resource->GetValue()) : nullptr;
		}

		template <typename T>
		const T* GetAs() const
		{
			return reinterpret_cast<const T*>(MemberPtr);
		}

		const ShaderParametersMetadata::Member* Member;
		const uint8* MemberPtr;
	};

	class RGUniformBufferBinding :protected RGParameter
	{
	public:
		RGUniformBufferBinding(const ShaderParametersMetadata::Member* InMember, const uint8* InMemberPtr)
			:RGParameter(InMember,InMemberPtr)
		{}

		RGConstBuffer* GetCBuffer() const
		{
			return GetAsRenderGraphResource<RGConstBuffer*>();
		}
	};


	class RGParameterStruct
	{
	public:
		template<typename TStruct>
		explicit RGParameterStruct(const TStruct* InParameters, const ShaderParametersMetadata* InParameterMetadata)
			:Contents(reinterpret_cast<const uint8*>(InParameters)), Metadata(InParameterMetadata)
		{}

		const uint8* GetContents() const { return Contents; }

		template<typename TStruct>
		static const ShaderParametersMetadata* GetStructMetadata()
		{
			if constexpr (requires { TStruct::TypeInfo::GetStructMetadata; })
				return TStruct::TypeInfo::GetStructMetadata();
			return nullptr;
		}

		uint32 GetSize() const 
		{ 
			if(Metadata)
				return Metadata->GetSize();
			return 0;
		}

		uint32 GetBufferParameterCount() const
		{
			if (!Metadata)
				return 0;

			return Metadata->GetBufferParameterCount();
		}

		uint32 GetTextureParameterCount() const
		{
			if (!Metadata)
				return 0;

			return Metadata->GetTextureParameterCount();
		}

		uint32 GetCBufferCount() const
		{
			if (!Metadata)
				return 0;

			return Metadata->GetCBufferParameterCount();
		}

		template<typename FunctionType>
		void EnumerateBuffers(FunctionType Function) const
		{
			if (!Metadata)
				return;

			Metadata->EnumerateMembers([&](const ShaderParametersMetadata::Member& member, uint32 Offset)
				{
					if (IsBufferType(member.GetShaderType()))
					{
						Function(RGParameter{ &member,Contents + Offset });
					}
				});
		}

		template<typename FunctionType>
		void EnumerateCBuffers(FunctionType Function) const
		{
			if (!Metadata)
				return;

			Metadata->EnumerateMembers([&](const ShaderParametersMetadata::Member& member, uint32 Offset)
				{
					if (member.GetShaderType() == ShaderParamType::SPT_ConstantBuffer)
					{
						Function(RGUniformBufferBinding{ &member,Contents + Offset });
					}
				});
		}

		template <typename FunctionType>
		void EnumerateTextures(FunctionType Function) const
		{
			if (!Metadata)
				return;

			Metadata->EnumerateMembers([&](const ShaderParametersMetadata::Member& member, uint32 Offset)
				{
					if (IsTextureType(member.GetShaderType()))
					{
						Function(RGParameter{ &member,Contents + Offset });
					}
				});
		}

	private:

		const ShaderParametersMetadata* Metadata = nullptr;
		const uint8* Contents;

		friend RGPass;
	};

	template<typename TStruct>
	class RGTParameterStruct : public RGParameterStruct
	{
	public:
		explicit RGTParameterStruct(const TStruct* InParameters)
			:RGParameterStruct(InParameters, GetStructMetadata<TStruct>())
		{}

		const TStruct* GetContents() const
		{
			return reinterpret_cast<TStruct*>(RGParameterStruct::GetContents());
		}

		const TStruct* operator->() const
		{
			return GetContents();
		}
	};

	class RGString
	{
	public:
		using value_type = char;

		const char* c_str() const
		{
			return Buffer ? Buffer : "";
		}

		void push_back(char c)
		{
			if (Length + 1 >= Capacity)
			{
				Reserve(Growth(Length+1, Capacity));
			}

			Buffer[Length++] = c;
			Buffer[Length] = 0;
		}
	private:
		uint32 Growth(uint32 Requested, uint32 Old)
		{
			auto Mask = Requested | 0xF;

			return std::max(Mask, Old + Old / 2);
		}

		void Reserve(uint32 NewCapacity)
		{
			if (NewCapacity <= Capacity)
			{
				return;
			}

			char* NewBuffer = GetAllocator().AllocUninitialized<char>(NewCapacity);
			if (Buffer)
			{
				memcpy(NewBuffer, Buffer, Length);
			}
			Buffer = NewBuffer;
			Capacity = NewCapacity;
		}
	private:
		char* Buffer = nullptr;
		uint32 Length = 0;
		uint32 Capacity = 0; 
	};

	class RGEventName final
	{
	public:
		RGEventName() = default;

		template<typename... Types>
		explicit RGEventName(const std::format_string<Types...> fmt, Types && ... Args)
		{
			std::format_to(std::back_inserter(EventName), fmt, std::forward<Types>(Args)...);
		}

		const char* GetName() const
		{
			return EventName.c_str();
		}
	private:
		RGString EventName;
	};

	constexpr bool GRGOverlapUAVs = true;
	constexpr bool GRGCullPasses = true;
	constexpr bool GRGTransientAllocator = false;
}
