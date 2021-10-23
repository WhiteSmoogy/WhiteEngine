#include <WBase/cformat.h>
#include <WFramework/WCLib/Logger.h>

#include "TaskScheduler.h"
#include "AutoResetEvent.h"
#include "SpinMutex.h"
#include <atomic>
#include <memory>
#include <thread>
#include <mutex>
#include <queue>

using namespace platform;
using WhiteEngine::SpinMutex;

namespace
{
	namespace local
	{
		// Keep each thread's local queue under 1MB
		constexpr std::size_t max_local_queue_size = 1024 * 1024 / sizeof(void*);
		constexpr std::size_t initial_local_queue_size = 256;
	}
}

namespace white::threading {
	void SetThreadDescription(void* hThread, const wchar_t* lpThreadDescription);
}

thread_local white::coroutine::ThreadScheduler* thread_local_scheduler = nullptr;
thread_local white::coroutine::ThreadScheduler::thread_state* white::coroutine::ThreadScheduler::thread_local_state = nullptr;

white::threading::TaskScheduler* task_scheduler = nullptr;

namespace white::coroutine {
	class ThreadScheduler::thread_state
	{
	public:
		thread_state()
			:queue(std::make_unique<std::atomic<schedule_operation*>[]>(
				local::initial_local_queue_size)),
			queue_head(0),
			queue_tail(0),
			queue_mask(local::initial_local_queue_size - 1),
			sleeping(false)
		{}

		bool try_wake_up()
		{
			if (sleeping.load(std::memory_order_seq_cst))
			{
				if (sleeping.exchange(false, std::memory_order_seq_cst))
				{
					wakeup_event.set();
					return true;
				}
			}

			return false;
		}

		void notify_intent_to_sleep() noexcept
		{
			sleeping.store(true, std::memory_order_relaxed);
		}

		void sleep_until_woken() noexcept
		{
			wakeup_event.wait();
		}

		bool has_any_queued_work() noexcept
		{
			std::lock_guard lock{ remote_mutex };
			auto tail = queue_head.load(std::memory_order_relaxed);
			auto head = queue_tail.load(std::memory_order_seq_cst);
			return difference(head, tail) > 0;
		}

		bool try_local_enqueue(schedule_operation*& operation) noexcept
		{
			// Head is only ever written-to by the current thread so we
			// are safe to use relaxed memory order when reading it.
			auto head = queue_head.load(std::memory_order_relaxed);

			// It is possible this method may be running concurrently with
			// try_remote_steal() which may have just speculatively incremented m_tail
			// trying to steal the last item in the queue but has not yet read the
			// queue item. So we need to make sure we don't write to the last available
			// space (at slot m_tail - 1) as this may still contain a pointer to an
			// operation that has not yet been executed.
			//
			// Note that it's ok to read stale values from m_tail since new values
			// won't ever decrease the number of available slots by more than 1.
			// Reading a stale value can just mean that sometimes the queue appears
			// empty when it may actually have slots free.
			//
			// Here m_mask is equal to buffersize - 1 so we can only write to a slot
			// if the number of items consumed in the queue (head - tail) is less than
			// the mask.
			auto tail = queue_tail.load(std::memory_order_relaxed);
			if (difference(head, tail) < static_cast<offset_t>(queue_mask))
			{
				queue[queue_head & queue_mask].store(operation, std::memory_order_relaxed);
				queue_head.store(head + 1, std::memory_order_seq_cst);
				return true;
			}

			if (queue_mask == local::max_local_queue_size)
			{
				// No space in the buffer and we don't want to grow
				// it any further.
				return false;
			}

			// Allocate the new buffer before taking out the lock so that
			// we ensure we hold the lock for as short a time as possible.
			const size_t newSize = (queue_mask + 1) * 2;
			std::unique_ptr<std::atomic<schedule_operation*>[]> newLocalQueue{
				new (std::nothrow) std::atomic<schedule_operation*>[newSize]
			};
			if (!newLocalQueue)
			{
				// Unable to allocate more memory.
				return false;
			}

			if (!remote_mutex.try_lock())
			{
				// Don't wait to acquire the lock if we can't get it immediately.
				// Fail and let it be enqueued to the global queue.
				// TODO: Should we have a per-thread overflow queue instead?
				return false;
			}

			std::scoped_lock lock{ std::adopt_lock, remote_mutex };

			// We can now re-read tail, guaranteed that we are not seeing a stale version.
			tail = queue_tail.load(std::memory_order_relaxed);

			// Copy the existing operations.
			const size_t newMask = newSize - 1;
			for (size_t i = tail; i != head; ++i)
			{
				newLocalQueue[i & newMask].store(
					queue[i & queue_mask].load(std::memory_order_relaxed),
					std::memory_order_relaxed);
			}

			// Finally, write the new operation to the queue.
			newLocalQueue[head & newMask].store(operation, std::memory_order_relaxed);

			queue_head.store(head + 1, std::memory_order_relaxed);
			queue = std::move(newLocalQueue);
			queue_mask = newMask;
			return true;
		}

		schedule_operation* try_local_pop() noexcept
		{
			auto head = queue_head.load(std::memory_order_relaxed);
			auto tail = queue_tail.load(std::memory_order_relaxed);

			if (difference(head, tail) <= 0)
				return nullptr;

			auto new_head = head - 1;
			queue_head.store(new_head, std::memory_order_seq_cst);

			tail = queue_tail.load(std::memory_order_seq_cst);
			if (difference(new_head, tail) < 0)
			{
				// There was a race to get the last item.
				// We don't know whether the remote steal saw our write
				// and decided to back off or not, so we acquire the mutex
				// so that we wait until the remote steal has completed so
				// we can see what decision it made.
				std::lock_guard lock{ remote_mutex };

				// Use relaxed since the lock guarantees visibility of the writes
				// that the remote steal thread performed.
				tail = queue_tail.load(std::memory_order_relaxed);

				if (difference(new_head, tail) < 0)
				{
					// The other thread didn't see our write and stole the last item.
					// We need to restore the head back to it's old value.
					// We hold the mutex so can just use relaxed memory order for this.
					queue_head.store(head, std::memory_order_relaxed);
					return nullptr;
				}
			}

			return queue[new_head & queue_mask].load(std::memory_order_relaxed);
		}

		schedule_operation* try_steal(bool* lockUnavailable = nullptr) noexcept
		{
			if (lockUnavailable == nullptr)
				remote_mutex.lock();
			else if (!remote_mutex.try_lock())
			{
				*lockUnavailable = true;
				return nullptr;
			}

			std::scoped_lock lock{ std::adopt_lock,remote_mutex };

			auto tail = queue_tail.load(std::memory_order_relaxed);
			auto head = queue_head.load(std::memory_order_seq_cst);

			if (difference(head, tail) <= 0)
			{
				return nullptr;
			}

			queue_tail.store(tail + 1, std::memory_order_seq_cst);
			head = queue_head.load(std::memory_order_seq_cst);

			if (difference(head, tail) > 0)
			{
				// There was still an item in the queue after incrementing tail.
				// We managed to steal an item from the bottom of the stack.
				return queue[tail & queue_mask].load(std::memory_order_relaxed);
			}
			else
			{
				// Otherwise we failed to steal the last item.
				// Restore the old tail position.
				queue_tail.store(tail, std::memory_order_seq_cst);
				return nullptr;
			}
		}
	private:
		using offset_t = std::make_signed_t<std::size_t>;

		static constexpr offset_t difference(size_t a, size_t b)
		{
			return static_cast<offset_t>(a - b);
		}

		std::unique_ptr<std::atomic<schedule_operation*>[]> queue;
		std::atomic<std::size_t> queue_head;
		std::atomic<std::size_t> queue_tail;
		std::size_t queue_mask;

		std::atomic<bool> sleeping;
		SpinMutex remote_mutex;
		white::threading::auto_reset_event wakeup_event;
	};

	void ThreadScheduler::schedule_operation::await_suspend(std::coroutine_handle<> continuation) noexcept
	{
		continuation_handle = continuation;
		scheduler ? scheduler->schedule_impl(this): (any_scheduler->schedule_impl(this));
	}

	ThreadScheduler::schedule_operation::~schedule_operation()
	{
	}

	ThreadScheduler::ThreadScheduler(unsigned thread_index,const std::wstring& name)
		:current_state(new thread_state())
	{
		std::thread fire_forget(
		[this,thread_index] {
			thread_local_scheduler = this;
			thread_local_state = current_state;
			this->run(thread_index);
			}
		);
		native_handle = fire_forget.native_handle();

		std::wstring descirption = name;
		if (descirption.empty())
		{
			descirption = white::sfmt(L"Scheduler Worker%d", thread_index);
		}

		white::threading::SetThreadDescription(native_handle, descirption.c_str());

		fire_forget.detach();
	}

	void ThreadScheduler::run(unsigned thread_index) noexcept
	{
		while (true)
		{
			// Process operations from the local queue.
			schedule_operation* op;

			auto get_remote = [&]() ->schedule_operation*
			{
				// Try to get some new work first from the global queue
				// then if that queue is empty then try to steal from
				// the local queues of other worker threads.
				// We try to get new work from the global queue first
				// before stealing as stealing from other threads has
				// the side-effect of those threads running out of work
				// sooner and then having to steal work which increases
				// contention.
				if (task_scheduler == nullptr)
					return nullptr;
				auto* op = task_scheduler->get_remote();
				if (op == nullptr)
				{
					op = task_scheduler->try_steal_from_other_thread(thread_index);
				}
				return op;
			};

			while (true)
			{
				op = current_state->try_local_pop();
				if (op == nullptr)
				{
					op = get_remote();
					if (op == nullptr)
						break;
				}

				op->continuation_handle.resume();
			}

			// No more operations in the local queue or remote queue.
			//TODO
			// We spin for a little while waiting for new items
			// to be enqueued. This avoids the expensive operation
			// of putting the thread to sleep and waking it up again
			// in the case that an external thread is queueing new work

			current_state->notify_intent_to_sleep();
			current_state->sleep_until_woken();
		}
	}

	void ThreadScheduler::schedule_impl(schedule_operation* oper) noexcept
	{
		current_state->try_local_enqueue(oper);
		wake_up();
	}

	void ThreadScheduler::wake_up() noexcept
	{
		current_state->try_wake_up();
	}

}

namespace white::threading {
	class TaskScheduler::Scheduler
	{
	public:
		Scheduler(unsigned int InMaxScheduler)
			:queue_head(nullptr)
			, queue_tail(nullptr)
			,io_scheduler(InMaxScheduler)
			,schedulers(nullptr)
			,max_scheduler(InMaxScheduler)
		{
			std::thread io_thread1([this] {this->io_scheduler.process_events(); });
			std::thread io_thread2([this] {this->io_scheduler.process_events(); });

			SetThreadDescription(io_thread1.native_handle(), L"IO Thread 1");
			SetThreadDescription(io_thread2.native_handle(), L"IO Thread 2");

			io_thread1.detach();
			io_thread2.detach();

			std::allocator<white::coroutine::ThreadScheduler> allocate{};
			schedulers = allocate.allocate(max_scheduler);
			for (unsigned threadIndex = 0; threadIndex != max_scheduler; ++threadIndex)
			{
				new (&schedulers[threadIndex]) white::coroutine::ThreadScheduler(threadIndex);
			}

			render_scheduler = new white::coroutine::ThreadScheduler(max_scheduler,L"Scheduler Render");
		}

	private:
		friend class TaskScheduler;

		std::atomic<white::coroutine::ThreadScheduler::schedule_operation*> queue_tail;
		std::atomic<white::coroutine::ThreadScheduler::schedule_operation*> queue_head;

		white::coroutine::IOScheduler io_scheduler;
		white::coroutine::ThreadScheduler* schedulers;
		white::coroutine::ThreadScheduler* render_scheduler;
		unsigned int max_scheduler;
	};

	TaskScheduler::TaskScheduler()
		:scheduler_impl(new Scheduler(4))
	{
		task_scheduler = this;
	}

	white::coroutine::IOScheduler& threading::TaskScheduler::GetIOScheduler() noexcept
	{
		return scheduler_impl->io_scheduler;
	}

	white::coroutine::ThreadScheduler::schedule_operation threading::TaskScheduler::schedule() noexcept
	{
		return white::coroutine::ThreadScheduler::schedule_operation { this };
	}

	white::coroutine::ThreadScheduler::schedule_operation threading::TaskScheduler::schedule_render() noexcept
	{
		return white::coroutine::ThreadScheduler::schedule_operation{ scheduler_impl->render_scheduler };
	}

	bool threading::TaskScheduler::is_render_schedule() const noexcept
	{
		return scheduler_impl->render_scheduler == thread_local_scheduler;
	}

	void TaskScheduler::schedule_impl(white::coroutine::ThreadScheduler::schedule_operation* operation) noexcept
	{
		if (is_render_schedule() || thread_local_scheduler == nullptr || !white::coroutine::ThreadScheduler::thread_local_state->try_local_enqueue(operation))
		{
			remote_enqueue(operation);
		}
		
		wake_one_thread();
	}

	

	void TaskScheduler::wake_one_thread() noexcept
	{
		while (true)
		{
			for (std::uint32_t i = 0; i < scheduler_impl->max_scheduler; ++i)
			{
				if (scheduler_impl->schedulers[i].current_state->try_wake_up())
				{
					return;
				}
			}
			return;
		}
	}

	std::mutex queue_head_mutex;


	void TaskScheduler::remote_enqueue(white::coroutine::ThreadScheduler::schedule_operation* operation) noexcept
	{
		auto* tail = scheduler_impl->queue_tail.load(std::memory_order_relaxed);
		do {
			operation->next_oper = tail;
		} while (!scheduler_impl->queue_tail.compare_exchange_weak(tail, operation, std::memory_order_seq_cst,
			std::memory_order_relaxed));
	}

	white::coroutine::ThreadScheduler::schedule_operation* TaskScheduler::get_remote() noexcept
	{
		std::scoped_lock lock{ queue_head_mutex };

		auto* head = scheduler_impl->queue_head.load(std::memory_order_relaxed);

		if (head == nullptr)
		{
			if (scheduler_impl->queue_tail.load(std::memory_order_seq_cst) == nullptr)
				return nullptr;

			auto* tail = scheduler_impl->queue_tail.exchange(nullptr, std::memory_order_acquire);
			if (tail == nullptr)
				return nullptr;

			// Reverse the list 
			do
			{
				auto* next = std::exchange(tail->next_oper, head);
				head = std::exchange(tail, next);
			} while (tail != nullptr);
		}

		scheduler_impl->queue_head = head->next_oper;

		return head;
	}

	white::coroutine::ThreadScheduler::schedule_operation* TaskScheduler::try_steal_from_other_thread(unsigned this_index) noexcept
	{
		if (this_index == scheduler_impl->max_scheduler)
			return nullptr;
		// Try first with non-blocking steal attempts.
		bool anyLocksUnavailable = false;
		for (std::uint32_t otherThreadIndex = 0; otherThreadIndex < scheduler_impl->max_scheduler; ++otherThreadIndex)
		{
			if (otherThreadIndex == this_index) continue;
			auto& other_scheduler = scheduler_impl->schedulers[otherThreadIndex];
			auto* op = other_scheduler.current_state->try_steal(&anyLocksUnavailable);
			if (op != nullptr)
			{
				return op;
			}
		}

		if (anyLocksUnavailable)
		{
			// We didn't check all of the other threads for work to steal yet.
			// Try again, this time waiting to acquire the locks.
			for (std::uint32_t otherThreadIndex = 0; otherThreadIndex < scheduler_impl->max_scheduler; ++otherThreadIndex)
			{
				if (otherThreadIndex == this_index) continue;
				auto& other_scheduler = scheduler_impl->schedulers[otherThreadIndex];
				auto* op = other_scheduler.current_state->try_steal();
				if (op != nullptr)
				{
					return op;
				}
			}
		}

		return nullptr;
	}
}