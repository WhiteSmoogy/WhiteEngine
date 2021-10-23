/*!	\file Mutex.h
\ingroup Framework
\brief ª•≥‚¡ø°£
*/

#ifndef FrameWork_Mutex_h
#define FrameWork_Mutex_h 1

#include <WFramework/WCLib/FCommon.h>
#include <WBase/type_op.hpp>
#if WF_Multithread == 1
#include <atomic>
#include <WBase/concurrency.h>
#define WFL_Mutex_Space std
#define WFL_Threading_Space white
#else
#include <WBase/pseudo_mutex.h>
#define WFL_Mutex_Space white::single_thread
#define WFL_Threading_Space white
#endif

namespace platform {
	namespace Concurrency {
		template<typename _type>
		using atomic = white::cond_or_t<white::or_<std::is_integral<_type>,
			std::is_pointer<_type>>, void, std::atomic, _type>;

		using WFL_Mutex_Space::mutex;
		using WFL_Mutex_Space::recursive_mutex;

		using WFL_Mutex_Space::lock_guard;
		using WFL_Mutex_Space::unique_lock;
		
		using white::threading::lockable_adaptor;
		using white::threading::shared_lockable_adaptor;

#if WF_Multithread == 1 || !defined(NDEBUG)
		wconstfn bool UseLockDebug(true);
#else
		wconstfn bool UseLockDebug = {};
#endif

		template<class _tMutex>
		using shared_lock = white::threading::shared_lock<_tMutex, UseLockDebug>;

		template<class _tMutex>
		using shared_lock_guard = white::threading::lock_guard<_tMutex,
			UseLockDebug, shared_lockable_adaptor<_tMutex>>;

		using WFL_Mutex_Space::lock;
		using WFL_Mutex_Space::try_lock;

		using WFL_Mutex_Space::once_flag;
		using WFL_Mutex_Space::call_once;

		template<class _type, typename _tReference = white::lref<_type>>
		using AdaptedLock = white::threading::lock_base<_type, UseLockDebug,
			lockable_adaptor<_type, _tReference>>;

		template<class _type, typename _tReference = white::lref<_type>>
		using SharedAdaptedLock = white::threading::lock_base<_type, UseLockDebug,
			shared_lockable_adaptor<_type, _tReference>>;

		template<class _type, typename _tReference = white::lref<_type>>
		using AdaptedLockGuard = white::threading::lock_guard<_type, UseLockDebug,
			lockable_adaptor<_type, _tReference>>;

		template<class _type, typename _tReference = white::lref<_type>>
		using SharedAdaptedLockGuard = white::threading::lock_guard<_type,
			UseLockDebug, shared_lockable_adaptor<_type, _tReference>>;

		template<class _type>
		using IndirectLock = AdaptedLock<_type, white::indirect_ref_adaptor<_type>>;

		template<class _type>
		using SharedIndirectLock
			= SharedAdaptedLock<_type, white::indirect_ref_adaptor<_type>>;

		template<class _type>
		using IndirectLockGuard
			= AdaptedLockGuard<_type, white::indirect_ref_adaptor<_type>>;

		template<class _type>
		using SharedIndirectLockGuard
			= SharedAdaptedLockGuard<_type, white::indirect_ref_adaptor<_type>>;
	}

	namespace Threading{
		using WFL_Threading_Space::unlock_delete;
		using WFL_Threading_Space::locked_ptr;

	} // namespace Threading;
}

#undef WFL_Mutex_Space
#undef WFL_Threading_Space

#endif