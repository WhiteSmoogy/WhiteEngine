#ifndef FrameWork_ECS_Component_h
#define FrameWork_ECS_Component_h 1

#include "ECSCommon.h"
#include <WBase/typeindex.h>

namespace ecs {
	
	struct Component {
		white::type_index type_index;

		template<typename... _tParams>
		Component(const white::type_info& type_info, _tParams&&...)
			:type_index(type_info) {
		}

		virtual ~Component();
	};

}


#endif
