/*! \file Engine\Render\Effect\BiltEffect.h
\ingroup Engine\Render\Effect
\brief 实现GDI式的Blit效果。
*/
#ifndef WE_RENDER_EFFECT_BILT_h
#define WE_RENDER_EFFECT_BILT_h 1

#include "EffectProperty.h"

namespace platform::Render::Effect {

	class BiltEffect : public Effect {
	public:
		BiltEffect(const std::string& name);

		float3property src_offset;
		float3property src_scale;
		intproperty src_level;
		white::lref<Technique> BlitLinear2D;
	};
}

#endif