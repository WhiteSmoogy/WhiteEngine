#pragma once

#include <WBase/type_traits.hpp>
#include <WBase/smart_ptr.hpp>
#include <WBase/winttype.hpp>
#include <atomic>

namespace platform::Render {
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
		void release(RObject* obj) const
		{
			obj->Release();
		}

		void add_ref(RObject* obj) const
		{
			obj->AddRef();
		}
	};

	template<typename T,wimpl(typename = white::enable_if_t<std::is_convertible_v<T*,RObject*>>)>
		std::shared_ptr<T> shared_raw_robject(T* p)
	{
			return white::share_raw(p, RObjectDeleter());
	}

}