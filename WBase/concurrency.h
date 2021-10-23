/*! \file concurrency.h
\ingroup WBase
\brief ����������

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
	\brief ����ɾ������Ĭ��ʹ�ñ�׼�⻥����������
	\sa threading::unlock_delete
	*/
	template<class _tMutex = std::mutex,
		class _tLock = std::unique_lock<_tMutex>>
		using unlock_delete = threading::unlock_delete<_tMutex, _tLock>;


	/*!
	\brief ��ռ����Ȩ������ָ�룺Ĭ��ʹ�ñ�׼�⻥����������
	\sa threading::locked_ptr
	*/
	template<typename _type, class _tMutex = std::mutex,
		class _tLock = std::unique_lock<_tMutex>>
		using locked_ptr = threading::locked_ptr<_type, _tMutex, _tLock>;


	/*!
	\brief ת�� \c thread::id Ϊ�ַ�����ʾ��
	\note ʹ�� \c operator<< ʵ�֡�
	*/
	WB_API std::string
		to_string(const std::thread::id&);

	/*!
	\brief ȡ��ǰ�̱߳�ʶ���ַ�����ʾ��
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
	\brief �̳߳ء�
	\note ��������Լ�������й�����Ա�����̰߳�ȫ��
	\note δ�����̶߳��еĳ��ȡ�
	*/
	class WB_API thread_pool
	{
	private:
		std::vector<std::thread> workers;
		std::queue<std::packaged_task<void()>> tasks{};
		mutable std::mutex queue_mutex{};
		std::condition_variable condition{};
		/*!
		\brief ֹͣ״̬��
		\note ��������ʱ����ֹͣ��
		*/
		bool stopped = {};

	public:
		/*!
		\brief ��ʼ����ʹ��ָ���ĳ�ʼ�����˳��ص�ָ�������Ĺ����̡߳�
		\note ���ص�Ϊ������ԡ�
		\warning �ص���ִ�в��ṩ˳��Ͳ�����ȫ��֤��
		*/
		thread_pool(size_t, std::function<void()> = {}, std::function<void()> = {});
		/*!
		\brief ����������ֹͣ״̬���ȴ�����ִ���е��߳̽�����
		\note ��������ֹͣ״̬ʱ���׳� std::system_error ��
		\note ��������������ÿ���߳������� std::system_error ��
		\note ���쳣�׳������ԡ�
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
		\warning ���̰߳�ȫ��
		*/
		size_t
			size_unlocked() const wnothrow
		{
			return tasks.size();
		}

		/*!
		\brief �ȴ�����������С�
		\param f ׼��������еĲ���
		\param args ������в���ʱ�Ĳ�����
		\param waiter �ȴ�������
		\pre waiter ���ú��������������������������ԣ���������
		\return ������״̬�����ȴ�ʧ������Ч����
		\warning ��Ҫȷ��δ��ֹͣ��δ���������������򲻱�֤�������С�
		\warning ʹ�÷ǵݹ������ȴ�ʱ�����ٴ�������
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
	\brief ����أ����ж��д�С���Ƶ��̳߳ء�
	\note ��������Լ�������й�����Ա�����̰߳�ȫ��
	\todo ����������д�С���ơ�
	*/
	class WB_API task_pool : private thread_pool
	{
	private:
		size_t max_tasks;
		std::condition_variable enqueue_condition{};

	public:
		/*!
		\brief ��ʼ����ʹ��ָ���ĳ�ʼ�����˳��ص�ָ�������Ĺ����̺߳������������
		\sa thread_pool::thread_pool
		*/
		task_pool(size_t n, std::function<void()> on_enter = {},
			std::function<void()> on_exit = {})
			: thread_pool(std::max<size_t>(n, 1), on_enter, on_exit),
			max_tasks(std::max<size_t>(n, 1))
		{}

		//! \warning ���̰߳�ȫ��
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
		\brief �����̳߳ء�
		\note �����ȴ���ǰ����������ɺ����´�����
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