/*! \file Engine\Asset\EffectAsset.h
\ingroup Engine
\brief EFFECT Infomation ...
*/
#ifndef WE_ASSET_EFFECT_ASSET_H
#define WE_ASSET_EFFECT_ASSET_H 1

#include "ShaderAsset.h"

namespace asset {
	class TechniquePassAsset :public AssetName {
	public:
		using ShaderType = platform::Render::ShaderType;
		DefGetter(const wnothrow, const std::vector<ShaderMacro>&, Macros, macros)
			DefGetter(wnothrow, std::vector<ShaderMacro>&, MacrosRef, macros)

			DefGetter(const wnothrow, const platform::Render::PipleState&, PipleState, piple_state)
			DefGetter(wnothrow, platform::Render::PipleState&, PipleStateRef, piple_state)

			void AssignOrInsertHash(ShaderType type, size_t  blobhash)
		{
			blobindexs.insert_or_assign(type, blobhash);
		}

		size_t GetBlobHash(ShaderType type) const {
			return blobindexs.find(type)->second;
		}

		const std::unordered_map<ShaderType, size_t>& GetBlobs() const wnothrow {
			return blobindexs;
		}
	private:
		platform::Render::PipleState piple_state;
		std::vector<ShaderMacro> macros;
		std::unordered_map<ShaderType, size_t> blobindexs;
	};

	class EffectTechniqueAsset : public AssetName {
	public:
		DefGetter(const wnothrow, const std::vector<ShaderMacro>&, Macros, macros)
			DefGetter(wnothrow, std::vector<ShaderMacro>&, MacrosRef, macros)

			DefGetter(const wnothrow, const std::vector<TechniquePassAsset>&, Passes, passes)
			DefGetter(wnothrow, std::vector<TechniquePassAsset>&, PassesRef, passes)
	private:
		std::vector<ShaderMacro> macros;
		std::vector<TechniquePassAsset> passes;
	};

	class EffectAsset :public ShadersAsset,public  AssetName, white::noncopyable {
	public:
		EffectAsset() = default;

			DefGetter(const wnothrow, const std::vector<EffectTechniqueAsset>&, Techniques, techniques)
			DefGetter(wnothrow, std::vector<EffectTechniqueAsset>&, TechniquesRef, techniques)
	private:
		std::vector<EffectTechniqueAsset> techniques;
	};
}

#endif