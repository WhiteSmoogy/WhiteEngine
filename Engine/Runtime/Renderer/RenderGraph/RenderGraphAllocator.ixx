module;
#include "Runtime/MemStack.h"

export module RenderGraph:allocator;

import :fwd;

#pragma warning(disable:4291)

export namespace RenderGraph
{
	class RGAllocator
	{
	public:
		struct AObject
		{
			virtual ~AObject() = default;
		};

		template <typename T>
		class TObject final :public AObject
		{
			friend class RGAllocator;
		private:
			template <typename... TArgs>
			inline TObject(TArgs&&... Args)
				: Alloc(std::forward<TArgs&&>(Args)...)
			{}

			T Alloc;
		};

		inline void* Alloc(uint64 SizeInBytes, uint32 AlignInBytes)
		{
			return GetContext().MemStack.Alloc(static_cast<uint32>(SizeInBytes), AlignInBytes);
		}

		template <typename TrivialType>
		inline TrivialType* AllocUninitialized(uint64 Count = 1)
		{
			return reinterpret_cast<TrivialType*>(Alloc(sizeof(TrivialType) * Count, alignof(TrivialType)));
		}

		/** Allocates and constructs an object and tracks it for destruction. */
		template <typename T, typename... TArgs>
		inline T* Alloc(TArgs&&... Args)
		{
			AContext& LocalContext = GetContext();
			TObject<T>* Object = new(LocalContext.MemStack) TObject<T>(std::forward<TArgs&&>(Args)...);
			LocalContext.Objects.emplace_back(Object);
			return &Object->Alloc;
		}

		template <typename T, typename... TArgs>
		inline T* AllocNoDestruct(TArgs&&... Args)
		{
			return new (GetContext().MemStack) T(std::forward<TArgs&&>(Args)...);
		}

	private:
		void Release()
		{
			Context.Release();
		}

		struct AContext
		{
			WhiteEngine::MemStackBase MemStack;

			std::vector<AObject*> Objects;

			void Release()
			{
				for (auto itr = Objects.rbegin(); itr != Objects.rend(); ++itr)
				{
					(*itr)->~AObject();
				}

				Objects.clear();

				MemStack.Flush();
			}
		};

		alignas(std::hardware_destructive_interference_size) AContext Context;

		AContext& GetContext()
		{
			return Context;
		}

		friend class RGAllocatorScope;
	};

	RGAllocator& GetAllocator()
	{
		static RGAllocator Instance;
		return Instance;
	}

	template<class T>
	struct RGSTLAllocator
	{
		typedef T value_type;

		RGSTLAllocator() = default;

		template<class U>
		constexpr RGSTLAllocator(const RGSTLAllocator <U>&) noexcept {}

		[[nodiscard]] T* allocate(std::size_t n)
		{
			auto p = GetAllocator().Alloc(n * sizeof(T), alignof(T));

			return static_cast<T*>(p);
		}

		void deallocate(T* p, std::size_t n) noexcept
		{
		}
	};

	class RGAllocatorScope
	{
	public:
		RGAllocatorScope()
			: Allocator(GetAllocator())
		{}

		~RGAllocatorScope()
		{
			Allocator.Release();
		}

	protected:
		RGAllocator& Allocator;

	private:
	};
}

