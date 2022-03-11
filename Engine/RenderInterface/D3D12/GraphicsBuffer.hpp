/*! \file Engine\Render\D3D12\GraphicsBuffer.hpp
\ingroup Engine
\brief D3D12 Buffer½Ó¿ÚÀà¡£
*/
#ifndef WE_RENDER_D3D12_GraphicsBuffer_hpp
#define WE_RENDER_D3D12_GraphicsBuffer_hpp 1

#include "../IFormat.hpp"
#include "../IGraphicsBuffer.hpp"
#include "WFramework/Win32/WCLib/COM.h"
#include "Allocation.h"

namespace platform_ex::Windows::D3D12 {

	class ShaderResourceView;
	class UnorderedAccessView;
	class RenderTargetView;

	class GraphicsBuffer : public platform::Render::GraphicsBuffer,public DeviceChild,public ResourceLocationTrait{
	public:
		GraphicsBuffer(NodeDevice* Parent, platform::Render::Buffer::Usage usage, uint32_t access_hint,
			uint32_t size_in_byte,DXGI_FORMAT InFormat);
		~GraphicsBuffer() override;

		void CopyToBuffer(platform::Render::GraphicsBuffer& rhs) override;

		void UpdateSubresource(white::uint32 offset, white::uint32 size, void const * data) override;

		ID3D12Resource* UploadResource() const;

		RenderTargetView* RetriveRenderTargetView(uint16 width, uint16 height, platform::Render::EFormat pf);
		ShaderResourceView* RetriveShaderResourceView();
		UnorderedAccessView* RetriveUnorderedAccessView();

		DXGI_FORMAT GetFormat() const { return format; }
	private:
		void* Map(platform::Render::Buffer::Access ba) override;
		void Unmap() override;

		friend class Device;
		friend class Context;

		std::unique_ptr<ShaderResourceView> srv;
		std::unique_ptr<UnorderedAccessView> uav;
		std::unique_ptr<std::unordered_map<std::size_t, std::unique_ptr<RenderTargetView>>> rtv_maps;

		DXGI_FORMAT format;

		uint32 Alignment = 0;
		uint32 Stride = 0;
	};
}

#endif