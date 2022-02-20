/*! \file Engine\Render\D3D12\GraphicsBuffer.hpp
\ingroup Engine
\brief D3D12 Buffer½Ó¿ÚÀà¡£
*/
#ifndef WE_RENDER_D3D12_GraphicsBuffer_hpp
#define WE_RENDER_D3D12_GraphicsBuffer_hpp 1

#include "../IFormat.hpp"
#include "../IGraphicsBuffer.hpp"
#include "ResourceHolder.h"
#include "WFramework/Win32/WCLib/COM.h"
#include "Allocation.h"

namespace platform_ex::Windows::D3D12 {

	class ShaderResourceView;
	class UnorderedAccessView;
	class RenderTargetView;

	class GraphicsBuffer : public platform::Render::GraphicsBuffer,public ResourceHolder {
	public:
		GraphicsBuffer(platform::Render::Buffer::Usage usage, uint32_t access_hint,
			uint32_t size_in_byte,platform::Render::EFormat fmt);
		~GraphicsBuffer() override;

		void CopyToBuffer(platform::Render::GraphicsBuffer& rhs) override;

		void HWResourceCreate(void const * init_data) override;
		void HWResourceDelete() override;

		void UpdateSubresource(white::uint32 offset, white::uint32 size, void const * data) override;

		ID3D12Resource* UploadResource() const;

		RenderTargetView* RetriveRenderTargetView(uint16 width, uint16 height, platform::Render::EFormat pf);
		ShaderResourceView* RetriveShaderResourceView();
		UnorderedAccessView* RetriveUnorderedAccessView();

		platform::Render::EFormat GetFormat() const { return format; }
	private:
		void* Map(platform::Render::Buffer::Access ba) override;
		void Unmap() override;

		friend class Device;
		friend class Context;

		std::unique_ptr<ShaderResourceView> srv;
		std::unique_ptr<UnorderedAccessView> uav;
		std::unique_ptr<std::unordered_map<std::size_t, std::unique_ptr<RenderTargetView>>> rtv_maps;

		COMPtr<ID3D12Resource> buffer_counter_upload;
		white::uint32 counter_offset;

		platform::Render::EFormat format;
	};

	class ConstantBuffer:public platform::Render::ConstantBuffer,public DeviceChild
	{
	public:
		using UsageType = platform::Render::Buffer::Usage;

		ResourceLocation Location;

		const UsageType Usage;

		ConstantBuffer(NodeDevice* InParent, UsageType InUsage)
			:DeviceChild(InParent),Location(InParent), Usage(InUsage)
		{}

		void Update(white::uint32 size, void const* data) final;
	};
}

#endif