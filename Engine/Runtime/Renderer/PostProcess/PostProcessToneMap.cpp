#include "PostProcessToneMap.h"
#include "RenderInterface/BuiltinShader.h"
#include "RenderInterface/ICommandList.h"
#include "RenderInterface/PipelineStateUtility.h"
#include "RenderInterface/PixelShaderUtils.h"
#include "RenderInterface/PipleState.h"
#include "RenderInterface/ShaderTextureTraits.hpp"

#include "Runtime/Renderer/VolumeRendering.h"

using namespace  platform::Render;

class TonemapVS : public BuiltInShader
{
public:
	EXPORTED_BUILTIN_SHADER(TonemapVS);
};

IMPLEMENT_BUILTIN_SHADER(TonemapVS, "PostProcess/PostProcessToneMap.wsl", "MainVS", platform::Render::VertexShader);

BEGIN_SHADER_PARAMETER_STRUCT(TonemapParameters)
	SHADER_PARAMETER_TEXTURE(Texture2D, ColorTexture)
	SHADER_PARAMETER_TEXTURE(Texture3D, ColorGradingLUT)
	SHADER_PARAMETER_SAMPLER(TextureSampleDesc, ColorSampler)
	SHADER_PARAMETER_SAMPLER(TextureSampleDesc, ColorGradingLUTSampler)
END_SHADER_PARAMETER_STRUCT()

class TonemapPS : public BuiltInShader
{
public:
	using Parameters = TonemapParameters;
	EXPORTED_BUILTIN_SHADER(TonemapPS);
};

IMPLEMENT_BUILTIN_SHADER(TonemapPS, "PostProcess/PostProcessToneMap.wsl", "MainPS", platform::Render::PixelShader);

void platform::TonemapPass(Render::CommandList& CmdList, const TonemapInputs& Inputs)
{
	auto PixelShader = Render::GetBuiltInShaderMap()->GetShader<TonemapPS>();

	Render::RenderPassInfo passInfo(Inputs.OverrideOutput.Texture, Render::RenderTargetActions::Clear_Store);

	CmdList.BeginRenderPass(passInfo, "TonemapPass");

	Render::GraphicsPipelineStateInitializer GraphicsPSOInit {};
	PixelShaderUtils::InitFullscreenPipelineState(CmdList,PixelShader,GraphicsPSOInit);

	auto VertexShader = Render::GetBuiltInShaderMap()->GetShader<TonemapVS>();
	GraphicsPSOInit.ShaderPass.VertexShader = VertexShader.GetVertexShader();

	SetGraphicsPipelineState(CmdList, GraphicsPSOInit);

	TonemapParameters CommonParameters;
	CommonParameters.ColorGradingLUT =static_cast<Texture3D*>(Inputs.ColorGradingTexture.get());
	CommonParameters.ColorTexture = static_cast<Texture2D*>(Inputs.SceneColor.get());

	Render::TextureSampleDesc BilinearClampSampler{};
	BilinearClampSampler.address_mode_u = BilinearClampSampler.address_mode_v = BilinearClampSampler.address_mode_w = Render::TexAddressingMode::Clamp;
	BilinearClampSampler.filtering = Render::TexFilterOp::Min_Mag_Linear_Mip_Point;

	CommonParameters.ColorGradingLUTSampler = BilinearClampSampler;
	CommonParameters.ColorSampler = BilinearClampSampler;

	SetShaderParameters(CmdList, PixelShader, PixelShader.GetPixelShader(), CommonParameters);

	PixelShaderUtils::DrawFullscreenTriangle(CmdList);
}
