#ifndef WE_RENDER_D3D12_RayContxt_h
#define WE_RENDER_D3D12_RayContxt_h 1

#include <WBase/wmemory.hpp>
#include "RenderInterface/IRayContext.h"
#include "RayDevice.h"

namespace platform_ex::Windows::D3D12 {
	class Device;
	class Context;
	class BasicRayTracingPipeline;
	class CommandContext;

	class RayContext :public platform::Render::RayContext
	{
	public:
		RayContext(Device* pDevice, Context* pContext);

		RayDevice& GetDevice() override;

		CommandContext* GetCommandContext() const;

		void RayTraceShadow(platform::Render::RayTracingScene* InScene, const platform::Render::ShadowRGParameters& InConstants) final;

		BasicRayTracingPipeline* GetBasicRayTracingPipeline() const;
	private:
		std::shared_ptr<RayDevice> ray_device;

		Device* device;
		Context* context;

		CommandContext* command_context;
	};
}

#endif
