#pragma once

#include <WBase/wdef.h>
#include "../Coroutine/Task.h"
#include "../Coroutine/ThreadScheduler.h"
#include "../Coroutine/IOScheduler.h"
#include "../Coroutine/AwaitableTraits.h"

namespace white::threading {
	enum class TaskTag
	{
		None = 0,
		WorkerThread = 1 << 0,
		//CommandListExecuteThread
		RenderThread = 1 << 1,
		IOThread = 1 << 2,
	};

	extern thread_local TaskTag ActiveTag;

	class TaskScheduler
	{
	public:
		[[nodiscard]]
		white::coroutine::ThreadScheduler::schedule_operation schedule() noexcept;

		TaskScheduler();

		template<typename AWAITABLE>
		requires requires{typename white::coroutine::AwaitableTraits<AWAITABLE>::awaiter_t; }
		white::coroutine::Task<void> Schedule(AWAITABLE awaitable)
		{
			co_await schedule();

			co_return co_await std::move(awaitable);
		}

		template<typename F,class... ArgTypes>
		white::coroutine::Task<invoke_result_t<F, ArgTypes...>> Schedule(F&& f, ArgTypes&&... args)
		{
			co_await schedule();

			co_return  std::invoke(std::forward<F>(f),std::forward<ArgTypes>(args)...);
		}

		white::coroutine::IOScheduler& GetIOScheduler() noexcept;

		[[nodiscard]] white::coroutine::ThreadScheduler::schedule_operation schedule_render() noexcept;

		bool is_render_schedule() const noexcept;
	private:
		friend class white::coroutine::ThreadScheduler;
		friend class white::coroutine::PoolThreadScheduler;

		void schedule_impl(white::coroutine::ThreadScheduler::schedule_operation* operation) noexcept;

		void remote_enqueue(white::coroutine::ThreadScheduler::schedule_operation* operation) noexcept;

		void wake_one_thread() noexcept;

		white::coroutine::ThreadScheduler::schedule_operation* get_remote() noexcept;
		white::coroutine::ThreadScheduler::schedule_operation* try_steal_from_other_thread(unsigned this_index) noexcept;

		class Scheduler;

		Scheduler* scheduler_impl;
	};
}