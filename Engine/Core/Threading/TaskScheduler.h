#pragma once

#include <WBase/wdef.h>
#include "../Coroutine/Task.h"
#include "../Coroutine/ThreadScheduler.h"
#include "../Coroutine/IOScheduler.h"

namespace white::threading {
	class TaskScheduler
	{
	public:
		[[nodiscard]]
		white::coroutine::ThreadScheduler::schedule_operation schedule() noexcept;

		TaskScheduler();

		template<typename AWAITABLE>
		white::coroutine::Task<void> Schedule(AWAITABLE awaitable)
		{
			co_await schedule();

			co_return co_await std::move(awaitable);
		}

		white::coroutine::IOScheduler& GetIOScheduler() noexcept;

		[[nodiscard]] white::coroutine::ThreadScheduler::schedule_operation schedule_render() noexcept;

		bool is_render_schedule() const noexcept;
	private:
		friend class white::coroutine::ThreadScheduler;

		void schedule_impl(white::coroutine::ThreadScheduler::schedule_operation* operation) noexcept;

		void remote_enqueue(white::coroutine::ThreadScheduler::schedule_operation* operation) noexcept;

		void wake_one_thread() noexcept;

		white::coroutine::ThreadScheduler::schedule_operation* get_remote() noexcept;
		white::coroutine::ThreadScheduler::schedule_operation* try_steal_from_other_thread(unsigned this_index) noexcept;

		class Scheduler;

		Scheduler* scheduler_impl;
	};
}