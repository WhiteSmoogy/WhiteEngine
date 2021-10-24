#include "SceneComponent.h"
#include "../EntityComponentSystem/EntitySystem.h"
namespace ecs {
	SceneComponent::SceneComponent(const white::type_info& type_info, EntityId parent)
	:Component(type_info,parent), pParent(EntitySystem::Instance().GetEntity(parent)){
	}

}