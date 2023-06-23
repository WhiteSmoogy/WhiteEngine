/*! \file Engine\Render\IRayTracingGeometry.h
\ingroup Engine
\brief ���߼�����Ϣ�ӿ��ࡣ
*/
#ifndef WE_RENDER_IRayRayTracingGeometry_h
#define WE_RENDER_IRayRayTracingGeometry_h 1

namespace platform::Render {
	class CommandContext;

	/** Bottom level ray tracing acceleration structure (contains triangles). */
	class RayTracingGeometry
	{
	public:
		virtual ~RayTracingGeometry();

		virtual void BuildAccelerationStructure(CommandContext& context) = 0;
	};
}

#endif