#pragma once

#include <WBase/winttype.hpp>
#include "d3d12_dxgi.h"
#include "WBase/enum.hpp"


namespace platform_ex::Windows::D3D12 {
	using white::has_anyflags;

	/** Find the appropriate depth-stencil typeless DXGI format for the given format. */
	inline DXGI_FORMAT FindDepthStencilParentDXGIFormat(DXGI_FORMAT InFormat)
	{
		switch (InFormat)
		{
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
		case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
			return DXGI_FORMAT_R24G8_TYPELESS;
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
		case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
			return DXGI_FORMAT_R32G8X24_TYPELESS;
		case DXGI_FORMAT_D32_FLOAT:
			return DXGI_FORMAT_R32_TYPELESS;
		case DXGI_FORMAT_D16_UNORM:
			return DXGI_FORMAT_R16_TYPELESS;
		};
		return InFormat;
	}

	/** Find the appropriate depth-stencil targetable DXGI format for the given format. */
	inline DXGI_FORMAT FindDepthStencilDXGIFormat(DXGI_FORMAT InFormat)
	{
		switch (InFormat)
		{
		case DXGI_FORMAT_R24G8_TYPELESS:
			return DXGI_FORMAT_D24_UNORM_S8_UINT;
			// Changing Depth Buffers to 32 bit on Dingo as D24S8 is actually implemented as a 32 bit buffer in the hardware
		case DXGI_FORMAT_R32G8X24_TYPELESS:
			return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
		case DXGI_FORMAT_R32_TYPELESS:
			return DXGI_FORMAT_D32_FLOAT;
		case DXGI_FORMAT_R16_TYPELESS:
			return DXGI_FORMAT_D16_UNORM;
		};
		return InFormat;
	}

	inline uint8 GetPlaneSliceFromViewFormat(DXGI_FORMAT ResourceFormat, DXGI_FORMAT ViewFormat)
	{
		// Currently, the only planar resources used are depth-stencil formats
		switch (FindDepthStencilParentDXGIFormat(ResourceFormat))
		{
		case DXGI_FORMAT_R24G8_TYPELESS:
			switch (ViewFormat)
			{
			case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
				return 0;
			case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
				return 1;
			}
			break;
		case DXGI_FORMAT_R32G8X24_TYPELESS:
			switch (ViewFormat)
			{
			case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
				return 0;
			case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
				return 1;
			}
			break;
		}

		return 0;
	}

	inline uint8 GetPlaneCount(DXGI_FORMAT Format)
	{
		// Currently, the only planar resources used are depth-stencil formats
		// Note there is a D3D12 helper for this, D3D12GetFormatPlaneCount
		switch (FindDepthStencilParentDXGIFormat(Format))
		{
		case DXGI_FORMAT_R24G8_TYPELESS:
		case DXGI_FORMAT_R32G8X24_TYPELESS:
			return 2;
		default:
			return 1;
		}
	}

	struct ShaderBytecodeHash
	{
		uint64 Hash[2];

		bool operator ==(const ShaderBytecodeHash& b) const
		{
			return (Hash[0] == b.Hash[0] && Hash[1] == b.Hash[1]);
		}

		bool operator !=(const ShaderBytecodeHash& b) const
		{
			return (Hash[0] != b.Hash[0] || Hash[1] != b.Hash[1]);
		}
	};

	class ComputeHWShader;
	struct QuantizedBoundShaderState;

	void QuantizeBoundShaderState(
		const D3D12_RESOURCE_BINDING_TIER& ResourceBindingTier,
		const ComputeHWShader* const ComputeShader,
		QuantizedBoundShaderState& OutQBSS
	);

	static bool IsDirectQueueExclusiveD3D12State(D3D12_RESOURCE_STATES InState)
	{
		return has_anyflags(InState, D3D12_RESOURCE_STATE_RENDER_TARGET | D3D12_RESOURCE_STATE_DEPTH_WRITE | D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}
}