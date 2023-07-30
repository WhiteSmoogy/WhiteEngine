/*! \file Engine\Render\IRayContext.h
\ingroup Engine
\brief 射线执行接口类。
*/
#ifndef WE_RENDER_IRayContext_h
#define WE_RENDER_IRayContext_h 1

#include "IRayDevice.h"
#include "IFrameBuffer.h"
#include "Runtime/RenderCore/ShaderParamterTraits.hpp"
#include "Runtime/RenderCore/ShaderTextureTraits.hpp"

namespace platform::Render {

	BEGIN_SHADER_PARAMETER_STRUCT(ShadowRGParameters)
		SHADER_PARAMETER(white::math::float4x4, SVPositionToWorld)
		SHADER_PARAMETER(white::math::float3, WorldCameraOrigin)
		SHADER_PARAMETER(white::math::float4, BufferSizeAndInvSize)
		SHADER_PARAMETER(float, NormalBias)
		SHADER_PARAMETER(white::math::float3, LightDirection)
		SHADER_PARAMETER(float, SourceRadius)
		SHADER_PARAMETER(unsigned, SamplesPerPixel)
		SHADER_PARAMETER(unsigned, StateFrameIndex)
		SHADER_PARAMETER_TEXTURE(platform::Render::Texture2D, WorldNormalBuffer)
		SHADER_PARAMETER_TEXTURE(platform::Render::Texture2D, Depth)
		SHADER_PARAMETER_UAV(RWTexture2D, Output)
		END_SHADER_PARAMETER_STRUCT();

	class RayContext
	{
	public:
		virtual RayDevice& GetDevice() = 0;

		virtual void RayTraceShadow(RayTracingScene* InScene,const ShadowRGParameters& InConstants) =0;
	};
}

#endif
