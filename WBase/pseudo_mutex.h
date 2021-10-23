/*! \file pseudo_mutex.h
\ingroup WBase
\brief 伪互斥量。
*/

#ifndef WBase_pseudo_mutex_h
#define WBase_pseudo_mutex_h 1

#include "sutility.h" // for noncopyable, nonmovable, std::declval;
#include "exception.h" // for throw_error, std::errc;
#include "ref.hpp"
#include <chrono> // for std::chrono::duration, std::chrono::time_point;

namespace white
{
	namespace threading
	{
		/*!
		\see ISO C++14 30.4.2[thread.lock] 。
		*/
		//@{
		wconstexpr const struct defer_lock_t
		{} defer_lock{};

		wconstexpr const struct try_to_lock_t
		{} try_to_lock{};

		wconstexpr const struct adopt_lock_t
		{} adopt_lock{};
		//@}

		/*!
		\note 第一模板参数不需要保证满足 BasicLockable 要求。
		\warning 非虚析构。
		*/
		//@{
		//! \pre _tReference 值的 get() 返回的类型满足 BasicLockable 要求。
		template<class _type, typename _tReference = lref<_type>>
		class lockable_adaptor
		{
		public:
			//! \since build 722
			using holder_type = _type;
			using reference = _tReference;

		private:
			reference ref;

		public:
			//@{
			lockable_adaptor(holder_type& m) wnothrow
				: ref(m)
			{}

			explicit
				operator holder_type&() wnothrow
			{
				return ref;
			}
			explicit
				operator const holder_type&() const wnothrow
			{
				return ref;
			}

			void
				lock()
			{
				ref.get().lock();
			}

			//! \pre holder_type 满足 Lockable 要求。
			bool
				try_lock()
			{
				return ref.get().try_lock();
			}

			//! \pre holder_type 满足 TimedLockable 要求。
			//@{
			template<typename _tRep, typename _tPeriod>
			bool
				try_lock_for(const std::chrono::duration<_tRep, _tPeriod>& rel_time)
			{
				return ref.get().try_lock_for(rel_time);
			}

			template<typename _tClock, typename _tDuration>
			bool
				try_lock_until(const std::chrono::time_point<_tClock, _tDuration>& abs_time)
			{
				return ref.get().try_lock_until(abs_time);
			}
			//@}

			void
				unlock() wnothrowv
			{
				ref.get().unlock();
			}
			//@}
		};


		//! \pre _tReference 值的 get() 返回的类型满足共享 BasicLockable 要求。
		template<class _type, typename _tReference = lref<_type>>
		class shared_lockable_adaptor
		{
		public:
			using holder_type = _type;
			using reference = _tReference;

		private:
			reference ref;

		public:
			shared_lockable_adaptor(holder_type& m) wnothrow
				: ref(m)
			{}

			explicit
				operator holder_type&() wnothrow
			{
				return ref;
			}
			explicit
				operator const holder_type&() const wnothrow
			{
				return ref;
			}

			//@{
			void
				lock()
			{
				ref.get().lock_shared();
			}

			//! \pre holder_type 满足 Lockable 要求。
			bool
				try_lock()
			{
				return ref.get().try_lock_shared();
			}

			//! \pre holder_type 满足 TimedLockable 要求。
			//@{
			template<typename _tRep, typename _tPeriod>
			bool
				try_lock_for(const std::chrono::duration<_tRep, _tPeriod>& rel_time)
			{
				return ref.get().try_lock_shared_for(rel_time);
			}

			template<typename _tClock, typename _tDuration>
			bool
				try_lock_until(const std::chrono::time_point<_tClock, _tDuration>& abs_time)
			{
				return ref.get().try_lock_shared_until(abs_time);
			}
			//@}

			void
				unlock() wnothrow
			{
				ref.get().unlock_shared();
			}
			//@}
		};
		//@}


		//@{
		/*!
		\note 第一参数为存储的互斥量类型。
		\note 第二参数决定是否进行检查，不检查时优化实现为空操作。
		\warning 不检查时不保证线程安全：多线程环境下假定线程同步语义可能引起未定义行为。
		*/
		//@{
		/*!
		\pre _tReference 满足 BasicLockable 要求。
		\pre 静态断言： _tReference 使用 _tMutex 左值初始化保证无异常抛出。
		\note 第三参数为调用的引用类型。
		\warning 非虚析构。
		*/
		//@{
		template<class _tMutex, bool = true, typename _tReference = _tMutex&>
		class lock_guard : private wimpl(noncopyable), private wimpl(nonmovable)
		{
			wnoexcept_assert("Invalid type found.",
				_tReference(std::declval<_tMutex&>()));

		public:
			using mutex_type = _tMutex;
			using reference = _tReference;

		private:
			mutex_type& owned;

		public:
			/*!
			\pre 若 mutex_type 非递归锁，调用线程不持有锁。
			\post <tt>std::addressof(owned) == std::addressof(m)</tt> 。
			*/
			explicit
				lock_guard(mutex_type& m)
				: owned(m)
			{
				reference(owned).lock();
			}
			/*!
			\pre 调用线程持有锁。
			\post <tt>std::addressof(owned) == std::addressof(m)</tt> 。
			*/
			lock_guard(mutex_type& m, adopt_lock_t) wimpl(wnothrow)
				: owned(m)
			{}
			~lock_guard()
			{
				reference(owned).unlock();
			}
		};

		template<class _tMutex, typename _tReference>
		class lock_guard<_tMutex, false, _tReference>
			: private wimpl(noncopyable), private wimpl(nonmovable)
		{
		public:
			using mutex_type = _tMutex;

			explicit
				lock_guard(mutex_type&) wimpl(wnothrow)
			{}
			lock_guard(mutex_type&, adopt_lock_t) wimpl(wnothrow)
			{}
		};


		//! \brief 锁基类：可适配独占锁和共享锁。
		//@{
		template<class _tMutex, bool _bEnableCheck = true, class _tReference = _tMutex&>
		class lock_base : private noncopyable
		{
			wnoexcept_assert("Invalid type found.",
				_tReference(std::declval<_tMutex&>()));

		public:
			using mutex_type = _tMutex;
			using reference = _tReference;

		private:
			mutex_type* pm;
			bool owns;

		public:
			lock_base() wnothrow
				: pm(), owns()
			{}

			//! \post <tt>pm == std::addressof(m)</tt> 。
			//@{
			explicit
				lock_base(mutex_type& m)
				: lock_base(m, defer_lock)
			{
				lock();
				owns = true;
			}
			lock_base(mutex_type& m, defer_lock_t) wnothrow
				: pm(std::addressof(m)), owns()
			{}
			/*!
			\pre mutex_type 满足 Lockable 要求。
			\pre 若 mutex_type 非递归锁，调用线程不持有锁。
			*/
			lock_base(mutex_type& m, try_to_lock_t)
				: pm(std::addressof(m)), owns(get_ref().try_lock())
			{}
			//! \pre 调用线程持有锁。
			lock_base(mutex_type& m, adopt_lock_t) wimpl(wnothrow)
				: pm(std::addressof(m)), owns(true)
			{}
			/*!
			\pre mutex_type 满足 TimedLockable 要求。
			\pre 若 mutex_type 非递归锁，调用线程不持有锁。
			*/
			//@{
			template<typename _tClock, typename _tDuration>
			lock_base(mutex_type& m, const std::chrono::time_point<_tClock,
				_tDuration>& abs_time)
				: pm(std::addressof(m)), owns(get_ref().try_lock_until(abs_time))
			{}
			template<typename _tRep, typename _tPeriod>
			lock_base(mutex_type& m,
				const std::chrono::duration<_tRep, _tPeriod>& rel_time)
				: pm(std::addressof(m)), owns(get_ref().try_lock_for(rel_time))
			{}
			//@}
			//@}
			/*!
			\post <tt>pm == u_p.pm && owns == u_p.owns</tt> 当 \c u_p 是 u 之前的状态。
			\post <tt>!u.pm && !u.owns</tt> 。
			*/
			lock_base(lock_base&& u) wnothrow
				: pm(u.pm), owns(u.owns)
			{
				wunseq(u.pm = {}, u.owns = {});
			}
			~lock_base()
			{
				if (owns)
					unlock();
			}

			/*!
			\post <tt>pm == u_p.pm && owns == u_p.owns</tt> 当 \c u_p 是 u 之前的状态。
			\post <tt>!u.pm && !u.owns</tt> 。
			\see LWG 2104 。
			*/
			lock_base&
				operator=(lock_base&& u) wimpl(wnothrow)
			{
				if (owns)
					unlock();
				swap(u, *this);
				u.clear_members();
				return *this;
			}

			explicit
				operator bool() const wnothrow
			{
				return owns;
			}

		private:
			void
				check_lock()
			{
				using namespace std;

				if (!pm)
					throw_error(errc::operation_not_permitted);
				if (owns)
					throw_error(errc::resource_deadlock_would_occur);
			}

			void
				clear_members() wimpl(wnothrow)
			{
				wunseq(pm = {}, owns = {});
			}

			reference
				get_ref() wnothrow
			{
				wassume(pm);
				return reference(*pm);
			}

		public:
			void
				lock()
			{
				check_lock();
				get_ref().lock();
				owns = true;
			}

			//! \pre reference 满足 Lockable 要求。
			bool
				try_lock()
			{
				check_lock();
				return owns = get_ref().try_lock();
			}

			//! \pre reference 满足 TimedLockable 要求。
			//@{
			template<typename _tRep, typename _tPeriod>
			bool
				try_lock_for(const std::chrono::duration<_tRep, _tPeriod>& rel_time)
			{
				check_lock();
				return owns = get_ref().try_lock_for(rel_time);
			}

			template<typename _tClock, typename _tDuration>
			bool
				try_lock_until(const std::chrono::time_point<_tClock, _tDuration>& abs_time)
			{
				check_lock();
				return owns = get_ref().try_lock_until(abs_time);
			}
			//@}

			void
				unlock()
			{
				if (!owns)
					throw_error(std::errc::operation_not_permitted);
				if (pm)
				{
					get_ref().unlock();
					owns = {};
				}
			}

			friend void
				swap(lock_base& x, lock_base& y) wnothrow
			{
				std::swap(x.pm, y.pm),
					std::swap(x.owns, y.owns);
			}

			mutex_type*
				release() wnothrow
			{
				const auto res(pm);

				clear_members();
				return res;
			}

			bool
				owns_lock() const wnothrow
			{
				return owns;
			}

			mutex_type*
				mutex() const wnothrow
			{
				return pm;
			}
		};

		template<class _tMutex, class _tReference>
		class lock_base<_tMutex, false, _tReference>
		{
		public:
			using mutex_type = _tMutex;

			lock_base() wnothrow wimpl(= default);
			explicit
				lock_base(mutex_type&) wimpl(wnothrow)
			{}
			lock_base(mutex_type&, defer_lock_t) wnothrow
			{}
			lock_base(mutex_type&, try_to_lock_t) wimpl(wnothrow)
			{}
			lock_base(mutex_type&, adopt_lock_t) wimpl(wnothrow)
			{}
			template<typename _tClock, typename _tDuration>
			lock_base(mutex_type&,
				const std::chrono::time_point<_tClock, _tDuration>&) wimpl(wnothrow)
			{}
			template<typename _tRep, typename _tPeriod>
			lock_base(mutex_type&, const std::chrono::duration<_tRep, _tPeriod>&)
				wimpl(wnothrow)
			{}
			lock_base(lock_base&&) wimpl(= default);

			lock_base&
				operator=(lock_base&&) = wimpl(default);

			explicit
				operator bool() const wnothrow
			{
				return true;
			}

			void
				lock()
			{}

			bool
				try_lock() wimpl(wnothrow)
			{
				return true;
			}

			template<typename _tRep, typename _tPeriod>
			bool
				try_lock_for(const std::chrono::duration<_tRep, _tPeriod>&) wimpl(wnothrow)
			{
				return true;
			}

			template<typename _tClock, typename _tDuration>
			bool
				try_lock_until(const std::chrono::time_point<_tClock, _tDuration>&)
				wimpl(wnothrow)
			{
				return true;
			}

			void
				unlock() wimpl(wnothrow)
			{}

			friend void
				swap(lock_base&, lock_base&) wnothrow
			{}

			mutex_type*
				release() wnothrow
			{
				return {};
			}

			bool
				owns_lock() const wnothrow
			{
				return true;
			}

			mutex_type*
				mutex() const wnothrow
			{
				return {};
			}
		};
		//@}
		//@}


		//! \warning 非虚析构。
		template<class _tMutex, bool _bEnableCheck = true>
		class unique_lock : private wimpl(lock_base<_tMutex, _bEnableCheck>)
		{
		private:
			using base = wimpl(lock_base<_tMutex, _bEnableCheck>);

		public:
			using mutex_type = _tMutex;

		public:
			unique_lock() wnothrow wimpl(= default);
			using wimpl(base::base);
			unique_lock(unique_lock&&) wimpl(= default);

			/*!
			\post <tt>pm == u_p.pm && owns == u_p.owns</tt> 当 \c u_p 是 u 之前的状态。
			\post <tt>!u.pm && !u.owns</tt> 。
			\see LWG 2104 。
			\since build 722
			*/
			unique_lock&
				operator=(unique_lock&&) wimpl(= default);

			using wimpl(base)::operator bool;

			using wimpl(base)::lock;

			//! \pre mutex_type 满足 Lockable 要求。
			using wimpl(base)::try_lock;

			//! \pre mutex_type 满足 TimedLockable 要求。
			//@{
			using wimpl(base)::try_lock_for;

			using wimpl(base)::try_lock_until;
			//@}

			using wimpl(base)::unlock;

			void
				swap(unique_lock& u) wnothrow
			{
				swap(static_cast<base&>(*this), static_cast<base&>(u));
			}

			using wimpl(base)::release;

			using wimpl(base)::owns_lock;

			using wimpl(base)::mutex;
		};

		//! \relates unique_lock
		template<class _tMutex, bool _bEnableCheck>
		inline void
			swap(unique_lock<_tMutex, _bEnableCheck>& x,
				unique_lock<_tMutex, _bEnableCheck>& y) wnothrow
		{
			x.swap(y);
		}


		//! \warning 非虚析构。
		template<class _tMutex, bool _bEnableCheck = true>
		class shared_lock : private wimpl(lock_base<_tMutex, _bEnableCheck,
			shared_lockable_adaptor<_tMutex>>)
		{
		private:
			using base = wimpl(lock_base<_tMutex, _bEnableCheck,
				shared_lockable_adaptor<_tMutex>>);

		public:
			using mutex_type = _tMutex;

		public:
			shared_lock() wnothrow wimpl(= default);
			using wimpl(base::base);
			shared_lock(shared_lock&&) wimpl(= default);

			/*!
			\post <tt>pm == u_p.pm && owns == u_p.owns</tt> 当 \c u_p 是 u 之前的状态。
			\post <tt>!u.pm && !u.owns</tt> 。
			\see LWG 2104 。
			*/
			shared_lock&
				operator=(shared_lock&&) wimpl(= default);

			using wimpl(base)::operator bool;

			//@{
			void
				lock()
			{
				base::lock_shared();
			}

			//! \pre mutex_type 满足 Lockable 要求。
			bool
				try_lock()
			{
				return base::try_lock_shared();
			}

			//! \pre mutex_type 满足 TimedLockable 要求。
			//@{
			template<typename _tRep, typename _tPeriod>
			bool
				try_lock_shared_for(const std::chrono::duration<_tRep, _tPeriod>& rel_time)
			{
				return base::template try_lock_shared_for(rel_time);
			}

			template<typename _tClock, typename _tDuration>
			bool
				try_lock_shared_until(const
					std::chrono::time_point<_tClock, _tDuration>& abs_time)
			{
				return base::template try_lock_shared_until(abs_time);
			}
			//@}
			//@}

			using wimpl(base)::unlock;

			void
				swap(shared_lock& u) wnothrow
			{
				swap(static_cast<base&>(*this), static_cast<base&>(u));
			}

			using wimpl(base)::release;

			using wimpl(base)::owns_lock;

			using wimpl(base)::mutex;
		};

		//! \relates shared_lock
		template<class _tMutex, bool _bEnableCheck>
		inline void
			swap(shared_lock<_tMutex, _bEnableCheck>& x,
				shared_lock<_tMutex, _bEnableCheck>& y) wnothrow
		{
			x.swap(y);
		}
		//@}
		//@}

	} // namespace threading;

	/*!
	\brief 单线程操作：保证单线程环境下接口及符合对应的 std 命名空间下的接口。
	\note 不包含本机类型相关的接口。
	\since build 1.3
	\todo 添加 ISO C++ 14 共享锁。
	*/
	namespace single_thread
	{

		//! \since build 1.3
		//@{
		wconstexpr const struct defer_lock_t
		{} defer_lock{};

		wconstexpr const struct try_to_lock_t
		{} try_to_lock{};

		wconstexpr const struct adopt_lock_t
		{} adopt_lock{};


		//! \warning 不保证线程安全：多线程环境下假定线程同步语义可能引起未定义行为。
		//@{
		//! \see ISO C++11 [thread.mutex.requirements.mutex] 。
		//@{
		class WB_API mutex : private wimpl(noncopyable), private wimpl(nonmovable)
		{
		public:
			wconstfn
				mutex() wimpl(= default);
			~mutex() wimpl(= default);

			//! \pre 调用线程不持有锁。
			void
				lock() wimpl(wnothrow)
			{}

			bool
				try_lock() wimpl(wnothrow)
			{
				return true;
			}

			//! \pre 调用线程持有锁。
			void
				unlock() wimpl(wnothrow)
			{}
		};


		class WB_API recursive_mutex
			: private wimpl(noncopyable), private wimpl(nonmovable)
		{
		public:
			recursive_mutex() wimpl(= default);
			~recursive_mutex() wimpl(= default);

			void
				lock() wimpl(wnothrow)
			{}

			bool
				try_lock() wimpl(wnothrow)
			{
				return true;
			}

			//! \pre 调用线程持有锁。
			void
				unlock() wimpl(wnothrow)
			{}
		};


		//! \see ISO C++11 [thread.timedmutex.requirements] 。
		//@{
		class WB_API timed_mutex : private wimpl(noncopyable), private wimpl(nonmovable)
		{
		public:
			timed_mutex() wimpl(= default);
			~timed_mutex() wimpl(= default);

			//! \pre 调用线程不持有锁。
			//@{
			void
				lock() wimpl(wnothrow)
			{}

			bool
				try_lock() wimpl(wnothrow)
			{
				return true;
			}

			template<typename _tRep, typename _tPeriod>
			bool
				try_lock_for(const std::chrono::duration<_tRep, _tPeriod>&) wimpl(wnothrow)
			{
				return true;
			}

			template<typename _tClock, typename _tDuration>
			bool
				try_lock_until(const std::chrono::time_point<_tClock, _tDuration>&)
				wimpl(wnothrow)
			{
				return true;
			}
			//@}

			//! \pre 调用线程持有锁。
			void
				unlock() wimpl(wnothrow)
			{}
		};


		class WB_API recursive_timed_mutex
			: private wimpl(noncopyable), private wimpl(nonmovable)
		{
		public:
			recursive_timed_mutex() wimpl(= default);
			~recursive_timed_mutex() wimpl(= default);

			void
				lock() wimpl(wnothrow)
			{}

			bool
				try_lock() wimpl(wnothrow)
			{
				return true;
			}

			template<typename _tRep, typename _tPeriod>
			bool
				try_lock_for(const std::chrono::duration<_tRep, _tPeriod>&) wimpl(wnothrow)
			{
				return true;
			}

			template<typename _tClock, typename _tDuration>
			bool
				try_lock_until(const std::chrono::time_point<_tClock, _tDuration>&)
				wimpl(wnothrow)
			{
				return true;
			}

			//! \pre 调用线程持有锁。
			void
				unlock() wimpl(wnothrow)
			{}
		};
		//@}
		//@}


		template<class _tMutex>
		class lock_guard : private wimpl(noncopyable), private wimpl(nonmovable)
		{
		public:
			using mutex_type = _tMutex;

#ifdef NDEBUG
			explicit
				lock_guard(mutex_type&) wimpl(wnothrow)
			{}
			lock_guard(mutex_type&, adopt_lock_t) wimpl(wnothrow)
			{}

#else
		private:
			mutex_type& pm;

		public:
			/*!
			\pre 若 mutex_type 非递归锁，调用线程不持有锁。
			\post <tt>pm == &m</tt> 。
			*/
			explicit
				lock_guard(mutex_type& m) wimpl(wnothrow)
				: pm(m)
			{
				m.lock();
			}
			/*!
			\pre 调用线程持有锁。
			\post <tt>pm == &m</tt> 。
			*/
			lock_guard(mutex_type& m, adopt_lock_t) wimpl(wnothrow)
				: pm(&m)
			{}
			~lock_guard() wimpl(wnothrow)
			{
				pm.unlock();
			}
#endif
		};


		//! \note 定义宏 \c NDEBUG 时不进行检查，优化实现为空操作。
		//@{
		template<class _tMutex>
		class unique_lock : private wimpl(noncopyable)
		{
		public:
			using mutex_type = _tMutex;

#ifdef NDEBUG
			//! \since build 1.4
			//@{
			unique_lock() wimpl(= default);
			explicit
				unique_lock(mutex_type&) wimpl(wnothrow)
			{}
			//! \since build 1.4
			unique_lock(mutex_type&, defer_lock_t) wnothrow
			{}
			unique_lock(mutex_type&, try_to_lock_t) wimpl(wnothrow)
			{}
			unique_lock(mutex_type&, adopt_lock_t) wimpl(wnothrow)
			{}
			template<typename _tClock, typename _tDuration>
			unique_lock(mutex_type&,
				const std::chrono::time_point<_tClock, _tDuration>&) wimpl(wnothrow)
			{}
			template<typename _tRep, typename _tPeriod>
			unique_lock(mutex_type&, const std::chrono::duration<_tRep, _tPeriod>&)
				wimpl(wnothrow)
			{}
			unique_lock(unique_lock&&) wimpl(= default);

			explicit
				operator bool() const wnothrow
			{
				return true;
			}

			void
				lock()
			{}

			bool
				owns_lock() const wnothrow
			{
				return true;
			}

			mutex_type*
				release() wnothrow
			{
				return{};
			}

			void
				swap(unique_lock&) wnothrow
			{}

			bool
				try_lock() wimpl(wnothrow)
			{
				return true;
			}

			template<typename _tRep, typename _tPeriod>
			bool
				try_lock_for(const std::chrono::duration<_tRep, _tPeriod>&) wimpl(wnothrow)
			{
				return true;
			}

			template<typename _tClock, typename _tDuration>
			bool
				try_lock_until(const std::chrono::time_point<_tClock, _tDuration>&)
				wimpl(wnothrow)
			{
				return true;
			}

			void
				unlock() wimpl(wnothrow)
			{}

			mutex_type*
				mutex() const wnothrow
			{
				return{};
			}
			//@}
#else
		private:
			mutex_type* pm;
			bool owns;

		public:
			unique_lock() wnothrow
				: pm(), owns()
			{}

			//! \post <tt>pm == &m</tt> 。
			//@{
			explicit
				unique_lock(mutex_type& m) wimpl(wnothrow)
				: unique_lock(m, defer_lock)
			{
				lock();
				owns = true;
			}
			unique_lock(mutex_type& m, defer_lock_t) wnothrow
				: pm(&m), owns()
			{}
			/*!
			\pre mutex_type 满足 Lockable 要求。
			\pre 若 mutex_type 非递归锁，调用线程不持有锁。
			*/
			unique_lock(mutex_type& m, try_to_lock_t) wimpl(wnothrow)
				: pm(&m), owns(pm->try_lock())
			{}
			//! \pre 调用线程持有锁。
			unique_lock(mutex_type& m, adopt_lock_t) wimpl(wnothrow)
				: pm(&m), owns(true)
			{}
			/*!
			\pre mutex_type 满足 TimedLockable 要求。
			\pre 若 mutex_type 非递归锁，调用线程不持有锁。
			*/
			//@{
			template<typename _tClock, typename _tDuration>
			unique_lock(mutex_type& m, const std::chrono::time_point<_tClock,
				_tDuration>& abs_time) wimpl(wnothrow)
				: pm(&m), owns(pm->try_lock_until(abs_time))
			{}
			template<typename _tRep, typename _tPeriod>
			unique_lock(mutex_type& m,
				const std::chrono::duration<_tRep, _tPeriod>& rel_time) wimpl(wnothrow)
				: pm(&m), owns(pm->try_lock_for(rel_time))
			{}
			//@}
			//@}
			/*!
			\post <tt>pm == u_p.pm && owns == u_p.owns</tt> 当 \c u_p 是 u 之前的状态。
			\post <tt>!u.pm && !u.owns</tt> 。
			*/
			unique_lock(unique_lock&& u) wnothrow
				: pm(u.pm), owns(u.owns)
			{
				wunseq(u.pm = {}, u.owns = {});
			}
			~unique_lock()
			{
				if (owns)
					unlock();
			}

			/*!
			\post <tt>pm == u_p.pm && owns == u_p.owns</tt> 当 \c u_p 是 u 之前的状态。
			\post <tt>!u.pm && !u.owns</tt> 。
			\see http://wg21.cmeerw.net/lwg/issue2104 。
			*/
			unique_lock&
				operator=(unique_lock&& u) wimpl(wnothrow)
			{
				if (owns)
					unlock();
				unique_lock(std::move(u)).swap(*this);
				u.clear_members();
				return *this;
			}

			explicit
				operator bool() const wnothrow
			{
				return owns;
			}

		private:
			void
				check_lock() wimpl(wnothrow)
			{
				using namespace std;

				if (!pm)
					throw_error(errc::operation_not_permitted);
				if (owns)
					throw_error(errc::resource_deadlock_would_occur);
			}

			void
				clear_members() wimpl(wnothrow)
			{
				wunseq(pm = {}, owns = {});
			}

		public:
			void
				lock() wimpl(wnothrow)
			{
				check_lock();
				pm->lock();
				owns = true;
			}

			bool
				owns_lock() const wnothrow
			{
				return owns;
			}

			mutex_type*
				release() wnothrow
			{
				const auto res(pm);

				clear_members();
				return res;
			}

			void
				swap(unique_lock& u) wnothrow
			{
				std::swap(pm, u.pm),
					std::swap(owns, u.owns);
			}

			//! \pre mutex_type 满足 Lockable 要求。
			bool
				try_lock() wimpl(wnothrow)
			{
				check_lock();
				return owns = pm->lock();
			}

			//! \pre mutex_type 满足 TimedLockable 要求。
			//@{
			template<typename _tRep, typename _tPeriod>
			bool
				try_lock_for(const std::chrono::duration<_tRep, _tPeriod>& rel_time)
				wimpl(wnothrow)
			{
				check_lock();
				return owns = pm->try_lock_for(rel_time);
			}

			template<typename _tClock, typename _tDuration>
			bool
				try_lock_until(const std::chrono::time_point<_tClock, _tDuration>& abs_time)
				wimpl(wnothrow)
			{
				check_lock();
				return owns = pm->try_lock_until(abs_time);
			}
			//@}

			void
				unlock() wimpl(wnothrow)
			{
				if (!owns)
					throw_error(std::errc::operation_not_permitted);
				if (pm)
				{
					pm->unlock();
					owns = {};
				}
			}

			mutex_type*
				mutex() const wnothrow
			{
				return pm;
			}
#endif
		};


		//! \since build 1.4
		struct WB_API once_flag : private wimpl(noncopyable), private wimpl(nonmovable)
		{
			wimpl(bool) state = {};

			wconstfn
				once_flag() wnothrow wimpl(= default);
		};


		/*!
		\brief 按标识调用函数，保证调用一次。
		\note 类似 std::call_once ，但不保证线程安全性。
		\note ISO C++11（至 N3691 ） 30.4 synopsis 处的声明存在错误。
		\bug 未实现支持成员指针。
		\see https://github.com/cplusplus/draft/issues/151 。
		\see http://wg21.cmeerw.net/cwg/issue1591 。
		\see http://wg21.cmeerw.net/cwg/issue2442 。
		\since build 1.4

		当标识为非初值时候无作用，否则调用函数。
		*/
		template<typename _fCallable, typename... _tParams>
		inline void
			call_once(once_flag& flag, _fCallable&& f, _tParams&&... args)
		{
			if (!flag.state)
			{
				f(wforward(args)...);
				flag.state = true;
			}
		}
		//@}

		//! \relates unique_lock
		template<class _tMutex>
		inline void
			swap(unique_lock<_tMutex>& x, unique_lock<_tMutex>& y) wnothrow
		{
			x.swap(y);
		}


		template<class _tLock1, class _tLock2, class... _tLocks>
		void
			lock(_tLock1&&, _tLock2&&, _tLocks&&...) wimpl(wnothrow)
		{}

		template<class _tLock1, class _tLock2, class... _tLocks>
		wconstfn int
			try_lock(_tLock1&&, _tLock2&&, _tLocks&&...) wimpl(wnothrow)
		{
			return -1;
		}
		//@}
		//@}

	} // namespace single_thread;


	  /*!
	  \brief 在单线程环境和多线程环境下都可用的线程同步接口。
	  \since build 1.3
	  */
	namespace threading
	{

		/*!
		\brief 解锁删除器。
		\pre _tMutex 满足 \c BasicLockable 要求。
		\since build 1.4
		*/
		template<class _tMutex = single_thread::mutex,
			class _tLock = single_thread::unique_lock<_tMutex>>
			class unlock_delete : private noncopyable
		{
		public:
			using mutex_type = _tMutex;
			using lock_type = _tLock;

			mutable lock_type lock;

			unlock_delete(mutex_type& mtx)
				: lock(mtx)
			{}
			template<typename
				= wimpl(enable_if_t)<is_nothrow_move_constructible<lock_type>::value>>
				unlock_delete(lock_type&& lck) wnothrow
				: lock(std::move(lck))
			{}
			template<typename... _tParams>
			unlock_delete(mutex_type& mtx, _tParams&&... args) wnoexcept(
				std::declval<mutex_type&>()(std::declval<_tParams&&>()...))
				: lock(mtx, wforward(args)...)
			{}

			//! \brief 删除：解锁。
			template<typename _tPointer>
			void
				operator()(const _tPointer&) const wnothrow
			{
				lock.unlock();
			}
		};


		/*!
		\brief 独占所有权的锁定指针。
		\since build 1.3
		*/
		template<typename _type, class _tMutex = single_thread::mutex,
			class _tLock = single_thread::unique_lock<_tMutex>>
			using locked_ptr = std::unique_ptr<_type, unlock_delete<_tMutex, _tLock>>;
	} // namespace threading;

} // namespace white;

#endif