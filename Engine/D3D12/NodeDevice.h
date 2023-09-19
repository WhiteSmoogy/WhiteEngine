#pragma once

#include <WBase/id.hpp>
#include "Common.h"
#include "d3d12_dxgi.h"
#include "View.h"
#include "DescriptorCache.h"
#include "Allocation.h"
#include "Descriptors.h"

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

		void CreateCommandContexts();

		ID3D12Device* GetDevice();

		ID3D12Device5* GetRayTracingDevice();

		void InitRayTracing();
		RayTracingDescriptorHeapCache* GetRayTracingDescriptorHeapCache() { return RayTracingDescriptorHeapCache; }

		CommandContext& GetDefaultCommandContext()
		{
			return *CommandContextArray[0];
		}

		uint32 GetNumContexts() const
		{
			return static_cast<uint32>(CommandContextArray.size());
		}

		CommandContext& GetCommandContext(int index)
		{
			return *CommandContextArray[index];
		}

		ID3D12CommandQueue* GetD3DCommandQueue(CommandQueueType InQueueType);

		CommandListManager* GetCommandListManager(CommandQueueType InQueueType);

		CommandListManager& GetCommandListManager() const { return *CommandListManager; }

		std::vector<CommandListHandle> PendingCommandLists;

		void CreateSamplerInternal(const D3D12_SAMPLER_DESC& Desc, D3D12_CPU_DESCRIPTOR_HANDLE Descriptor);

		std::shared_ptr<SamplerState> CreateSampler(const D3D12_SAMPLER_DESC& Desc);

		inline GlobalOnlineSamplerHeap& GetGlobalSamplerHeap() { return GlobalSamplerHeap; }

		FastAllocator& GetDefaultFastAllocator() {
			return DefaultFastAllocator;
		}

		BufferAllocator& GetDefaultBufferAllocator() {
			return DefaultBufferAllocator;
		}

		inline DescriptorHeapManager& GetDescriptorHeapManager() { return NormalDescriptorHeapManager; }

		inline OfflineDescriptorManager& GetOfflineDescriptorManager(DescriptorHeapType InType)
		{
			return OfflineDescriptorManagers[static_cast<int>(InType)];
		}

	private:
		void SetupAfterDeviceCreation();
	protected:
		using D3D12CommandListManager = CommandListManager;
		/** A pool of command lists we can cycle through for the global D3D device */
		D3D12CommandListManager* CommandListManager;
		D3D12CommandListManager* CopyCommandListManager;
		D3D12CommandListManager* AsyncCommandListManager;

		// Must be before the StateCache so that destructor ordering is valid
		std::vector<OfflineDescriptorManager> OfflineDescriptorManagers;

		std::vector<CommandContext*> CommandContextArray;

		std::unordered_map<D3D12_SAMPLER_DESC, std::shared_ptr<SamplerState>, SamplerDescHash> SamplerMap;
		uint32 SamplerID;

		GlobalOnlineSamplerHeap GlobalSamplerHeap;

		DescriptorHeapManager NormalDescriptorHeapManager;

		FastAllocator DefaultFastAllocator;
		BufferAllocator DefaultBufferAllocator;

		RayTracingDescriptorHeapCache* RayTracingDescriptorHeapCache = nullptr;
		void DestroyRayTracingDescriptorCache();
	};
}