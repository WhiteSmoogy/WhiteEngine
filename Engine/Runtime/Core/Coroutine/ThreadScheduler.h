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

		ThreadScheduler(unsigned thread_index, const std::wstring& name = {});
	private:
		void schedule_impl(schedule_operation* operation) noexcept;

		void wake_up() noexcept;

		void run(unsigned thread_index) noexcept;

		class thread_state;

		thread_state* current_state;

		static thread_local thread_state* thread_local_state;

		std::thread::native_handle_type native_handle;

		friend class white::threading::TaskScheduler;
	};
}