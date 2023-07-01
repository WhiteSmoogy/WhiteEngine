#pragma once
#ifndef WE_RENDER_D3D12_RayTracingPipelineState_h
#define WE_RENDER_D3D12_RayTracingPipelineState_h 1

#include "RenderInterface/IRayTracingPipelineState.h"
#include "RenderInterface/IRayDevice.h"
#include "d3d12_dxgi.h"
#include "D3D12RayTracing.h"
#include "RootSignature.h"

namespace platform_ex::Windows::D3D12 {
	class RayTracingPipelineState : public platform::Render::RayTracingPipelineState
	{
	public:
		RayTracingPipelineState(const platform::Render::RayTracingPipelineStateInitializer& initializer);

		ID3D12RootSignature* GetRootSignature() const
		{
			return GlobalRootSignature->GetSignature();
		}
	public:
		COMPtr<ID3D12StateObject> StateObject;
		COMPtr<ID3D12StateObjectProperties> PipelineProperties;

		// This is useful for the case where user only provides default RayGen, Miss and HitGroup shaders.
		::RayTracingShaderTable DefaultShaderTable;

		::RayTracingShaderLibrary RayGenShaders;
		::RayTracingShaderLibrary MissShaders;
		::RayTracingShaderLibrary HitGroupShaders;
		::RayTracingShaderLibrary CallableShaders;

		white::observer_ptr<RootSignature> GlobalRootSignature = nullptr;

		bool bAllowHitGroupIndexing = true;
		white::uint32 MaxLocalRootSignatureSize = 0;
		white::uint32 MaxHitGroupViewDescriptors = 0;
	};
}

#endif