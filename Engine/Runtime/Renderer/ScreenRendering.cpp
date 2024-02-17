#include "ScreenRendering.h"
#include "RenderInterface/IContext.h"
#include <WBase/wmath.hpp>
#include <WBase/smart_ptr.hpp>

using namespace WhiteEngine;
using namespace platform;

std::shared_ptr<platform::Render::GraphicsBuffer> WhiteEngine::GFullScreenVertexBuffer()
{
	white::math::float4 DestVertex[] =
	{
		white::math::float4(-1.0f, 1.0f, 0.0f, 1.0f),
		white::math::float4(1.0f, 1.0f, 0.0f, 1.0f),
		white::math::float4(-1.0f, -1.0f, 0.0f, 1.0f),
		white::math::float4(1.0f, -1.0f, 0.0f, 1.0f),
	};

	platform::Render::ResourceCreateInfoEx CreateInfo(DestVertex, "ScreenVertexBuffer");

	static std::shared_ptr<platform::Render::GraphicsBuffer> ClearVertexBuffer
		 = white::share_raw(Render::Context::Instance().GetDevice().CreateVertexBuffer(Render::Buffer::Usage::Static,
			 Render::EAccessHint::VertexOrIndexBuffer | Render::EAccessHint::Immutable,
			 sizeof(DestVertex),
			 Render::EF_Unknown, CreateInfo));

	return ClearVertexBuffer;
}