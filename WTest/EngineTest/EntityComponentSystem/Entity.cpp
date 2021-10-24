#include "Entity.h"
#include "EntitySystem.h"
#include "ecsmsgdef.h"

using namespace ecs;

ImplDeDtor(Entity)

white::observer_ptr<Component> ecs::Entity::Add(const white::type_info & type_info, std::unique_ptr<Component> pComponent)
{
	EntitySystem::Instance().PostMessage({ Messaging::AddComponent,white::make_pair(this, pComponent.get()) });
	return white::make_observer(components.emplace(white::type_index(type_info), pComponent.release())->second.get());
}
