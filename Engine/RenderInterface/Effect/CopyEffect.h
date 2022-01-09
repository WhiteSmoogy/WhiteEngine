/*! \file Engine\Render\Effect\BiltEffect.h
\ingroup Engine\Render\Effect
\brief �����ڲ�ͬ��С��ͼ֮�临��(��:����Mipmap)��
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