/*! \file concurrency.h
\ingroup WBase
\brief 并发操作。

*/
#ifndef WBase_concurrency_h
#define WBase_concurrency_h 1

#include "pseudo_mutex.h" // for result_of_t, std::declval, std::make_shared,
//	threading::unlock_delete;
#include "future.hpp" // for std::packaged_task, future_result_t, pack_task;
#include "functional.hpp" // for std::bind, std::function, white::invoke;
#include "cassert.h" // for wassume;
#include <condition_variable> // for std::condition_variable;
#include <mutex> // for std::mutex, std::unique_lock;
#include <thread> // for std::thread;
#include <vector> // for std::vector;
#include <queue> // for std::queue;

namespace white
{

	/*!
	\brief 解锁删除器：默认使用标准库互斥量和锁。
	\sa threading::unlock_delete
	*/
	template<class _tMutex = std::mutex,
		class _tLock = std::unique_lock<_tMutex>>
		using unlock_delete = threading::unlock_delete<_tMutex, _tLock>;


	/*!
	\brief 独占所有权的锁定指针：默认使用标准库互斥量和锁。
	\sa threading::locked_ptr
	*/
	template<typename _type, class _tMutex = std::mutex,
		class _tLock = std::unique_lock<_tMutex>>
		using locked_ptr = threading::locked_ptr<_type, _tMutex, _tLock>;


	/*!
	\brief 转换 \c thread::id 为字符串表示。
	\note 使用 \c operator<< 实现。
	*/
	WB_API std::string
		to_string(const std::thread::id&);

	/*!
	\brief 取当前线程标识的字符串表示。
	\sa white::to_string
	*/
	inline std::string
		get_this_thread_id()
	{
		return white::to_string(std::this_thread::get_id());
	}


#	if !__GLIBCXX__ || (defined(_GLIBCXX_HAS_GTHREADS) \
	&& defined(_GLIBCXX_USE_C99_STDINT_TR1) && (ATOMIC_INT_LOCK_FREE > 1))
	/*!
	\brief 线程池。
	\note 除非另行约定，所有公开成员函数线程安全。
	\note 未控制线程队列的长度。
	*/
	class WB_API thread_pool
	{
	private:
		std::vector<std::thread> workers;
		std::queue<std::packaged_task<void()>> tasks{};
		mutable std::mutex queue_mutex{};
		std::condition_variable condition{};
		/*!
		\brief 停止状态。
		\note 仅在析构时设置停止。
		*/
		bool stopped = {};

	public:
		/*!
		\brief 初始化：使用指定的初始化和退出回调指定数量的工作线程。
		\note 若回调为空则忽略。
		\warning 回调的执行不提供顺序和并发安全保证。
		*/
		thread_pool(size_t, std::function<void()> = {}, std::function<void()> = {});
		/*!
		\brief 析构：设置停止状态并等待所有执行中的线程结束。
		\note 断言设置停止状态时不抛出 std::system_error 。
		\note 可能阻塞。忽略每个线程阻塞的 std::system_error 。
		\note 无异常抛出：断言。
		*/
		~thread_pool() wnothrow;

		/*!
		\see wait_to_enqueue
		*/
		template<typename _fCallable, typename... _tParams>
		future_result_t<_fCallable, _tParams...>
			enqueue(_fCallable&& f, _tParams&&... args)
		{
			return wait_to_enqueue([](std::unique_lock<std::mutex>&) wnothrow{
				return true;
			}, wforward(f), wforward(args)...);
		}

		size_t
			size() const;

		/*!
		\warning 非线程安全。
		*/
		size_t
			size_unlocked() const wnothrow
		{
			return tasks.size();
		}

		/*!
		\brief 等待操作进入队列。
		\param f 准备进入队列的操作
		\param args 进入队列操作时的参数。
		\param waiter 等待操作。
		\pre waiter 调用后满足条件变量后置条件；断言：持有锁。
		\return 任务共享状态（若等待失败则无效）。
		\warning 需要确保未被停止（未进入析构），否则不保证任务被运行。
		\warning 使用非递归锁，等待时不能再次锁定。
		*/
		template<typename _fWaiter, typename _fCallable, typename... _tParams>
		future_result_t<_fCallable, _tParams...>
			wait_to_enqueue(_fWaiter waiter, _fCallable&& f, _tParams&&... args)
		{
			auto bound(white::pack_task(wforward(f), wforward(args)...));
			auto res(bound.get_future());

			{
				std::unique_lock<std::mutex> lck(queue_mutex);

				if (waiter(lck))
				{
					wassume(lck.owns_lock());
					// TODO: Blocked. Use ISO C++14 lambda initializers to reduce
					//	initialization cost by directly moving from %bound.
					tasks.push(std::packaged_task<void()>(
						std::bind([](decltype(bound)& tsk) {
						tsk();
					}, std::move(bound))));
				}
				else
					return{};
			}
			condition.notify_one();
			return res;
		}
	};


	/*!
	\brief 任务池：带有队列大小限制的线程池。
	\note 除非另行约定，所有公开成员函数线程安全。
	\todo 允许调整队列大小限制。
	*/
	class WB_API task_pool : private thread_pool
	{
	private:
		size_t max_tasks;
		std::condition_variable enqueue_condition{};

	public:
		/*!
		\brief 初始化：使用指定的初始化和退出回调指定数量的工作线程和最大任务数。
		\sa thread_pool::thread_pool
		*/
		task_pool(size_t n, std::function<void()> on_enter = {},
			std::function<void()> on_exit = {})
			: thread_pool(std::max<size_t>(n, 1), on_enter, on_exit),
			max_tasks(std::max<size_t>(n, 1))
		{}

		//! \warning 非线程安全。
		bool
			can_enqueue_unlocked() const wnothrow
		{
			return size_unlocked() < max_tasks;
		}

		size_t
			get_max_task_num() const wnothrow
		{
			return max_tasks;
		}

		/*!
		\brief 重置线程池。
		\note 阻塞等待当前所有任务完成后重新创建。
		*/
		void
			reset() 
		{
			reset(max_tasks);
		}

		void
			reset(size_t);

		using thread_pool::size;

		//@{
		template<typename _func, typename _fCallable, typename... _tParams>
		future_result_t<_fCallable, _tParams...>
			poll(_func poller, _fCallable&& f, _tParams&&... args)
		{
			return wait_to_enqueue([=](std::unique_lock<std::mutex>& lck) {
				enqueue_condition.wait(lck, [=] {
					return poller() && can_enqueue_unlocked();
				});
				return true;
			}, wforward(f), wforward(args)...);
		}

		template<typename _func, typename _tDuration, typename _fCallable,
			typename... _tParams>
			future_result_t<_fCallable, _tParams...>
			poll_for(_func poller, const _tDuration& duration, _fCallable&& f,
				_tParams&&... args)
		{
			return wait_to_enqueue([=](std::unique_lock<std::mutex>& lck) {
				return enqueue_condition.wait_for(lck, duration, [=] {
					return poller() && can_enqueue_unlocked();
				});
			}, wforward(f), wforward(args)...);
		}

		template<typename _func, typename _tTimePoint, typename _fCallable,
			typename... _tParams>
			future_result_t<_fCallable, _tParams...>
			poll_until(_func poller, const _tTimePoint& abs_time,
				_fCallable&& f, _tParams&&... args)
		{
			return wait_to_enqueue([=](std::unique_lock<std::mutex>& lck) {
				return enqueue_condition.wait_until(lck, abs_time, [=] {
					return poller() && can_enqueue_unlocked();
				});
			}, wforward(f), wforward(args)...);
		}

		template<typename _fCallable, typename... _tParams>
		inline future_result_t<_fCallable, _tParams...>
			wait(_fCallable&& f, _tParams&&... args)
		{
			return poll([] {
				return true;
			}, wforward(f), wforward(args)...);
		}

		template<typename _tDuration, typename _fCallable, typename... _tParams>
		inline future_result_t<_fCallable, _tParams...>
			wait_for(const _tDuration& duration, _fCallable&& f, _tParams&&... args)
		{
			return poll_for([] {
				return true;
			}, duration, wforward(f), wforward(args)...);
		}

		template<typename _fWaiter, typename _fCallable, typename... _tParams>
		future_result_t<_fCallable, _tParams...>
			wait_to_enqueue(_fWaiter waiter, _fCallable&& f, _tParams&&... args)
		{
			return thread_pool::wait_to_enqueue(
				[=](std::unique_lock<std::mutex>& lck) -> bool {
				while (!can_enqueue_unlocked())
					if (!waiter(lck))
						return{};
				return true;
			}, [=](_tParams&&... f_args) {
				// TODO: Blocked. Use ISO C++14 lambda initializers to implement
				//	passing %f with %white::decay_copy.
				enqueue_condition.notify_one();
				// XXX: Blocked. 'wforward' cause G++ 5.2 failed (perhaps silently)
				//	with exit code 1.
				return white::invoke(f, std::forward<_tParams&&>(f_args)...);
			}, wforward(args)...);
		}

		template<typename _tTimePoint, typename _fCallable, typename... _tParams>
		inline future_result_t<_fCallable, _tParams...>
			wait_until(const _tTimePoint& abs_time, _fCallable&& f, _tParams&&... args)
		{
			return poll_until([] {
				return true;
			}, abs_time, wforward(f), wforward(args)...);
		}
		//@}
	};
#	endif

} // namespace white;

#endif