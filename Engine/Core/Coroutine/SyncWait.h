#pragma once

#include "sync_wait_task.h"

namespace white::coroutine {
	template<typename AWAITABLE>
	auto SyncWait(AWAITABLE&& awaitable)
	{
		auto task = details::make_sync_wait_task<AWAITABLE>(awaitable);

		white::threading::manual_reset_event event;

		task.start(event);
		event.wait();
		return task.result();
	}
}