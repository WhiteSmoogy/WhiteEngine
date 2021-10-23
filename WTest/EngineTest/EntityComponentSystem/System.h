#ifndef FrameWork_ECS_SYSTEM_h
#define FrameWork_ECS_SYSTEM_h 1

#include <WBase/winttype.hpp>
#include <WFramework/Core/WMessage.h>
#include "ECSCommon.h"

namespace ecs {
	struct UpdateParams {
		double timeStep;
	};

	class System {
	public:
		virtual ~System();

		virtual white::uint32 Update(const UpdateParams&) = 0;

		virtual void OnGotMessage(const white::Message& message) = 0;
	};
}


#endif
