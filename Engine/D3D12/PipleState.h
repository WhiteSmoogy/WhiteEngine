#ifndef WE_RENDER_D3D12_PipeState_h
#define WE_RENDER_D3D12_PipeState_h 1

#include "RenderInterface/Effect/Effect.hpp"
#include "RenderInterface/IFrameBuffer.h"
#include "RenderInterface/InputLayout.hpp"
#include "d3d12_dxgi.h"

namespace platform_ex::Windows::D3D12 {
	class ShaderCompose;
	class PipleState : public platform::Render::PipleState {
	public:
		using base = platform::Render::PipleState;
		PipleState(const base&);

		union {
			D3D12_GRAPHICS_PIPELINE_STATE_DESC graphics_ps_desc;
			D3D12_COMPUTE_PIPELINE_STATE_DESC compute_ps_desc;
		};
	private:
		

		mutable std::unordered_map<size_t, COMPtr<ID3D12PipelineState>> psos;
	};
}

#endif