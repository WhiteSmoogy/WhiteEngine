#include "FrameBuffer.h"
#include "Context.h"

namespace platform_ex::Windows::D3D12 {
	FrameBuffer::FrameBuffer()
		:d3d12_viewport({ 0,0,0,0,0,1 })
	{
	}
	FrameBuffer::~FrameBuffer() = default;

	void FrameBuffer::OnBind() {
	}

	void FrameBuffer::OnUnBind()
	{
	}

	void FrameBuffer::Clear(white::uint32 flags, const white::math::float4 & clr, float depth, white::int32 stencil)
	{
	}

	DepthStencilView* D3D12::FrameBuffer::GetDepthStencilView() const
	{
		if (ds_view.Texture)
			return dynamic_cast<Texture*>(ds_view.Texture)->GetDepthStencilView({});
		return nullptr;
	}
	
}
