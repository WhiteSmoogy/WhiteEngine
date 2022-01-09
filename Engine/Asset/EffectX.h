/*! \file Engine\Asset\EffectX.h
\ingroup Engine
\brief EFFECT IO ...
*/
#ifndef WE_ASSET_EFFECT_X_H
#define WE_ASSET_EFFECT_X_H 1


#include "EffectAsset.h"
#include "RenderInterface/Effect/Effect.hpp"
#include <filesystem>
#include <string_view>
namespace platform {
	namespace X {
		using path = std::filesystem::path;

		std::shared_ptr<asset::EffectAsset> LoadEffectAsset(path const& effectpath);
		std::shared_ptr<Render::Effect::Effect> LoadEffect(std::string const& name);
	}
}

#endif