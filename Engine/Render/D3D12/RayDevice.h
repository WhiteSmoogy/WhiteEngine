#ifndef WE_RENDER_D3D12_RayDevice_h
#define WE_RENDER_D3D12_RayDevice_h 1

#include "../IRayDevice.h"
#include "d3d12_dxgi.h"
#include "RayTracingGeometry.h"
#include "RayTracingScene.h"
#include "RayTracingPipelineState.h"
#include "RayTracingShader.h"
#include "Fence.h"

class RayTracingDescriptorHeapCache;

namespace platform_ex::Windows::D3D12 {
	class Device;
	class RayContext;

	class RayDevice:public platform::Render::RayDevice
	{
	public:
		RayDevice(Device* pDevice, RayContext* pContext);

		RayTracingGeometry* CreateRayTracingGeometry(const platform::Render::RayTracingGeometryInitializer& initializer) final override;

		RayTracingScene* CreateRayTracingScene(const platform::Render::RayTracingSceneInitializer& initializer)  final override;

		RayTracingPipelineState* CreateRayTracingPipelineState(const platform::Render::RayTracingPipelineStateInitializer& initializer) final override;

		RayTracingShader* CreateRayTracingSahder(const  platform::Render::RayTracingShaderInitializer& initializer) final override;


		//TODO:state abstract cause performance(GPU sync point)
		void BuildAccelerationStructure(platform::Render::RayTracingGeometry* pGeometry) final override;
		void BuildAccelerationStructure(platform::Render::RayTracingScene* pGeometry) final override;

		ID3D12Device5* GetRayTracingDevice() const
		{
			return d3d_ray_device.Get();
		}

		const Fence& GetFence() const;

		RayTracingPipelineCache* GetRayTracingPipelineCache() const;
	private:
		Device* device;
		RayContext* context;

		COMPtr<ID3D12Device5> d3d_ray_device;

		std::unique_ptr<RayTracingPipelineCache> ray_tracing_pipeline_cache;
	};

	bool IsDirectXRaytracingSupported(ID3D12Device* device);
}

#endif
