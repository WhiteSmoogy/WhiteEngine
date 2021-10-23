/*! \file Engine\Asset\MaterailAsset.h
\ingroup Engine
\brief Materail Infomation ...
*/
#ifndef WE_ASSET_MATERIAL_ASSET_H
#define WE_ASSET_MATERIAL_ASSET_H 1

#include "EffectAsset.h"
#include <WFramework/Core/WObject.h>

namespace asset {
	class MaterailAsset :white::noncopyable {
	public:
		MaterailAsset() = default;

	public:
		struct PassInfo {
			std::pair<white::uint8,white::uint8> pass_used_range;
		};

		DefGetter(const wnothrow, const std::string&, EffectName, effect_name)
			DefGetter(wnothrow, std::string&, EffectNameRef, effect_name)

		DefGetter(const wnothrow, const std::vector<std::pair<size_t WPP_Comma white::ValueObject>>&, BindValues, bind_values)
			DefGetter(wnothrow, std::vector<std::pair<size_t WPP_Comma white::ValueObject>>&, BindValuesRef, bind_values)
	private:
		std::string effect_name;
		std::vector<std::pair<size_t, white::ValueObject>> bind_values;

		std::string asset_path;
	};
}

#endif