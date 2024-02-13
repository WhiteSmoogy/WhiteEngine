#pragma once

#include <WBase/type_traits.hpp>
#include <WBase/smart_ptr.hpp>
#include <WBase/winttype.hpp>
#include <atomic>

#include "IFormat.hpp"

namespace platform::Render {
	struct ResourceCreateInfo
	{
		unsigned int GPUIndex = 0;
		bool WithoutNativeResource = false;

		const ElementInitData* InitData = nullptr;
		const ClearValueBinding* clear_value = nullptr;
		const char* Name = "unknown";

		ResourceCreateInfo() = default;

		ResourceCreateInfo(const char* InName)
			:Name(InName)
		{}

		ResourceCreateInfo(const ElementInitData* init_data, const char* InName)
			:InitData(init_data), Name(InName)
		{}

		const void* GetData() const
		{
			if (InitData != nullptr)
				return InitData->data;
			return nullptr;
		}
	};

	struct ResourceCreateInfoEx :ResourceCreateInfo
	{
		ElementInitData InitData;
		template<typename Data>
		ResourceCreateInfoEx(const Data* init_data,const char* name)
			:ResourceCreateInfo(&InitData,name),InitData{.data=init_data}
		{}
	};

	class RObject :public white::ref_count_base
	{
	public:
		virtual ~RObject();

		value_type AddRef()
		{
			return ++uses;
		}

		value_type Release()
		{
			auto NewValue = --uses;
			if (NewValue == 0)
			{
				EnqueueDelete(this);
			}
			return NewValue;
		}

		static void FlushPendingDeletes();

	private:
		static void EnqueueDelete(RObject* value);

		//D3D12 API don't do internal reference counting
		struct RObjectToDelete
		{
			RObjectToDelete(white::uint32 InFrameDeleted = 0)
				: FrameDeleted(InFrameDeleted)
			{

			}

			std::vector<RObject*>	Objects;
			white::uint32					FrameDeleted;
		};

		static std::vector<RObjectToDelete> DeferredDeletionQueue;
		static white::uint32 CurrentFrame;
	};

	struct RObjectDeleter
	{
		void operator()(RObject* obj) const
		{
			obj->Release();
		}
	};

	struct RObjectController
	{
		static void release(RObject* obj)
		{
			obj->Release();
		}

		static void add_ref(RObject* obj)
		{
			obj->AddRef();
		}

		static white::uint32 count(RObject* obj)
		{
			return obj->count();
		}
	};

	template<typename T,wimpl(typename = white::enable_if_t<std::is_convertible_v<T*,RObject*>>)>
		std::shared_ptr<T> shared_raw_robject(T* p)
	{
			return white::share_raw(p, RObjectDeleter());
	}

}