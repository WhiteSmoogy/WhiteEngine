/*! \file Engine\Render\IFormat.hpp
\ingroup Engine
\brief GPU��ԴView��
*/
#ifndef WE_RENDER_IGPUResourceView_h
#define WE_RENDER_IGPUResourceView_h 1

#include "ITexture.hpp"

namespace platform {
	namespace Render {
		class FrameBuffer;

		class ShaderResourceView{
		public:
			virtual ~ShaderResourceView();
		};

		class UnorderedAccessView :public RObject
		{
		public:
			virtual ~UnorderedAccessView();
		};
	}
}

#endif