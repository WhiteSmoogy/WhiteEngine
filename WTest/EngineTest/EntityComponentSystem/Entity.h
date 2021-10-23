#ifndef FrameWork_ECS_Entity_h
#define FrameWork_ECS_Entity_h 1

#include <WBase/wmacro.h>
#include <WBase/typeindex.h>
#include <WFramework/Adaptor/WAdaptor.h>
#include <WFramework/Core/WException.h>
#include "Component.h"
#include "ECSException.h"

namespace ecs {

	wconstexpr EntityId InvalidEntityId = {};

	class Entity {
	public:
		template<typename... _tParams>
		Entity(EntityId id_, _tParams&&...)
			:id(id_)
		{}

		virtual ~Entity();

		DefGetter(const wnothrow,EntityId,Id,id)
		DefSetter(wnothrow,EntityId,Id,id)

		template<typename _type,typename ..._tParams>
		white::observer_ptr<_type> AddComponent(_tParams&&... args) {
			TryRet(white::make_observer(static_cast<_type*>(Add(white::type_id<_type>(), std::make_unique<_type>(white::type_id<_type>(),wforward(args)...)).get())))
				CatchThrow(ECSException& e, white::LoggedEvent(white::sfmt("AddComponent failed. (Inner %s)", e.message()), Warning))
		}

		template<typename _type>
		white::observer_ptr<_type> GetComponent() {
			return {};
		}
	private:
		white::observer_ptr<Component> Add(const white::type_info& type_info,std::unique_ptr<Component> pComponent);
	private:
		EntityId id;

		white::unordered_multimap<white::type_index, white::unique_ptr<Component>> components;
	};
}


#endif
