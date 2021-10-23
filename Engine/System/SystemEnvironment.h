/*! \file System\SystemEnvironment.h
\ingroup Engine
\brief 全局环境，包含经常用接口的指针。
*/

#ifndef WE_System_Environment_H
#define WE_System_Environment_H 1

#include "NinthTimer.h"
#include "../Core/Threading/TaskScheduler.h"
#include <WBase/wmemory.hpp>

namespace WhiteEngine::System {
	struct GlobalEnvironment {
		platform::chrono::NinthTimer* Timer;

		white::threading::TaskScheduler* Scheduler;

		float Gamma;
	};

	[[nodiscard]]
	std::shared_ptr<void> InitGlobalEnvironment();
}

extern WhiteEngine::System::GlobalEnvironment* Environment;

#endif