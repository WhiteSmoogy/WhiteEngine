/*! \file Engine\Render\Effect\BiltEffect.h
\ingroup Engine\Render\Effect
\brief 用于在不同大小贴图之间复制(如:构建Mipmap)。
*/
#ifndef WE_RENDER_EFFECT_COPY_h
#define WE_RENDER_EFFECT_COPY_h 1

#include "Effect.hpp"

namespace platform::Render::Effect {

	class CopyEffect : public Effect {
	public:
		CopyEffect(const std::string& name);

		white::lref<Technique> BilinearCopy;
	};
}

#endif