/*! \file Engine\Render\D3D12\FrameBuffer.h
\ingroup Engine\Render\D3D12
\brief ��Ⱦ������
*/
#ifndef WE_RENDER_D3D12_FrameBuffer_h
#define WE_RENDER_D3D12_FrameBuffer_h 1

#include "RenderInterface/IFrameBuffer.h"
#include "d3d12_dxgi.h"

namespace platform_ex::Windows::D3D12 {
	class DepthStencilView;

	class FrameBuffer :public platform::Render::FrameBuffer {
	public:
		FrameBuffer();
		~FrameBuffer();

		void OnBind() override;
		void OnUnBind() override;

		void Clear(white::uint32 flags, const white::math::float4  & clr, float depth, white::int32 stencil) override;

		DepthStencilView* GetDepthStencilView() const;

	public:

		friend class Display;
	private:
		D3D12_VIEWPORT d3d12_viewport;
	};
}

#endif
