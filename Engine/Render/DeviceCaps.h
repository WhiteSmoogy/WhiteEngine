/*! \file Engine\Render\DeviceCaps.h
\ingroup Engine
\brief �豸�������Լ��������塣
*/
#ifndef WE_RENDER_DeviceCaps_h
#define WE_RENDER_DeviceCaps_h 1

#include "IFormat.hpp"

#include <functional>

namespace platform{
	namespace Render {
		struct Caps {
			enum class Type {
				D3D12
			};

			Type type;

			uint16 max_texture_depth;

			bool support_hdr;

			std::function<bool(EFormat)> TextureFormatSupport;
			std::function<bool(EFormat)> VertexFormatSupport;
			std::function<bool(EFormat, SampleDesc)>	RenderTargetMSAASupport;
		};

		/** The number of render-targets that may be simultaneously written to. */
		constexpr unsigned MaxSimultaneousRenderTargets = 8;

		/** The number of UAVs that may be simultaneously bound to a shader. */
		constexpr unsigned MaxSimultaneousUAVs = 8;
	}
}

#endif