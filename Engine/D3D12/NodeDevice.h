#pragma once

#include <WBase/id.hpp>
#include "Common.h"
#include "d3d12_dxgi.h"
#include "View.h"
#include "DescriptorCache.h"
#include "Allocation.h"
#include "Descriptors.h"
#include "NodeQueue.h"

#include <vector>
#include <unordered_map>

class RayTracingDescriptorHeapCache;

struct SamplerDescHash
{
	static_assert(sizeof(D3D12_SAMPLER_DESC) % 4 == 0);

	std::size_t operator()(const D3D12_SAMPLER_DESC& desc) const noexcept {
		auto ptr = reinterpret_cast<const char*>(&desc);
		return CityHash32(ptr,sizeof(desc));
	}
};

inline bool operator==(const D3D12_SAMPLER_DESC& lhs, const D3D12_SAMPLER_DESC& rhs)
{
	return std::memcmp(&lhs, &rhs, sizeof(lhs)) == 0;
}

namespace platform_ex::Windows::D3D12
{
	class CommandContext;

	class NodeDevice : public SingleNodeGPUObject, public AdapterChild
	{
	public:
		NodeDevice(GPUMaskType InGpuMask, D3D12Adapter* InAdapter);

		void Initialize();

		void CreateDefaultViews();

		ID3D12Device* GetDevice();

		ID3D12Device5* GetRayTracingDevice();

		void InitRayTracing();
		RayTracingDescriptorHeapCache* GetRayTracingDescriptorHeapCache() { return RayTracingDescriptorHeapCache; }

		CommandContext& GetDefaultCommandContext()
		{
			return *ImmediateCommandContext;
		}

		void CreateSamplerInternal(const D3D12_SAMPLER_DESC& Desc, D3D12_CPU_DESCRIPTOR_HANDLE Descriptor);

		std::shared_ptr<SamplerState> CreateSampler(const D3D12_SAMPLER_DESC& Desc);

		inline GlobalOnlineSamplerHeap& GetGlobalSamplerHeap() { return GlobalSamplerHeap; }

		FastAllocator& GetDefaultFastAllocator() {
			return DefaultFastAllocator;
		}

		BufferAllocator& GetDefaultBufferAllocator() {
			return DefaultBufferAllocator;
		}

		DescriptorHeapManager& GetDescriptorHeapManager() { return NormalDescriptorHeapManager; }

		OnlineDescriptorManager& GetOnlineDescriptorManager() { return OnlineDescriptorManager; }

		OfflineDescriptorManager& GetOfflineDescriptorManager(DescriptorHeapType InType)
		{
			return OfflineDescriptorManagers[static_cast<int>(InType)];
		}

		const D3DDefaultViews& GetDefaultViews() const { return DefaultViews; }

		CommandAllocator* ObtainCommandAllocator(QueueType Type);
		void ReleaseCommandAllocator(CommandAllocator* Allocator);

		CommandList* ObtainCommandList(CommandAllocator* CommandAllocator);
		void ReleaseCommandList(CommandList* Cmd);

		NodeQueue& GetQueue(QueueType QueueType) { return Queues[(uint32)QueueType]; }

	private:
		void SetupAfterDeviceCreation();
	protected:
		// Must be before the StateCache so that destructor ordering is valid
		std::vector<OfflineDescriptorManager> OfflineDescriptorManagers;
		OnlineDescriptorManager OnlineDescriptorManager;

		CommandContext* ImmediateCommandContext = nullptr;

		std::unordered_map<D3D12_SAMPLER_DESC, std::shared_ptr<SamplerState>, SamplerDescHash> SamplerMap;
		uint32 SamplerID;

		GlobalOnlineSamplerHeap GlobalSamplerHeap;

		DescriptorHeapManager NormalDescriptorHeapManager;
		D3DDefaultViews DefaultViews;

		FastAllocator DefaultFastAllocator;
		BufferAllocator DefaultBufferAllocator;

		std::vector<NodeQueue> Queues;

		RayTracingDescriptorHeapCache* RayTracingDescriptorHeapCache = nullptr;
		void DestroyRayTracingDescriptorCache();
	};
}