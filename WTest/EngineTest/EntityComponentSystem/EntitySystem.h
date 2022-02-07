#ifndef FrameWork_ECS_Entity_SYSTEM_h
#define FrameWork_ECS_Entity_SYSTEM_h 1

#include <WBase/typeinfo.h>
#include <WBase/wmacro.h>
#include <WBase/memory.hpp>
#include "System.h"
#include "Entity.h"
#include "ECSException.h"

namespace ecs {

	class EntitySystem:System {
	public:
		//TODO Coroutine!
		white::uint32 Update(const UpdateParams&) override;

		template<typename _type,typename... _tParams>
		white::observer_ptr<_type> AddSystem(_tParams&&... args) {
			TryRet(white::make_observer(static_cast<_type*>(Add(white::type_id<_type>(),std::make_unique<_type>(lforward(args)...)).get())))
				CatchThrow(ECSException& e, white::LoggedEvent(white::sfmt("AddSystem failed. (Inner %s)",e.what()),Warning))
		}

		template<typename _type, typename... _tParams>
		EntityId AddEntity(_tParams&&... args) {
			auto ret = GenerateEntityId();
			if (ret == InvalidEntityId) {
				WF_Trace(Critical, "AddEntity Failed.ID range is full!");
				return ret;
			}
			TryRet((Add(white::type_id<_type>(), ret,std::make_unique<_type>(ret,lforward(args)...))->GetId()))
				CatchThrow(ECSException& e, white::LoggedEvent(white::sfmt("AddEntity failed. (Inner %s)", e.what()), Warning))
		}

		white::observer_ptr<Entity> GetEntity(EntityId id);

		void RemoveEntity(EntityId id) wnothrow;

		void PostMessage(const white::Message& message);

		void OnGotMessage(const white::Message& message) override;

		static EntitySystem& Instance();
	private:
		EntityId GenerateEntityId() const wnothrow;

		white::observer_ptr<System> Add(const white::type_info& type_info, std::unique_ptr<System> pSystem);
		white::observer_ptr<Entity> Add(const white::type_info& type_info,EntityId id,std::unique_ptr<Entity> pEntity);
	};
}


#endif
