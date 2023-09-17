/*! \file Engine\Render\D3D12\Convert.h
\ingroup Engine
\brief 把上层转换至D3D12层。
*/

#ifndef WE_RENDER_D3D12_Convert_h
#define WE_RENDER_D3D12_Convert_h 1

#include "RenderInterface/IFormat.hpp"
#include "RenderInterface/PipleState.h"
#include "RenderInterface/InputLayout.hpp"
#include "RenderInterface/IGraphicsPipelineState.h"
#include <dxgiformat.h>
#include "d3d12_dxgi.h"

namespace platform_ex {
	namespace Windows {
		namespace D3D12 {
			DXGI_FORMAT Convert(platform::Render::EFormat format);
			platform::Render::EFormat Convert(DXGI_FORMAT format);

			D3D12_SAMPLER_DESC Convert(platform::Render::TextureSampleDesc desc);

			D3D12_TEXTURE_ADDRESS_MODE Convert(platform::Render::TexAddressingMode mode);
			D3D12_FILTER Convert(platform::Render::TexFilterOp op);
			D3D12_COMPARISON_FUNC Convert(platform::Render::CompareOp op);

			std::vector<D3D12_INPUT_ELEMENT_DESC> Convert(const platform::Render::Vertex::Stream& stream);

			template<typename T> 
			T Convert(platform::Render::PrimtivteType type);

			D3D12_PRIMITIVE_TOPOLOGY_TYPE Convert(platform::Render::PrimtivteType type);

			D3D12_BLEND_DESC Convert(const platform::Render::BlendDesc& desc);
			D3D12_RASTERIZER_DESC Convert(const platform::Render::RasterizerDesc& desc);
			D3D12_DEPTH_STENCIL_DESC Convert(const platform::Render::DepthStencilDesc& desc);

			D3D12_BLEND_OP Convert(platform::Render::BlendOp op);
			D3D12_BLEND Convert(platform::Render::BlendFactor factor);

			D3D12_FILL_MODE Convert(platform::Render::RasterizerMode mode);
			D3D12_CULL_MODE Convert(platform::Render::CullMode mode);

			D3D12_STENCIL_OP Convert(platform::Render::StencilOp op);

			inline D3D12_DESCRIPTOR_HEAP_TYPE Convert(platform::Render::DescriptorHeapType InHeapType)
			{
				switch (InHeapType)
				{
				case platform::Render::DescriptorHeapType::Standard:     return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
				case platform::Render::DescriptorHeapType::RenderTarget: return D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
				case platform::Render::DescriptorHeapType::DepthStencil: return D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
				case platform::Render::DescriptorHeapType::Sampler:      return D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
				}

				return D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
			}
		}
	}
}

#endif