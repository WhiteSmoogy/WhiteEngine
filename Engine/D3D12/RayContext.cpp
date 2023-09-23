#include "RayContext.h"
#include "RayDevice.h"
#include "Context.h"
#include "GraphicsBuffer.hpp"
#include "View.h"
#include "NodeDevice.h"
#include "CommandListManager.h"
#include "BuiltInRayTracingShaders.h"

using std::make_unique;

namespace D12 = platform_ex::Windows::D3D12;
namespace R = platform::Render;

using namespace D12;

bool D12::IsDirectXRaytracingSupported(ID3D12Device* device)
{
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureSupportData = {};

	return SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5,&featureSupportData,sizeof(featureSupportData)))
		&& featureSupportData.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
}

D12::RayDevice& D12::RayContext::GetDevice()
{
	return *ray_device;
}

D12::CommandContext* D12::RayContext::GetCommandContext() const
{
	return command_context;
}

D12::RayDevice::RayDevice(Device* pDevice, RayContext* pContext)
	:device(pDevice), context(pContext)
{
	if (!SUCCEEDED(device->d3d_device->QueryInterface(COMPtr_RefParam(d3d_ray_device, IID_ID3D12Device5))))
		throw white::unsupported();

	ray_tracing_pipeline_cache = make_unique<RayTracingPipelineCache>(d3d_ray_device.Get());
}

D12::RayTracingGeometry* D12::RayDevice::CreateRayTracingGeometry(const R::RayTracingGeometryInitializer& initializer)
{
	D12::RayTracingGeometry* Geometry = new D12::RayTracingGeometry(initializer);

	return Geometry;
}

RayTracingScene* D12::RayDevice::CreateRayTracingScene(const R::RayTracingSceneInitializer& initializer)
{
	auto Scene = new RayTracingScene(initializer);

	return Scene;
}

RayTracingPipelineState* D12::RayDevice::CreateRayTracingPipelineState(const platform::Render::RayTracingPipelineStateInitializer& initializer)
{
	auto PipelineState = new RayTracingPipelineState(initializer);

	return PipelineState;
}

RayTracingShader* D12::RayDevice::CreateRayTracingShader(white::span<const uint8> Code)
{
	auto Shader = new RayTracingShader(Code);

	return Shader;
}

const Fence& platform_ex::Windows::D3D12::RayDevice::GetFence() const
{
	return Context::Instance().GetDefaultCommandContext()->GetParentDevice()->GetCommandListManager(CommandQueueType::Default)->GetFence();
}

RayTracingPipelineCache* platform_ex::Windows::D3D12::RayDevice::GetRayTracingPipelineCache() const
{
	return ray_tracing_pipeline_cache.get();
}

D12::RayContext::RayContext(Device* pDevice, Context* pContext)
	:device(pDevice),context(pContext)
{
	ray_device = std::make_shared<RayDevice>(pDevice,this);

	command_context = context->GetDefaultCommandContext();
}

namespace platform_ex::Windows::D3D12 {
	class BasicRayTracingPipeline
	{
	public:
		BasicRayTracingPipeline(RayDevice* InDevice)
		{
			//Shadow pipeline
			{
				R::RayTracingPipelineStateInitializer ShadowInitializer;

				R::RayTracingShader* ShadowRGSTable[] = { GetBuildInRayTracingShader<ShadowRG>() };
				ShadowInitializer.RayGenTable = white::make_span(ShadowRGSTable);
				ShadowInitializer.bAllowHitGroupIndexing = false;

				Shadow = new RayTracingPipelineState(ShadowInitializer);
			}
		}
	public:
		RayTracingPipelineState* Shadow;
	};
}

void D12::RayContext::RayTraceShadow(R::RayTracingScene* InScene, const platform::Render::ShadowRGParameters& InConstants)
{
	auto Resource = dynamic_cast<D12::Texture2D*>(InConstants.Depth);

	auto ITex = dynamic_cast<R::Texture*>(Resource);

	D12::ShaderResourceView* DepthSRV = Resource->RetriveShaderResourceView();

	platform::Render::RayTracingShaderBindingsWriter Bindings;
	platform::Render::ShaderMapRef<ShadowRG> RayGenerationShader(GetBuiltInShaderMap());
	SetShaderParameters(Bindings, RayGenerationShader, InConstants);

	Bindings.SRVs[0] =static_cast<D12::RayTracingScene*>(InScene)->GetShaderResourceView();

	wconstraint(Bindings.SRVs[0]);

	D12::RayTracingPipelineState* Pipeline = GetBasicRayTracingPipeline()->Shadow;

	auto& ShaderTable = Pipeline->DefaultShaderTable;
	ShaderTable.UploadToGPU(*command_context);

	D3D12_DISPATCH_RAYS_DESC DispatchDesc = ShaderTable.GetDispatchRaysDesc(0, 0, 0);
	auto desc = Resource->GetResource()->GetDesc();
	DispatchDesc.Width =static_cast<UINT>(desc.Width);
	DispatchDesc.Height = static_cast<UINT>(desc.Height);
	DispatchDesc.Depth = 1;

	DispatchRays(*command_context, Bindings, Pipeline, 0, nullptr, DispatchDesc);
}


D12::BasicRayTracingPipeline* D12::RayContext::GetBasicRayTracingPipeline() const
{
	static BasicRayTracingPipeline Basics(ray_device.get());

	return &Basics;
}
