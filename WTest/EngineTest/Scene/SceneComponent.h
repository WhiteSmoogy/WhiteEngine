#ifndef FrameWork_ECS_SceneComponent_h
#define FrameWork_ECS_SceneComponent_h 1

#include "../EntityComponentSystem/Component.h"
#include "../EntityComponentSystem/Entity.h"
#include <WFramework/Adaptor/WAdaptor.h>
namespace ecs {
	struct SceneComponent final :public Component {
		white::observer_ptr<Entity> pOwner;
		white::observer_ptr<Entity> pParent;
		white::vector<white::observer_ptr<Entity>> pChilds;

		SceneComponent(const white::type_info& type_info, EntityId parent);

		SceneComponent(const white::type_info& type_info, white::observer_ptr<Entity> pParent)
			:SceneComponent(type_info, pParent->GetId()) 
		{}
	};
}

#endif