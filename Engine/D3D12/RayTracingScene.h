/*! \file Engine\Render\D3D12\RayTracingScene.h
\ingroup Engine
\brief 射线场景信息实现类。
*/
#pragma once

#ifndef WE_RENDER_D3D12_RayTracingScene_h
#define WE_RENDER_D3D12_RayTracingScene_h 1

#include "RenderInterface/IRayTracingScene.h"
#include "RenderInterface/IRayDevice.h"

#include "d3d12_dxgi.h"
#include "GraphicsBuffer.hpp"
#include "RayTracingGeometry.h"
#include "View.h"

namespace platform_ex::Windows::D3D12 {
	class CommandContext;

	class RayTracingScene :public platform::Render::RayTracingScene
	{
	public:
		RayTracingScene(const platform::Render::RayTracingSceneInitializer& initializer);

		void BuildAccelerationStructure(platform::Render::CommandContext& CommandContext) override;
		void BuildAccelerationStructure(CommandContext& CommandContext);

		ShaderResourceView* GetShaderResourceView() const
		{
			return AccelerationStructureView.get();
		}
	private:
		std::vector<platform::Render::RayTracingGeometryInstance> Instances;

		white::uint32 ShaderSlotsPerGeometrySegment;
		white::uint32 NumCallableShaderSlots;

		std::shared_ptr<GraphicsBuffer> AccelerationStructureBuffer;

		white::shared_ptr<ShaderResourceView> AccelerationStructureView;
	};
}

#endif
