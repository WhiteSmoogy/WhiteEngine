#pragma once

#include "RenderInterface/ITexture.hpp"
#include "RenderInterface/RenderPassInfo.h"
#include "RenderInterface/ICommandList.h"

namespace platform
{
	struct TonemapInputs
	{
		// [Required] Render to the specified output. 
		// TODO: If invalid, a new texture is created and returned.
		Render::RenderTarget OverrideOutput;

		// [Required] HDR scene color to tonemap.
		Render::TexturePtr SceneColor;

		// [Required] Color grading texture used to remap colors.
		Render::TexturePtr ColorGradingTexture;
	};

	void TonemapPass(Render::CommandList& CmdList, const TonemapInputs& Inputs);
}
