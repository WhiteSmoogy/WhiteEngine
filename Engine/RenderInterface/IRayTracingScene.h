/*! \file Engine\Render\IRayTracingScene.h
\ingroup Engine
\brief ���߳�����Ϣ�ӿ��ࡣ
*/
#pragma once
#ifndef WE_RENDER_IRayRayTracingScene_h
#define WE_RENDER_IRayRayTracingScene_h 1

#include  <WBase/wmathtype.hpp>

namespace platform::Render {
	/** Top level ray tracing acceleration structure (contains instances of meshes). */
	class RayTracingScene
	{
	public:
		virtual ~RayTracingScene();
	};
}

#endif