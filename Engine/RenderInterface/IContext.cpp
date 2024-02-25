#include "IFrameBuffer.h"
#include "IContext.h"

platform::Render::Context* GRenderIF;
platform::Render::DeviceCaps platform::Render::Caps;

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

platform::Render::Device* platform::Render::GDevice = nullptr;

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

	GraphicsBuffer* CreateVertexBuffer(white::span<const std::byte> Contents, Buffer::Usage Usage, uint32 Access)
	{
		ResourceCreateInfoEx CreateInfo(Contents.data(), "VertexBuffer");

		return GDevice->CreateVertexBuffer(Usage, Access,static_cast<uint32>(Contents.size()), EFormat::EF_Unknown, CreateInfo);
	}

	GraphicsBuffer* CreateBuffer(Buffer::Usage usage, white::uint32 access, uint32 size_in_byte, uint32 stride, ResourceCreateInfo init_data)
	{
		return GDevice->CreateBuffer(usage, access, size_in_byte, stride, init_data);
	}
}
