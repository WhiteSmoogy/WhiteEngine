#include "VolumeRendering.h"
#include <WBase/wmathtype.hpp>
#include <RenderInterface/IDevice.h>
#include <RenderInterface/IContext.h>
#include <WBase/smart_ptr.hpp>

using namespace platform;
using namespace platform::Render::Shader;

IMPLEMENT_BUILTIN_SHADER(WriteToSliceVS, "PostProcess/VolumeRendering.wsl", "WriteToSliceMainVS", platform::Render::VertexShader);
IMPLEMENT_BUILTIN_SHADER(WriteToSliceGS, "PostProcess/VolumeRendering.wsl", "WriteToSliceMainGS", platform::Render::GeometryShader);


struct ScreenVertex
{
	white::math::float2 Position;
	white::math::float2 UV;
};

std::shared_ptr<Render::GraphicsBuffer> platform::GVolumeRasterizeVertexBuffer()
{
	static struct VolumeRasterizeVertexBuffer
	{
		VolumeRasterizeVertexBuffer()
		{
			ScreenVertex DestVertex[4];

			DestVertex[0].Position = white::math::float2(1, -1);
			DestVertex[0].UV = white::math::float2(1, 1);

			DestVertex[1].Position = white::math::float2(1, 1);
			DestVertex[1].UV = white::math::float2(1, 0);

			DestVertex[2].Position = white::math::float2(-1, -1);
			DestVertex[2].UV = white::math::float2(0, 1);

			DestVertex[3].Position = white::math::float2(-1, 1);
			DestVertex[3].UV = white::math::float2(0, 0);

			platform::Render::ElementInitData InitData{ .data = DestVertex };
			platform::Render::ResourceCreateInfo CreateInfo{ &InitData,"VolumeRasterizeVertex" };

			VertexBuffer = white::share_raw(Render::Context::Instance().GetDevice().CreateVertexBuffer(Render::Buffer::Usage::Static,
				Render::EAccessHint::EA_GPURead | Render::EAccessHint::EA_Immutable,
				sizeof(DestVertex),
				Render::EF_Unknown, CreateInfo));
		}

		std::shared_ptr<Render::GraphicsBuffer> VertexBuffer;
	} Buffer;

	return Buffer.VertexBuffer;
}

Render::VertexDeclarationElements platform::GScreenVertexDeclaration()
{
	constexpr std::array<Render::VertexElement, 2> Elements =
	{ 
		Render::CtorVertexElement(0,woffsetof(ScreenVertex,Position),Render::Vertex::Usage::Position,0,Render::EF_GR32F,sizeof(ScreenVertex)),
		Render::CtorVertexElement(0,woffsetof(ScreenVertex,UV),Render::Vertex::Usage::TextureCoord,0,Render::EF_GR32F,sizeof(ScreenVertex)),
	};

	return { Elements.begin(),Elements.end()};
}

void platform::RasterizeToVolumeTexture(Render::CommandList& CmdList, VolumeBounds VolumeBounds)
{
	CmdList.SetViewport(VolumeBounds.MinX, VolumeBounds.MinY, 0, VolumeBounds.MaxX, VolumeBounds.MaxY, 0);
	CmdList.SetVertexBuffer(0, GVolumeRasterizeVertexBuffer().get());

	const auto NumInstances = VolumeBounds.MaxZ - VolumeBounds.MinZ;

	CmdList.DrawPrimitive(0, 0,2, NumInstances);
}
