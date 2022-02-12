#pragma once

#include <WBase/wdef.h>
#include <coroutine>
#include <thread>

namespace white::threading
{
	class TaskScheduler;
}

namespace white::coroutine {
	struct schedule_operation_basic
	{
		std::coroutine_handle<> continuation_handle;
	};

	class ThreadScheduler
	{
	public:
		class schedule_operation :public schedule_operation_basic
		{
		public:
			schedule_operation(ThreadScheduler* ts) noexcept : scheduler(ts), any_scheduler(nullptr), next_oper(nullptr){}
			schedule_operation(white::threading::TaskScheduler* ts) noexcept : scheduler(nullptr),any_scheduler(ts), next_oper(nullptr) {}

			~schedule_operation();

			bool await_ready() noexcept { return false; }
			void await_suspend(std::coroutine_handle<> continuation) noexcept;
			void await_resume() noexcept {}
		public:
			schedule_operation* next_oper;
		protected:

			friend class ThreadScheduler;

			ThreadScheduler* scheduler;
			white::threading::TaskScheduler* any_scheduler;
		};

		[[nodiscard]]
		schedule_operation schedule() noexcept { return schedule_operation{ this }; }
	protected:
		virtual bool schedule_impl(schedule_operation* operation) noexcept = 0;

		friend class white::threading::TaskScheduler;
	};

	class PoolThreadScheduler :public ThreadScheduler
	{
		class thread_state;

		thread_state* current_state;

	public:
		PoolThreadScheduler(unsigned thread_index);
	protected:
		bool schedule_impl(schedule_operation* operation) noexcept final;

		void run(unsigned thread_index) noexcept;

		void wake_up() noexcept;

		friend class white::threading::TaskScheduler;

	private:

		std::thread::native_handle_type native_handle;
	};

	class RenderThreadScheduler : public ThreadScheduler
	{
		class thread_state;

		thread_state* current_state;
	public:
		RenderThreadScheduler();
	protected:
		bool schedule_impl(schedule_operation* operation) noexcept final;

		void run() noexcept;

		std::thread::native_handle_type native_handle;
	};
}