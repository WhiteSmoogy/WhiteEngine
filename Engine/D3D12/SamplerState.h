#pragma once

#include "d3d12_dxgi.h"
#include "Common.h"
#include "Descriptors.h"

namespace platform_ex::Windows::D3D12 {
	class SamplerState : public DeviceChild
	{
	public:
		OfflineDescriptor Descriptor;
		const uint16 ID;

		SamplerState(NodeDevice* InParent, const D3D12_SAMPLER_DESC& Desc, uint16 SamplerID);
		~SamplerState();
	};
}
