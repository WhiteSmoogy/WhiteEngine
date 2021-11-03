#pragma once
#ifndef WE_RENDER_D3D12_RayTracingShader_h
#define WE_RENDER_D3D12_RayTracingShader_h 1

#include <WBase/observer_ptr.hpp>
#include "../Shader.h"
#include "../IRayTracingShader.h"
#include "../IRayDevice.h"
#include "d3d12_dxgi.h"
#include "RootSignature.h"
#include "HardwareShader.h"

namespace platform_ex::Windows::D3D12 {
	class RayTracingShader : public platform::Render::RayTracingShader,public D3D12HardwareShader
	{
	public:
		RayTracingShader(const white::span<const uint8>& Code);
	public:
		white::observer_ptr<RootSignature> pRootSignature = nullptr;


		std::u16string EntryPoint;
		std::u16string AnyHitEntryPoint;
		std::u16string IntersectionEntryPoint;

		platform::Render::ShaderCodeResourceCounts ResourceCounts;
	};

}

#endif