#pragma once

#include <WBase/wdef.h>
#include <atomic>

namespace WhiteEngine
{
	class SpinMutex
	{
	public:

		/// Initialise the mutex to the unlocked state.
		SpinMutex() noexcept;

		/// Attempt to lock the mutex without blocking
		///
		/// \return
		/// true if the lock was acquired, false if the lock was already held
		/// and could not be immediately acquired.
		bool try_lock() noexcept;

		/// Block the current thread until the lock is acquired.
		///
		/// This will busy-wait until it acquires the lock.
		///
		/// This has 'acquire' memory semantics and synchronises
		/// with prior calls to unlock().
		void lock() noexcept;

		/// Release the lock.
		///
		/// This has 'release' memory semantics and synchronises with
		/// lock() and try_lock().
		void unlock() noexcept;

	private:
		std::atomic<bool> locked;
	};
}