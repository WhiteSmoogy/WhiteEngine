#pragma once

#include <WBase/wdef.h>
#include <WBase/wmemory.hpp>
#include "Runtime/Core/Coroutine/ThreadScheduler.h"
#include "System/SystemEnvironment.h"

namespace platform::Render
{
	using schedule_operation = white::coroutine::ThreadScheduler::schedule_operation;

	class RenderTask;

	namespace details
	{
		class RenderPromise :public schedule_operation
		{
		public:
			RenderPromise()
				:schedule_operation(Environment->Scheduler->schedule_render())
			{}

			auto initial_suspend() noexcept
			{
				return *this;
			}

			auto final_suspend() noexcept
			{
				return std::suspend_always{};
			}

			RenderTask get_return_object() noexcept;

			void unhandled_exception() {
				exception = std::current_exception();
			}

			void return_void() noexcept
			{
				if (exception)
				{
					std::rethrow_exception(exception);
				}
			}
		private:
			std::exception_ptr exception;
		};
	}

	class RenderTask
	{
	public:
		using promise_type = details::RenderPromise;

		explicit RenderTask(std::coroutine_handle<promise_type> coroutine)
			: coroutine_handle(coroutine)
		{}

		explicit RenderTask(std::nullptr_t)
			:coroutine_handle(nullptr)
		{}

		RenderTask(RenderTask&& t) noexcept
			: coroutine_handle(t.coroutine_handle)
		{
			t.coroutine_handle = nullptr;
		}

		~RenderTask()
		{
			if (coroutine_handle)
			{
				coroutine_handle.destroy();
			}
			coroutine_handle = nullptr;
		}

		bool is_ready() const noexcept
		{
			return !coroutine_handle || coroutine_handle.done();
		}

		void swap(RenderTask&& t) noexcept
		{
			std::swap(coroutine_handle, t.coroutine_handle);
		}

		void assign(RenderTask&& t) noexcept
		{
			this->~RenderTask();
			swap(std::move(t));
		}

	private:
		std::coroutine_handle<promise_type> coroutine_handle;
	};

	namespace details
	{
		inline RenderTask RenderPromise::get_return_object() noexcept
		{
			return RenderTask{ std::coroutine_handle<RenderPromise>::from_promise(*this) };
		}
	}

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

	inline SyncPoint::awaiter operator co_await(std::shared_ptr<SyncPoint> dispatcher)
	{
		return { {}, dispatcher };
	}
}