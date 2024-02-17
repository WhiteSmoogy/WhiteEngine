module;
#include "Runtime/RenderCore/ShaderParametersMetadata.h"
#include <format>
export module RenderGraph:definition;

import "WBase/cassert.h";
import <concepts>;

import RenderGraphFwd;
import :allocator;

export namespace RenderGraph
{
	using platform::Render::ShaderParametersMetadata;
	using platform::Render::ShaderBaseType;

	template <typename LocalObjectType, typename LocalIndexType>
	class RGHandle
	{
	public:
		using ObjectType = LocalObjectType;
		using IndexType = LocalIndexType;

		static RGHandle Null;

		constexpr RGHandle() = default;

		explicit inline RGHandle(white::int32 InIndex)
		{
			wassume(InIndex >= 0 && InIndex <= kNullIndex);
			Index = (IndexType)InIndex;
		}

		IndexType GetIndex() const { wassume(IsValid()); return Index; }
		IndexType GetIndexUnwassumeed() const { return Index; }
		bool IsNull()  const { return Index == kNullIndex; }
		bool IsValid() const { return Index != kNullIndex; }
		operator bool() const { return IsValid(); }
		bool operator==(RGHandle Other) const { return Index == Other.Index; }
		bool operator!=(RGHandle Other) const { return Index != Other.Index; }
		bool operator<=(RGHandle Other) const { wassume(IsValid() && Other.IsValid()); return Index <= Other.Index; }
		bool operator>=(RGHandle Other) const { wassume(IsValid() && Other.IsValid()); return Index >= Other.Index; }
		bool operator< (RGHandle Other) const { wassume(IsValid() && Other.IsValid()); return Index < Other.Index; }
		bool operator> (RGHandle Other) const { wassume(IsValid() && Other.IsValid()); return Index > Other.Index; }

		RGHandle& operator+=(white::int32 Increment)
		{
			wassume(white::int64(Index + Increment) <= int64(kNullIndex));
			Index += (IndexType)Increment;
			return *this;
		}

		RGHandle& operator-=(white::int32 Decrement)
		{
			wassume(white::int64(Index - Decrement) > 0);
			Index -= (IndexType)Decrement;
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
			// If either index is null is will fail the comparison.
			return A.Index < B.Index ? A : B;
		}

		// Returns the max of two pass handles. Returns null if both are null; returns the valid handle if one is null.
		static RGHandle Max(RGHandle A, RGHandle B)
		{
			// If either index is null, it will wrap around to 0 and fail the comparison.
			return (IndexType)(A.Index + 1) > (IndexType)(B.Index + 1) ? A : B;
		}

	private:
		static const IndexType kNullIndex = std::numeric_limits<IndexType>::max();
		IndexType Index = kNullIndex;

		friend white::uint32 GetTypeHash(RGHandle Handle)
		{
			return Handle.GetIndex();
		}
	};

	template <typename LocalObjectType, typename LocalIndexType>
	RGHandle<LocalObjectType, LocalIndexType> RGHandle<LocalObjectType, LocalIndexType>::Null;

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
			Array.emplace_back(Object);
			Object->Handle = Last();
		}

		template<typename DerivedType = ObjectType, class ...TArgs>
		DerivedType* Allocate(RGAllocator& Allocator, TArgs&&... Args)
		{
			static_assert(std::derived_from<DerivedType, ObjectType>, "You must specify a type that derives from ObjectType");
			DerivedType* Object;
			if (DestructPolicy == ERGHandleRegistryDestructPolicy::Allocator)
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
			if (DestructPolicy == ERGHandleRegistryDestructPolicy::Registry)
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
			return Array[Handle.GetIndex()];
		}

		ObjectType* Get(HandleType Handle)
		{
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
	concept IsRGParameterStruct = requires{ TStruct::TypeInfo::GetStructMetadata; };

	class RGParameter
	{
	public:
		RGParameter(const ShaderParametersMetadata::Member* InMember, const uint8* InMemberPtr)
			:Member(InMember), MemberPtr(InMemberPtr)
		{}

		RGTextureSRVRef GetAsTextureSRV() const
		{
			return *GetAs<RGTextureSRVRef>();
		}

		RGBufferSRVRef GetAsBufferSRV() const
		{
			return *GetAs<RGBufferSRVRef>();
		}

		RGTextureUAVRef GetAsTextureUAV() const
		{
			return *GetAs<RGTextureUAVRef>();
		}

		RGBufferUAVRef GetAsBufferUAV() const
		{
			return *GetAs<RGBufferUAVRef>();
		}

		ShaderBaseType GetShaderBaseType() const
		{
			return platform::Render::GetBaseType(Member->GetShaderType());
		}

	private:
		template <typename T>
		const T* GetAs() const
		{
			return reinterpret_cast<const T*>(MemberPtr);
		}

		const ShaderParametersMetadata::Member* Member;
		const uint8* MemberPtr;
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

		template<typename FunctionType>
		void EnumerateBuffers(FunctionType Function) const
		{
			if (!Metadata)
				return;

			for (auto& member : Metadata->GetMembers())
			{
				if (IsBufferType(member.GetShaderType()))
				{
					Function(RGParameter{ &member,Contents + member.GetOffset() });
				}
			}
		}

		template <typename FunctionType>
		void EnumerateTextures(FunctionType Function) const
		{
			if (!Metadata)
				return;

			for (auto& member : Metadata->GetMembers())
			{
				if (IsTextureType(member.GetShaderType()))
				{
					Function(RGParameter{ &member,Contents + member.GetOffset() });
				}
			}
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

	class RGEventName final
	{
	public:
		RGEventName() = default;

		template<typename... Types>
		explicit RGEventName(const std::format_string<Types...> fmt, Types && ... Args)
		{
			EventName = std::format(fmt, std::forward<Types>(Args)...);
		}

		const char* GetName() const
		{
			return EventName.c_str();
		}
	private:
		const char* EventFormat;

		std::string EventName;
	};

	constexpr bool GRDGOverlapUAVs = true;
}