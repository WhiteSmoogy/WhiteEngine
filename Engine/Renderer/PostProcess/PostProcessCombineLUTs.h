#pragma once

#include <WBase/wmathtype.hpp>
#include "RenderInterface/ITexture.hpp"
#include "RenderInterface/ICommandList.h"

namespace platform
{
	struct CombineLUTSettings
	{
		CombineLUTSettings();

		white::math::float4 ColorSaturation;
		white::math::float4 ColorContrast;
		white::math::float4 ColorGamma;
		white::math::float4 ColorGain;
		white::math::float4 ColorOffset;
		white::math::float4 ColorSaturationShadows;
		white::math::float4 ColorContrastShadows;
		white::math::float4 ColorGammaShadows;
		white::math::float4 ColorGainShadows;
		white::math::float4 ColorOffsetShadows;
		white::math::float4 ColorSaturationMidtones;
		white::math::float4 ColorContrastMidtones;
		white::math::float4 ColorGammaMidtones;
		white::math::float4 ColorGainMidtones;
		white::math::float4 ColorOffsetMidtones;
		white::math::float4 ColorSaturationHighlights;
		white::math::float4 ColorContrastHighlights;
		white::math::float4 ColorGammaHighlights;
		white::math::float4 ColorGainHighlights;
		white::math::float4 ColorOffsetHighlights;

		float ColorCorrectionShadowsMax = 0.09f;
		float ColorCorrectionHighlightsMin = 0.5f;

		float WhiteTemp;
		float WhiteTint;

		/** Correct for artifacts with "electric" blues due to the ACEScg color space. Bright blue desaturates instead of going to violet. */
		float BlueCorrection;
		/** Expand bright saturated colors outside the sRGB gamut to fake wide gamut rendering. */
		float ExpandGamut;

		float FilmSlope;
		float FilmToe;
		float FilmShoulder;
		float FilmBlackClip;
		float FilmWhiteClip;
	};

	std::shared_ptr<Render::Texture> CombineLUTPass(Render::CommandList& CmdList,const CombineLUTSettings& args);
}
