/*! \file Engine\Render\IFrameBuffer.h
\ingroup Engine
\brief ‰÷»æµΩŒ∆¿Ì°£
*/
#ifndef WE_RENDER_IFrameBuffer_h
#define WE_RENDER_IFrameBuffer_h 1

#include "IGPUResourceView.h"
#include "ViewPort.h"
#include "RenderPassInfo.h"

#include <WBase/wmathtype.hpp>
#include <WBase/observer_ptr.hpp>

#include <vector>
#include <memory>

namespace platform::Render {

	class FrameBuffer {
	public:
		enum Attachment : uint8 {
			Target0,
			Target1,
			Target2,
			Target3,
			Target4,
			Target5,
			Target6,
			Target7,
			DepthStencil,
		};

		enum ClearFlag : uint8 {
			Color = 1 << 0,
			Depth = 1 << 1,
			Stencil = 1 << 2,
		};

		virtual ~FrameBuffer();

		virtual void OnBind();
		virtual void OnUnBind();

		void Attach(Attachment which, const RenderTarget& view);
		void Attach(Attachment which, const DepthRenderTarget& view);
		void Attach(Attachment which, const std::shared_ptr<UnorderedAccessView>& view);

		void Detach(Attachment which);
		void DetachUAV(white::uint8 which);

		virtual void Clear(white::uint32 flags, const white::math::float4  & clr, float depth, white::int32 stencil) = 0;

		Texture* Attached(FrameBuffer::Attachment) const;
	protected:
		std::vector<RenderTarget> clr_views;
		DepthRenderTarget ds_view;
		std::vector<std::shared_ptr<UnorderedAccessView>> uav_views;
	};
}

#endif