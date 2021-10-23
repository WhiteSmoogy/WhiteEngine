#include "SceneSystem.h"
#include "../EntityComponentSystem/ecsmsgdef.h"

namespace ecs {

	ImplDeDtor(SceneSystem)

	white::uint32 SceneSystem::Update(const UpdateParams &)
	{
		return {};
	}

	void SceneSystem::OnGotMessage(const white::Message& message) {
		switch (message.GetMessageID())
		{
		case Messaging::AddComponent:
			if (auto ptr = message.GetContent().AccessPtr<std::pair<Entity*, SceneComponent*>>()) {
				OnEntityAddSceneComponent(*ptr);
			}
			break;
		}
	}

	void SceneSystem::OnEntityAddSceneComponent(const std::pair<Entity*, SceneComponent*>& pair) {
		auto pEntity = white::make_observer(pair.first);
		auto pSceneComponent = pair.second;

		//Change Componnet Owner
		pSceneComponent->pOwner = pEntity;

		//Add Child
		if (pSceneComponent->pParent) {
			pSceneComponent->pParent->GetComponent<SceneComponent>()->pChilds.emplace_back(pEntity);
		}
	}

}

