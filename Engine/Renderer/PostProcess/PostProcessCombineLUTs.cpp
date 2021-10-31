#include "PostProcessCombineLUTs.h"
#include <Engine/Render/ShaderParamterTraits.hpp>
#include <Engine/Render/BuiltInShader.h>
#include <Engine/Render/IContext.h>
#include <Engine/Render/PipelineStateUtility.h>
#include <Engine/Render/ShaderParameterStruct.h>
#include <Engine/System/SystemEnvironment.h>
#include <WBase/smart_ptr.hpp>
#include <Engine/Render/ICommandList.h>
#include "../VolumeRendering.h"

using namespace platform;
using namespace platform::Render::Shader;

platform::CombineLUTSettings::CombineLUTSettings()
{
	ColorSaturation = white::math::float4(1.0f, 1.0f, 1.0f, 1.0f);
	ColorContrast = white::math::float4(1.0f, 1.0f, 1.0f, 1.0f);
	ColorGamma = white::math::float4(1.0f, 1.0f, 1.0f, 1.0f);
	ColorGain = white::math::float4(1.0f, 1.0f, 1.0f, 1.0f);
	ColorOffset = white::math::float4(0.0f, 0.0f, 0.0f, 0.0f);

	ColorSaturationShadows = white::math::float4(1.0f, 1.0f, 1.0f, 1.0f);
	ColorContrastShadows = white::math::float4(1.0f, 1.0f, 1.0f, 1.0f);
	ColorGammaShadows = white::math::float4(1.0f, 1.0f, 1.0f, 1.0f);
	ColorGainShadows = white::math::float4(1.0f, 1.0f, 1.0f, 1.0f);
	ColorOffsetShadows = white::math::float4(0.0f, 0.0f, 0.0f, 0.0f);

	ColorSaturationMidtones = white::math::float4(1.0f, 1.0f, 1.0f, 1.0f);
	ColorContrastMidtones = white::math::float4(1.0f, 1.0f, 1.0f, 1.0f);
	ColorGammaMidtones = white::math::float4(1.0f, 1.0f, 1.0f, 1.0f);
	ColorGainMidtones = white::math::float4(1.0f, 1.0f, 1.0f, 1.0f);
	ColorOffsetMidtones = white::math::float4(0.f, 0.0f, 0.0f, 0.0f);

	ColorSaturationHighlights = white::math::float4(1.0f, 1.0f, 1.0f, 1.0f);
	ColorContrastHighlights = white::math::float4(1.0f, 1.0f, 1.0f, 1.0f);
	ColorGammaHighlights = white::math::float4(1.0f, 1.0f, 1.0f, 1.0f);
	ColorGainHighlights = white::math::float4(1.0f, 1.0f, 1.0f, 1.0f);
	ColorOffsetHighlights = white::math::float4(0.0f, 0.0f, 0.0f, 0.0f);

	WhiteTemp = 6500;
	WhiteTint = 0;

	// ACES settings
	FilmSlope = 0.88f;
	FilmToe = 0.55f;
	FilmShoulder = 0.26f;
	FilmBlackClip = 0.0f;
	FilmWhiteClip = 0.04f;

	BlueCorrection = 0.6f;
	ExpandGamut = 1.0f;
}

class LUTBlenderShader : public BuiltInShader
{
public:
	using DerivedType = LUTBlenderShader;
	using BuiltInShader::BuiltInShader;
	//SetDefine("USE_VOLUME_LUT", true);
};

BEGIN_SHADER_PARAMETER_STRUCT(CombineLUTParameters)
SHADER_PARAMETER(white::math::float4, ColorSaturation)
SHADER_PARAMETER(white::math::float4, ColorContrast)
SHADER_PARAMETER(white::math::float4, ColorGamma)
SHADER_PARAMETER(white::math::float4, ColorGain)
SHADER_PARAMETER(white::math::float4, ColorOffset)
SHADER_PARAMETER(white::math::float4, ColorSaturationShadows)
SHADER_PARAMETER(white::math::float4, ColorContrastShadows)
SHADER_PARAMETER(white::math::float4, ColorGammaShadows)
SHADER_PARAMETER(white::math::float4, ColorGainShadows)
SHADER_PARAMETER(white::math::float4, ColorOffsetShadows)
SHADER_PARAMETER(white::math::float4, ColorSaturationMidtones)
SHADER_PARAMETER(white::math::float4, ColorContrastMidtones)
SHADER_PARAMETER(white::math::float4, ColorGammaMidtones)
SHADER_PARAMETER(white::math::float4, ColorGainMidtones)
SHADER_PARAMETER(white::math::float4, ColorOffsetMidtones)
SHADER_PARAMETER(white::math::float4, ColorSaturationHighlights)
SHADER_PARAMETER(white::math::float4, ColorContrastHighlights)
SHADER_PARAMETER(white::math::float4, ColorGammaHighlights)
SHADER_PARAMETER(white::math::float4, ColorGainHighlights)
SHADER_PARAMETER(white::math::float4, ColorOffsetHighlights)
SHADER_PARAMETER(float,ColorCorrectionShadowsMax)
SHADER_PARAMETER(float,ColorCorrectionHighlightsMin)
SHADER_PARAMETER(float, WhiteTemp)
SHADER_PARAMETER(float, WhiteTint)
SHADER_PARAMETER(float, BlueCorrection)
SHADER_PARAMETER(float, ExpandGamut)
SHADER_PARAMETER(float, FilmSlope)
SHADER_PARAMETER(float, FilmToe)
SHADER_PARAMETER(float, FilmShoulder)
SHADER_PARAMETER(float, FilmBlackClip)
SHADER_PARAMETER(float, FilmWhiteClip)
SHADER_PARAMETER(white::math::float3, InverseGamma)
END_SHADER_PARAMETER_STRUCT();


class LUTBlenderPS : public LUTBlenderShader
{
public:
	using Parameters = CombineLUTParameters;
	EXPORTED_BUILTIN_SHADER(LUTBlenderPS);
};

IMPLEMENT_BUILTIN_SHADER(LUTBlenderPS, "PostProcess/PostProcessCombineLUTs.wsl", "MainPS", platform::Render::PixelShader);

void GetCombineLUTParameters(CombineLUTParameters& Parameters, const CombineLUTSettings& args)
{
	std::memcpy(&Parameters.ColorSaturation, &args.ColorSaturation, woffsetof(CombineLUTParameters, InverseGamma) - woffsetof(CombineLUTParameters, ColorSaturation));

	auto DisplayGamma = Environment->Gamma;

	Parameters.InverseGamma.x = 1.0f / DisplayGamma;
	Parameters.InverseGamma.y = 2.2f / DisplayGamma;
	Parameters.InverseGamma.z = 1.0f / std::max(DisplayGamma, 1.0f);
}

constexpr int32 GLUTSize = 32;

std::shared_ptr<Render::Texture> platform::CombineLUTPass(const CombineLUTSettings& args)
{
	const bool bUseVolumeTextureLUT = true;

	Render::Texture* OutputTexture = nullptr;

	Render::Texture3DInitializer initializer;
	initializer.Width = initializer.Height = initializer.Depth = GLUTSize;
	initializer.ArraySize = 1;
	initializer.NumMipmaps = 1;
	initializer.Format = Render::EF_ABGR16F;
	initializer.Access = Render::EA_GPURead | Render::EA_RTV;
	initializer.NumSamples = 1;

	Render::ElementInitData data;
	data.clear_value = &Render::ClearValueBinding::Black;
	if(bUseVolumeTextureLUT)
		OutputTexture = Render::Context::Instance().GetDevice().CreateTexture(initializer,&data);

	auto& CmdList = Render::GetCommandList();

	Render::RenderPassInfo passInfo(OutputTexture,Render::RenderTargetActions::Clear_Store);

	CmdList.BeginRenderPass(passInfo,"CombineLUTPass");

	Render::GraphicsPipelineStateInitializer GraphicsPSOInit{};
	CmdList.FillRenderTargetsInfo(GraphicsPSOInit);

	GraphicsPSOInit.BlendState = {};
	GraphicsPSOInit.RasterizerState.cull = Render::CullMode::None;
	GraphicsPSOInit.DepthStencilState.depth_enable = false;
	GraphicsPSOInit.DepthStencilState.depth_func = Render::CompareOp::Pass;

	CombineLUTParameters PassParameters;
	GetCombineLUTParameters(PassParameters,args);

	if (bUseVolumeTextureLUT)
	{
		const VolumeBounds Bounds(GLUTSize);

		auto VertexShader = Render::GetBuiltInShaderMap()->GetShader<WriteToSliceVS>();

		auto GeometryShader = Render::GetBuiltInShaderMap()->GetShader<WriteToSliceGS>();

		auto PixelShader = Render::GetBuiltInShaderMap()->GetShader<LUTBlenderPS>();

		GraphicsPSOInit.Primitive = Render::PrimtivteType::TriangleStrip;
		GraphicsPSOInit.ShaderPass.VertexDeclaration = GScreenVertexDeclaration();
		GraphicsPSOInit.ShaderPass.VertexShader = VertexShader.GetVertexShader();
		GraphicsPSOInit.ShaderPass.GeometryShader = GeometryShader.GetGeometryShader();
		GraphicsPSOInit.ShaderPass.PixelShader = PixelShader.GetPixelShader();

		SetGraphicsPipelineState(CmdList, GraphicsPSOInit);

		VertexShader->SetParameters(CmdList, Bounds, white::math::int3(Bounds.MaxX - Bounds.MinX));

		SetShaderParameters(CmdList, PixelShader, PixelShader.GetPixelShader(), PassParameters);

		RasterizeToVolumeTexture(CmdList, Bounds);
	}

	return white::share_raw(OutputTexture);
}
