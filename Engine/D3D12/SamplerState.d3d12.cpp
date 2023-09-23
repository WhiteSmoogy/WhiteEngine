#include "SamplerState.h"
#include "NodeDevice.h"
#include <mutex>

using namespace platform_ex::Windows::D3D12;

SamplerState::SamplerState(NodeDevice* InParent, const D3D12_SAMPLER_DESC& Desc, uint16 SamplerID)
	:DeviceChild(InParent)
	, ID(SamplerID)
{
	auto& DescriptorAllocator = GetParentDevice()->GetOfflineDescriptorManager(DescriptorHeapType::Sampler);
	Descriptor = DescriptorAllocator.AllocateHeapSlot();

	GetParentDevice()->CreateSamplerInternal(Desc, Descriptor);
}

SamplerState::~SamplerState()
{
	if (Descriptor)
	{
		auto& DescriptorAllocator = GetParentDevice()->GetOfflineDescriptorManager(DescriptorHeapType::Sampler);
		DescriptorAllocator.FreeHeapSlot(Descriptor);
	}
}

std::mutex GD3D12SamplerStateCacheLock;

std::shared_ptr<SamplerState> NodeDevice::CreateSampler(const D3D12_SAMPLER_DESC& Desc)
{
	std::unique_lock Lock(GD3D12SamplerStateCacheLock);

	auto PreviouslyCreated = SamplerMap.find(Desc);

	if (PreviouslyCreated != SamplerMap.end())
		return PreviouslyCreated->second;
	else {
		// 16-bit IDs are used for faster hashing
		wconstraint(SamplerID < 0xffff);

		auto NewSampler =std::make_shared<SamplerState>(this, Desc, static_cast<uint16>(SamplerID));

		SamplerMap.emplace(Desc, NewSampler);

		SamplerID++;

		return NewSampler;
	}
}
