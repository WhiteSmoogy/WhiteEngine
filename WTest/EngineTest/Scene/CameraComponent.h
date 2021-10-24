#ifndef FrameWork_ECS_CameraComponent_h
#define FrameWork_ECS_CameraComponent_h 1

#include "../EntityComponentSystem/Component.h"
#include "../EntityComponentSystem/Entity.h"
#include <WFramework/Adaptor/LAdaptor.h>

namespace ecs {
	struct CameraComponent final :public Component {
		CameraComponent(const white::type_info& type_info);
		
		/*!
		\brief ͸��ͶӰ��Ϣ��
		*/
		//@{
		float near, far;
		float fov,aspect;
		//@}
	};
}

#endif