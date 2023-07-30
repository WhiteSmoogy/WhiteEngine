#include "UnifiedBuffer.h"
#include "RenderInterface/IContext.h"

using namespace platform::Render;

ConstantBuffer* platform::Render::CreateConstantBuffer(const void* Contents, Buffer::Usage Usage, const ShaderParametersMetadata& Layout)
{
	auto& Device = Context::Instance().GetDevice();

	Usage = static_cast<Buffer::Usage>(Usage | Buffer::Usage::Static);

	return Device.CreateConstantBuffer(Usage, Layout.GetSize(), Contents);
}