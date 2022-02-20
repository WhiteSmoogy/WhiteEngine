#include "IFrameBuffer.h"
#include "IContext.h"

platform::Render::Context* GRenderIF;

enum ContextType {
	Context_D3D12
};

namespace platform_ex {
	namespace Windows {
		namespace D3D12 {
			bool Support();
			platform::Render::Context& GetContext();
		}
	}
}

namespace platform::Render {
	HardwareShader::~HardwareShader() = default;

	Context& Context::Instance() {
		static bool call_onece = [&]()->bool {
			GRenderIF = &platform_ex::Windows::D3D12::GetContext();
			return GRenderIF != nullptr;
		}();
		return *GRenderIF;
	}

	void Context::SetFrame(const std::shared_ptr<FrameBuffer>& framebuffer)
	{
		if (!framebuffer && curr_frame_buffer)
			curr_frame_buffer->OnUnBind();

		if (framebuffer) {
			curr_frame_buffer = framebuffer;

			DoBindFrameBuffer(curr_frame_buffer);

			curr_frame_buffer->OnBind();
		}
	}
	const std::shared_ptr<FrameBuffer>& Context::GetCurrFrame() const
	{
		return curr_frame_buffer;
	}
	const std::shared_ptr<FrameBuffer>& Context::GetScreenFrame() const
	{
		return screen_frame_buffer;
	}

	ConstantBuffer* CreateConstantBuffer(const void* Contents, Buffer::Usage Usage, const ShaderParametersMetadata& Layout)
	{
		auto& Device = GRenderIF->GetDevice();

		Usage = Buffer::Usage::Static;

		return Device.CreateConstanBuffer(Usage,Layout.GetSize(),Contents);
	}

	GraphicsBuffer* CreateVertexBuffer(white::span<const std::byte> Contents, Buffer::Usage Usage, uint32 Access)
	{
		auto& Device = Context::Instance().GetDevice();

		return Device.CreateVertexBuffer(Usage, Access,static_cast<uint32>(Contents.size()), EFormat::EF_Unknown, Contents.data());
	}
}
