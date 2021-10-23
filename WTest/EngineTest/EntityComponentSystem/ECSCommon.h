#ifndef FrameWork_ECS_COMMON_h
#define FrameWork_ECS_COMMON_h 1

#include <WBase/winttype.hpp>
#include <WFramework/WCLib/Debug.h>

namespace ecs {
	class Entity;
	class System;
	struct Component;

	using EntityId = white::uint32;

	using namespace platform::Descriptions;
}


#endif
