#ifndef FrameWork_ECS_Scene_SYSTEM_h
#define FrameWork_ECS_Scene_SYSTEM_h 1

#include <WBase/typeinfo.h>
#include <WBase/wmacro.h>
#include <WBase/memory.hpp>
#include "SceneComponent.h"
#include "../EntityComponentSystem/System.h"

namespace ecs {
	class SceneSystem : System {
		~SceneSystem();

		//需要考虑时序设计
		white::uint32 Update(const UpdateParams&);

		void OnGotMessage(const white::Message& message);

		void OnEntityAddSceneComponent(const std::pair<Entity*, SceneComponent*>& pair);
	};
}

#endif