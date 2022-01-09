/*! \file Core\CameraController.h
\ingroup Engine
\brief 提供相机相关的基本操作器。
*/

#ifndef WE_Core_CameraController_H
#define WE_Core_CameraController_H 1

#include <WBase/sutility.h>
#include <WBase/memory.hpp>

#include "Camera.h"


namespace WhiteEngine::Core {
	namespace wm = white::math;

	class CameraManipulator : white::noncopyable {
	public:
		CameraManipulator(float rotationSpeed = 0.05f,float moveSpeed=1):
			rotationScaler(rotationSpeed), moveScaler(moveSpeed)
		{}

		void SetSpeed(float rotationSpeed, float moveSpeed) {
			rotationScaler = rotationSpeed;
			moveScaler = moveSpeed;
		}

		virtual ~CameraManipulator();

		virtual void Attach(Camera& camera);
		virtual void Detach();
	protected:
		white::observer_ptr<Camera> pCamera;

		float rotationScaler, moveScaler;
	};

	class TrackballCameraManipulator :public CameraManipulator {
	public:
		explicit TrackballCameraManipulator(float dist):
			look_at_dist(dist)
		{}

		void Attach(Camera& camera) override;

		void Move(wm::float2 offset);
		void Rotate(wm::float2 offset);
		void Zoom(wm::float2 offset);
	private:
		bool reverse_target;
		wm::float3 target;
		wm::float3 right;

		float look_at_dist;
	};
}

#endif