/*! \file Engine\Render\IFormat.hpp
\ingroup Engine
\brief GPU×ÊÔ´View¡£
*/
#ifndef WE_RENDER_IGPUResourceView_h
#define WE_RENDER_IGPUResourceView_h 1

#include "ITexture.hpp"

namespace platform {
	namespace Render {
		class FrameBuffer;

		class ShaderResourceView :public RObject {
		public:
		};

		class UnorderedAccessView :public RObject
		{
		public:
		};

		using SRVRIRef = std::shared_ptr<platform::Render::ShaderResourceView>;
		using UAVRIRef = std::shared_ptr<platform::Render::UnorderedAccessView>;
	}
}

#endif