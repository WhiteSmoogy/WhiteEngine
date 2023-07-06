#pragma once

#include <WBase/wdef.h>
#include <WBase/wmemory.hpp>
#include <coroutine>

namespace platform::Render
{
	struct SyncPoint
	{
		struct awaiter : std::suspend_always
		{
			bool await_ready() 
			{
				return syncpoint->IsReady();
			}

			void await_suspend(
				std::coroutine_handle<> handle)
			{
				return syncpoint->AwaitSuspend(handle);
			}

			std::shared_ptr<SyncPoint> syncpoint;
		};

		virtual ~SyncPoint();

		virtual bool IsReady() const = 0;

		virtual void Wait() = 0;

		virtual void AwaitSuspend(std::coroutine_handle<> handle) = 0;
	};

	SyncPoint::awaiter operator co_await(std::shared_ptr<SyncPoint> dispatcher)
	{
		return { {}, dispatcher };
	}
}