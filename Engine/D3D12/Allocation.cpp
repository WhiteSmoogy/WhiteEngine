#include "NodeDevice.h"
#include "Context.h"
#include "Core/Container/vector.hpp"
#include <spdlog/spdlog.h>
#include <ranges>
using namespace platform_ex::Windows::D3D12;

int32 FastConstantAllocatorPageSize = 64 * 1024;
int32 UploadHeapSmallBlockMaxAllocationSize = 64 * 1024;
int32 UploadHeapSmallBlockPoolSize = 4 * 1024 * 1024;
int32 UploadHeapBigBlockHeapSize = 8*1024*1024;
int32 UploadHeapBigBlockMaxAllocationSize = 64 * 1024 * 1024;
int32 FastAllocatorMinPagesToRetain = 5;
int32 PoolAllocatorBufferPoolSize = 32 * 1024 * 1024;
int32 PoolAllocatorBufferMaxAllocationSize = 16 * 1024 * 1024;

constexpr uint32 READBACK_BUFFER_POOL_DEFAULT_POOL_SIZE = 4 * 1024 * 1024;
constexpr uint32 MIN_PLACED_RESOURCE_SIZE = 64 * 1024;
constexpr uint32 READBACK_BUFFER_POOL_MAX_ALLOC_SIZE = (64 * 1024);

const char* GetMemoryPoolDebugName(const AllocatorConfig& InConfig)
{
	if (white::has_anyflags(InConfig.InitialResourceState, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE))
		return "/Resource Allocator(MemoryPool) Underlying Buffer(ACCELERATION_STRUCTURE)";
	else if(white::has_anyflags(InConfig.InitialResourceState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS))
		return "/Resource Allocator(MemoryPool) Underlying Buffer(UNORDERED_ACCESS)";
	return "/Resource Allocator(MemoryPool) Underlying Buffer";
}

MemoryPool::MemoryPool(NodeDevice* InParentDevice, GPUMaskType VisibleNodes, 
	const AllocatorConfig& InConfig, const std::string& InName, AllocationStrategy InAllocationStrategy,
	uint16 InPoolIndex, uint64 InPoolSize, uint32 InPoolAlignment, 
	PoolResouceTypes InAllocationResourceType, FreeListOrder InFreeListOrder)
	:DeviceChild(InParentDevice), MultiNodeGPUObject(InParentDevice->GetGPUMask(), VisibleNodes)
	,PoolIndex(InPoolIndex),PoolSize(InPoolSize),PoolAlignment(InPoolAlignment),SupportResouceTypes(InAllocationResourceType),ListOrder(InFreeListOrder),
	InitConfig(InConfig), Name(InName), Strategy(InAllocationStrategy), LastUsedFrameFence(0)
{
	//create device object
	if (PoolSize == 0)
	{
		return;
	}

	auto Device = GetParentDevice();
	auto Adapter = Device->GetParentAdapter();

	if (Strategy == AllocationStrategy::kPlacedResource)
	{
		// Alignment should be either 4K or 64K for places resources
		wconstraint(PoolAlignment == D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT || PoolAlignment == D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

		D3D12_HEAP_PROPERTIES HeapProps = CD3DX12_HEAP_PROPERTIES(InitConfig.HeapType);
		HeapProps.CreationNodeMask = GetGPUMask().GetNative();
		HeapProps.VisibleNodeMask = GetVisibilityMask().GetNative();

		D3D12_HEAP_DESC Desc = {};
		Desc.SizeInBytes = PoolSize;
		Desc.Properties = HeapProps;
		Desc.Alignment = 0;
		Desc.Flags = InitConfig.HeapFlags;
		if (Adapter->IsHeapNotZeroedSupported())
		{
			Desc.Flags |= D3D12_HEAP_FLAG_CREATE_NOT_ZEROED;
		}

		ID3D12Heap* Heap = nullptr;
		{
			CheckHResult(Adapter->GetDevice()->CreateHeap(&Desc, IID_PPV_ARGS(&Heap)));
		}
		D3D::Debug(Heap, std::format("{}/LinkListAllocator Backing Heap",Name).c_str());

		BackingHeap = new HeapHolder(GetParentDevice(), GetVisibilityMask());
		BackingHeap->SetHeap(Heap);

		// Only track resources that cannot be accessed on the CPU.
		if (IsGPUOnly(InitConfig.HeapType))
		{
		}
	}
	else
	{
		{
			const D3D12_HEAP_PROPERTIES HeapProps = CD3DX12_HEAP_PROPERTIES(InitConfig.HeapType, GetGPUMask().GetNative(), GetVisibilityMask().GetNative());
			CheckHResult(Adapter->CreateBuffer(HeapProps, GetGPUMask(), 
				InitConfig.InitialResourceState, InitConfig.InitialResourceState, PoolSize, BackingResource.ReleaseAndGetAddress(), 
				std::format("{}/{}", Name, GetMemoryPoolDebugName(InitConfig)).c_str(), InitConfig.ResourceFlags));
		}

		if (IsCPUAccessible(InitConfig.HeapType))
		{
			BackingResource->Map();
		}
	}

	FreeSize = PoolSize;
	for (auto i : std::ranges::iota_view(0, 32)) {
		auto AllocationData = new PoolAllocatorPrivateData();
		AllocationData->Reset();
		AllocationDataPool.emplace_back(AllocationData);
	}

	// Setup the special start and free block
	HeadBlock.InitAsHead(PoolIndex);
	PoolAllocatorPrivateData* FreeBlock = GetNewAllocationData();
	FreeBlock->InitAsFree(PoolIndex,static_cast<uint32>(PoolSize), PoolAlignment, 0);
	HeadBlock.AddAfter(FreeBlock);
	FreeBlocks.emplace_back(FreeBlock);
}

bool MemoryPool::TryAllocate(uint32 InSizeInBytes, uint32 InAllocationAlignment, PoolResouceTypes InAllocationResourceType, PoolAllocatorPrivateData& AllocationData)
{
	wconstraint(IsResourceTypeSupported(InAllocationResourceType));

	int32 FreeBlockIndex = FindFreeBlock(InSizeInBytes, InAllocationAlignment);
	if (FreeBlockIndex != white::INDEX_NONE)
	{
		uint32 AlignedSize = PoolAllocatorPrivateData::GetAlignedSize(InSizeInBytes, PoolAlignment, InAllocationAlignment);

		PoolAllocatorPrivateData* FreeBlock = FreeBlocks[FreeBlockIndex];
		wconstraint(FreeBlock->GetSize() >= AlignedSize);

		// Remove from the free blocks because size will change and need sorted insert again then
		FreeBlocks.erase(FreeBlocks.begin()+FreeBlockIndex);

		// update private allocator data of new and free block
		AllocationData.InitAsAllocated(InSizeInBytes, InAllocationAlignment, FreeBlock);
		wconstraint((AllocationData.GetOffset() % InAllocationAlignment) == 0);

		// Update working stats
		wconstraint(FreeSize >= AlignedSize);
		FreeSize -= AlignedSize;

		// Free block is empty then release otherwise sorted reinsert 
		if (FreeBlock->GetSize() == 0)
		{
			FreeBlock->RemoveFromLinkedList();
			ReleaseAllocationData(FreeBlock);
		}
		else
		{
			AddToFreeBlocks(FreeBlock);
		}

		return true;
	}
	else
	{
		return false;
	}
}

int32 MemoryPool::FindFreeBlock(uint32 InSizeInBytes, uint32 InAllocationAlignment)
{
	uint32 AlignedSize = PoolAllocatorPrivateData::GetAlignedSize(InSizeInBytes, PoolAlignment, InAllocationAlignment);

	// Early out if total free size doesn't fit
	if (FreeSize < AlignedSize)
	{
		return white::INDEX_NONE;
	}


	auto itr = std::lower_bound(FreeBlocks.begin(), FreeBlocks.end(), nullptr, [=](PoolAllocatorPrivateData* l, PoolAllocatorPrivateData* r) {
		return l->GetSize() < AlignedSize;
		});

	if(itr == FreeBlocks.end())
		return white::INDEX_NONE;

	return static_cast<int32>(std::distance(FreeBlocks.begin(), itr));
}

void MemoryPool::RemoveFromFreeBlocks(PoolAllocatorPrivateData* InFreeBlock)
{
	for (auto itr = FreeBlocks.begin(); itr != FreeBlocks.end(); ++itr)
	{
		if (*itr == InFreeBlock)
		{
			itr = FreeBlocks.erase(itr);
			return;
		}
	}
}

void MemoryPool::Deallocate(PoolAllocatorPrivateData& AllocationData)
{
	wconstraint(AllocationData.IsLocked());
	wconstraint(PoolIndex == AllocationData.GetPoolIndex());

	// Free block should not be locked anymore - can be reused immediatly when we get to actual pool deallocate
	bool bLocked = false;
	uint64 AllocationSize = AllocationData.GetSize();

	auto FreeBlock = GetNewAllocationData();
	FreeBlock->MoveFrom(AllocationData, bLocked);
	FreeBlock->MarkFree(PoolAlignment);

	// Update working stats
	FreeSize += FreeBlock->GetSize();

	AddToFreeBlocks(FreeBlock);
}

PoolAllocatorPrivateData* MemoryPool::AddToFreeBlocks(PoolAllocatorPrivateData* InFreeBlock)
{
	wconstraint(InFreeBlock->IsFree());
	wconstraint(IsAligned(InFreeBlock->GetSize(), PoolAlignment));
	wconstraint(IsAligned(InFreeBlock->GetOffset(), PoolAlignment));

	auto FreeBlock = InFreeBlock;

	// Coalesce with previous?
	if (FreeBlock->GetPrev()->IsFree() && !FreeBlock->GetPrev()->IsLocked())
	{
		auto PrevFree = FreeBlock->GetPrev();
		PrevFree->Merge(FreeBlock);
		RemoveFromFreeBlocks(PrevFree);
		ReleaseAllocationData(FreeBlock);

		FreeBlock = PrevFree;
	}

	// Coalesce with next?
	if (FreeBlock->GetNext()->IsFree() && !FreeBlock->GetNext()->IsLocked())
	{
		auto NextFree = FreeBlock->GetNext();
		FreeBlock->Merge(NextFree);
		RemoveFromFreeBlocks(NextFree);
		ReleaseAllocationData(NextFree);
	}

	auto pos = std::lower_bound(FreeBlocks.begin(), FreeBlocks.end(), FreeBlock, 
		ListOrder == FreeListOrder::SortBySize ? 
		[](PoolAllocatorPrivateData* l, PoolAllocatorPrivateData* r) {
		return l->GetSize() < r->GetSize();
		} : 
		[](PoolAllocatorPrivateData* l, PoolAllocatorPrivateData* r) {
			return l->GetOffset() < r->GetOffset();
		});

	FreeBlocks.insert(pos, FreeBlock);

	return FreeBlock;
}

PoolAllocatorPrivateData* MemoryPool::GetNewAllocationData()
{
	return AllocationDataPool.empty() ? new PoolAllocatorPrivateData() : white::pop(AllocationDataPool);
}

void MemoryPool::ReleaseAllocationData(PoolAllocatorPrivateData* InData)
{
	InData->Reset();
	if (AllocationDataPool.size() >= 32)
		delete InData;
	else
		AllocationDataPool.push_back(InData);
}

AllocatorConfig IPoolAllocator::GetResourceAllocatorInitConfig(D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_FLAGS InResourceFlags, uint32 InBufferAccess)
{
	AllocatorConfig InitConfig;
	InitConfig.HeapType = InHeapType;
	InitConfig.ResourceFlags = InResourceFlags;

	// Setup initial resource state depending on the requested buffer flags
	if (white::has_anyflags(InBufferAccess, EA_AccelerationStructure))
	{
		// should only have this flag and no other flags
		wconstraint(InBufferAccess == EA_AccelerationStructure);
		InitConfig.InitialResourceState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
	}
	else
		if (InitConfig.HeapType == D3D12_HEAP_TYPE_READBACK)
		{
			InitConfig.InitialResourceState = D3D12_RESOURCE_STATE_COPY_DEST;
		}
		else
		{
			if (white::has_anyflags(InBufferAccess, EA_GPUUnordered))
			{
				wconstraint(InResourceFlags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
			}
			InitConfig.InitialResourceState = D3D12_RESOURCE_STATE_COMMON;
		}

	InitConfig.HeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
	if (white::has_anyflags(InBufferAccess, EA_DrawIndirect))
	{
		wconstraint(InResourceFlags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		InitConfig.HeapFlags |= D3D12_HEAP_FLAG_NONE;
	}

	return InitConfig;
}

template<MemoryPool::FreeListOrder Order, bool Defrag>
PoolAllocator<Order, Defrag>::PoolAllocator(NodeDevice* InParentDevice, GPUMaskType VisibleNodes,
	const AllocatorConfig& InConfig, const std::string& InName, AllocationStrategy InAllocationStrategy,
	uint64 InDefaultPoolSize, uint32 InPoolAlignment, uint32 InMaxAllocationSize)
	:DeviceChild(InParentDevice), MultiNodeGPUObject(InParentDevice->GetGPUMask(), VisibleNodes),
	DefaultPoolSize(InDefaultPoolSize), PoolAlignment(InPoolAlignment), MaxAllocationSize(InMaxAllocationSize),
	InitConfig(InConfig), Name(InName), Strategy(InAllocationStrategy), PoolNewCount(0)
{
}

template<MemoryPool::FreeListOrder Order, bool Defrag>
PoolAllocator<Order, Defrag>::~PoolAllocator()
{
	for (auto& Pool : Pools)
		delete Pool;
}

template<MemoryPool::FreeListOrder Order, bool Defrag>
void PoolAllocator<Order, Defrag>::CleanUpAllocations(uint64 InFrameLag)
{
	std::unique_lock Lock{ CS };

	auto Adapter = GetParentDevice()->GetParentAdapter();
	auto& FrameFence = Adapter->GetFrameFence();

	uint32 PopCount = 0;
	for (size_t i = 0; i < FrameFencedOperations.size(); i++)
	{
		FrameFencedAllocationData& Operation = FrameFencedOperations[i];
		if (FrameFence.IsFenceComplete(Operation.FrameFence))
		{
			switch (Operation.Operation)
			{
			case FrameFencedAllocationData::EOperation::Deallocate:
			{
				// Deallocate the locked block (actually free now)
				DeallocateInternal(*Operation.AllocationData);
				Operation.AllocationData->Reset();
				ReleaseAllocationData(Operation.AllocationData);

				// Free placed resource if created
				if (Strategy == AllocationStrategy::kPlacedResource)
				{
					// Release the resource
					wconstraint(Operation.PlacedResource != nullptr);
					Operation.PlacedResource->Release();
					Operation.PlacedResource = nullptr;
				}
				else
				{
					wconstraint(Operation.PlacedResource == nullptr);
				}
				break;
			}
			case FrameFencedAllocationData::EOperation::Unlock:
			{
				Operation.AllocationData->UnLock();
				break;
			}
			case FrameFencedAllocationData::EOperation::Nop:
			{
				break;
			}
			default: wconstraint(false);
			}

			PopCount =static_cast<int>(i + 1);
		}
		else
		{
			break;
		}
	}

	if (PopCount)
	{
		FrameFencedOperations.erase(FrameFencedOperations.begin(), FrameFencedOperations.begin() + PopCount);
	}

	// Trim empty allocators if not used in last n frames
	const uint64 CompletedFence = FrameFence.UpdateLastCompletedFence();
	for (size_t PoolIndex = 0; PoolIndex < Pools.size(); ++PoolIndex)
	{
		auto Pool = Pools[PoolIndex];
		if (Pool != nullptr && Pool->IsEmpty() && (Pool->GetLastUsedFrameFence() + InFrameLag <= CompletedFence))
		{
			delete(Pool);
			Pools[PoolIndex] = nullptr;
		}
	}

	// Update the allocation order
	std::sort(PoolAllocationOrder.begin(), PoolAllocationOrder.end() ,[this](uint32 InLHS, uint32 InRHS)
		{
			// first allocate from 'fullest' pools when defrag is enabled, otherwise try and allocate from
			// default sized pools with stable order
			if (Defrag)
			{
				uint64 LHSUsePoolSize = Pools[InLHS] ? Pools[InLHS]->GetUsedSize() : UINT32_MAX;
				uint64 RHSUsePoolSize = Pools[InRHS] ? Pools[InRHS]->GetUsedSize() : UINT32_MAX;
				return LHSUsePoolSize > RHSUsePoolSize;
			}
			else
			{
				uint64 LHSPoolSize = Pools[InLHS] ? Pools[InLHS]->GetPoolSize() : UINT32_MAX;
				uint64 RHSPoolSize = Pools[InRHS] ? Pools[InRHS]->GetPoolSize() : UINT32_MAX;
				if (LHSPoolSize != RHSPoolSize)
				{
					return LHSPoolSize < RHSPoolSize;
				}

				uint64 LHSPoolIndex = Pools[InLHS] ? Pools[InLHS]->GetPoolIndex() : UINT32_MAX;
				uint64 RHSPoolIndex = Pools[InRHS] ? Pools[InRHS]->GetPoolIndex() : UINT32_MAX;
				return LHSPoolIndex < RHSPoolIndex;
			}
		});
}

template<MemoryPool::FreeListOrder Order, bool Defrag>
bool PoolAllocator<Order, Defrag>::SupportsAllocation(D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_FLAGS InResourceFlags, uint32 InBufferAccess, ResourceStateMode InResourceStateMode) const
{
	auto InInitConfig = GetResourceAllocatorInitConfig(InHeapType, InResourceFlags, InBufferAccess);
	auto InAllocationStrategy = GetResourceAllocationStrategy(InResourceFlags, InResourceStateMode);
	return (InitConfig == InInitConfig && Strategy == InAllocationStrategy);
}

template<MemoryPool::FreeListOrder Order, bool Defrag>
void PoolAllocator<Order, Defrag>::AllocDefaultResource(D3D12_HEAP_TYPE InHeapType, const D3D12_RESOURCE_DESC& InDesc, uint32 InBufferAccess, ResourceStateMode InResourceStateMode, D3D12_RESOURCE_STATES InCreateState, uint32 InAlignment, ResourceLocation& ResourceLocation, const char* InName)
{
#ifndef NDEBUG
	// Validate the create state
	if (InHeapType == D3D12_HEAP_TYPE_READBACK)
	{
		wconstraint(InCreateState == D3D12_RESOURCE_STATE_COPY_DEST);
	}
	else if (InHeapType == D3D12_HEAP_TYPE_UPLOAD)
	{
		wconstraint(InCreateState == D3D12_RESOURCE_STATE_GENERIC_READ);
	}
	else if (InBufferAccess == EAccessHint::EA_GPUUnordered && InResourceStateMode == ResourceStateMode::Single)
	{
		wconstraint(InCreateState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	}
	else if (InBufferAccess & EAccessHint::EA_AccelerationStructure)
	{
		// RayTracing acceleration structures must be created in a particular state and may never transition out of it.
		wconstraint(InResourceStateMode == ResourceStateMode::Single);
		wconstraint(InCreateState == D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);
	}
#endif

	AllocateResource(GetParentDevice()->GetGPUIndex(), InHeapType, InDesc, InDesc.Width, InAlignment, InResourceStateMode, InCreateState, nullptr, InName, ResourceLocation);
}

template<MemoryPool::FreeListOrder Order, bool Defrag>
void PoolAllocator<Order, Defrag>::AllocateResource(uint32 GPUIndex, D3D12_HEAP_TYPE InHeapType, const D3D12_RESOURCE_DESC& InDesc, uint64 InSize, uint32 InAllocationAlignment, ResourceStateMode InResourceStateMode, D3D12_RESOURCE_STATES InCreateState, const D3D12_CLEAR_VALUE* InClearValue, const char* InName, ResourceLocation& ResourceLocation)
{
	// If the resource location owns a block, this will deallocate it.
	ResourceLocation.Clear();
	if (InSize == 0)
	{
		return;
	}

	auto Device = GetParentDevice();
	auto Adapter = Device->GetParentAdapter();

	wconstraint(GPUIndex == Device->GetGPUIndex());

	const bool PoolResource = InSize <= MaxAllocationSize;
	if (PoolResource)
	{
		const bool bPlacedResource = (Strategy == AllocationStrategy::kPlacedResource);

		uint32 AllocationAlignment = InAllocationAlignment;

		// Ensure we're allocating from the correct pool
		if (bPlacedResource)
		{
			// Writeable resources get separate ID3D12Resource* with their own resource state by using placed resources. Just make sure it's UAV, other flags are free to differ.
			wconstraint((InDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0 || InHeapType == D3D12_HEAP_TYPE_READBACK || InAllocationAlignment > kManualSubAllocationAlignment || InResourceStateMode == ResourceStateMode::Multi);

			// If it's a placed resource then base offset will always be 0 from the actual d3d resource so ignore the allocation alignment - no extra offset required
			// for creating the views!
			wconstraint(InAllocationAlignment <= PoolAlignment);
			AllocationAlignment = PoolAlignment;
		}
		else
		{
			// Read-only resources get suballocated from big resources, thus share ID3D12Resource* and resource state with other resources. Ensure it's suballocated from a resource with identical flags.
			wconstraint(InDesc.Flags == InitConfig.ResourceFlags);
		}

		auto& AllocationData = ResourceLocation.GetPoolAllocatorPrivateData();

		// Find the correct allocation resource type
		if (InDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
		{
			bool ret = TryAllocateInternal<MemoryPool::PoolResouceTypes::Buffers>(static_cast<uint32>(InSize), AllocationAlignment, AllocationData);
			wassume(ret);
		}
		else if (white::has_anyflags(InDesc.Flags, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL))
		{
		}
		else
		{
		}

		// Setup the resource location
		ResourceLocation.SetType(ResourceLocation::LocationType::SubAllocation);
		ResourceLocation.SetPoolAllocator(this);
		ResourceLocation.SetSize(InSize);

		AllocationData.SetOwner(&ResourceLocation);

		if (Strategy == AllocationStrategy::kManualSubAllocation)
		{
			auto BackingResource = GetBackingResource(ResourceLocation);

			ResourceLocation.SetOffsetFromBaseOfResource(AllocationData.GetOffset());
			ResourceLocation.SetResource(BackingResource);
			ResourceLocation.SetGPUVirtualAddress(BackingResource->GetGPUVirtualAddress() + AllocationData.GetOffset());

			if (IsCPUAccessible(InitConfig.HeapType))
			{
				ResourceLocation.SetMappedBaseAddress((uint8*)BackingResource->GetResourceBaseAddress() + AllocationData.GetOffset());
			}
		}
		else
		{
			wconstraint(ResourceLocation.GetResource() == nullptr);

			D3D12_RESOURCE_DESC Desc = InDesc;
			Desc.Alignment = AllocationAlignment;

			auto NewResource = CreatePlacedResource(AllocationData, Desc, InCreateState, InResourceStateMode, InClearValue, InName);
			ResourceLocation.SetResource(NewResource);
		}
	}
	else
	{
		// Allocate Standalone - move to owner of resource because this allocator should only manage pooled allocations (needed for now to do the same as FD3D12DefaultBufferPool)
		ResourceHolder* NewResource = nullptr;
		const D3D12_HEAP_PROPERTIES HeapProps = CD3DX12_HEAP_PROPERTIES(InHeapType, GetGPUMask().GetNative(), GetVisibilityMask().GetNative());
		D3D12_RESOURCE_DESC Desc = InDesc;
		Desc.Alignment = 0;
		Adapter->CreateCommittedResource(Desc, GetGPUMask(), HeapProps, InCreateState, InCreateState, InClearValue, &NewResource, InName);

		ResourceLocation.AsStandAlone(NewResource, InSize);
	}
}

template<MemoryPool::FreeListOrder Order, bool Defrag>
ResourceHolder* PoolAllocator<Order, Defrag>::CreatePlacedResource(const PoolAllocatorPrivateData& InAllocationData, const D3D12_RESOURCE_DESC& InDesc, D3D12_RESOURCE_STATES InCreateState, ResourceStateMode InResourceStateMode, const D3D12_CLEAR_VALUE* InClearValue, const char* InName)
{
	auto Adapter = GetParentDevice()->GetParentAdapter();
	auto HeapAndOffset = GetBackingHeapAndAllocationOffsetInBytes(InAllocationData);

	ResourceHolder* NewResource = nullptr;
	Adapter->CreatePlacedResource(InDesc, HeapAndOffset.Heap, HeapAndOffset.Offset, InCreateState, InResourceStateMode, InCreateState, InClearValue, &NewResource, InName);
	return NewResource;
}

template<MemoryPool::FreeListOrder Order, bool Defrag>
HeapAndOffset PoolAllocator<Order, Defrag>::GetBackingHeapAndAllocationOffsetInBytes(const PoolAllocatorPrivateData& InAllocationData) const
{
	return HeapAndOffset{
		.Heap = Pools[InAllocationData.GetPoolIndex()]->GetBackingHeap(),
		.Offset = uint64(AlignDown(InAllocationData.GetOffset(),PoolAlignment))
	};
}

template<MemoryPool::FreeListOrder Order, bool Defrag>
template<MemoryPool::PoolResouceTypes InAllocationResourceType>
bool PoolAllocator<Order, Defrag>::TryAllocateInternal(uint32 InSizeInBytes, uint32 InAllocationAlignment, PoolAllocatorPrivateData& AllocationData)
{
	std::unique_lock Lock{ CS };

	wconstraint(PoolAllocationOrder.size() == Pools.size());
	for (int32 PoolIndex : PoolAllocationOrder)
	{
		auto Pool = Pools[PoolIndex];
		if (Pool != nullptr && Pool->IsResourceTypeSupported(InAllocationResourceType) && !Pool->IsFull() && Pool->TryAllocate(InSizeInBytes, InAllocationAlignment, InAllocationResourceType, AllocationData))
		{
			return true;
		}
	}

	// Find a free pool index
	int16 NewPoolIndex = -1;
	for (int32 PoolIndex = 0; PoolIndex <static_cast<int32>(Pools.size()); PoolIndex++)
	{
		if (Pools[PoolIndex] == nullptr)
		{
			NewPoolIndex = PoolIndex;
			break;
		}
	}

	if (NewPoolIndex >= 0)
	{
		Pools[NewPoolIndex] = CreateNewPool(NewPoolIndex, InSizeInBytes, InAllocationResourceType);
	}
	else
	{
		NewPoolIndex =static_cast<int16>(Pools.size());
		Pools.emplace_back(CreateNewPool(NewPoolIndex, InSizeInBytes, InAllocationResourceType));
		PoolAllocationOrder.emplace_back(NewPoolIndex);
	}

	return Pools[NewPoolIndex]->TryAllocate(InSizeInBytes, InAllocationAlignment, InAllocationResourceType, AllocationData);

	return false;
}

template<MemoryPool::FreeListOrder Order, bool Defrag>
MemoryPool* PoolAllocator<Order, Defrag>::CreateNewPool(int16 InPoolIndex, uint32 InMinimumAllocationSize, MemoryPool::PoolResouceTypes InAllocationResourceType)
{
	// Find out the pool size - use default, but if allocation doesn't fit then round up to next power of 2
	// so it 'always' fits the pool allocator
	auto PoolSize = DefaultPoolSize;
	if (InMinimumAllocationSize > PoolSize)
	{
		wconstraint(InMinimumAllocationSize <= MaxAllocationSize);
		PoolSize = std::min<uint64>(std::bit_ceil(InMinimumAllocationSize),MaxAllocationSize);
	}

	auto NewPool = new MemoryPool(GetParentDevice(), GetVisibilityMask(), InitConfig,
		std::format("{}/{}", Name ,PoolNewCount), Strategy, InPoolIndex, PoolSize, PoolAlignment, InAllocationResourceType, Order);
	++PoolNewCount;

	return NewPool;
}

template<MemoryPool::FreeListOrder Order, bool Defrag>
void PoolAllocator<Order, Defrag>::TransferOwnership(ResourceLocation& Destination, ResourceLocation& Source)
{
	std::unique_lock Lock{ CS };


	wconstraint(IsOwner(Source));
	
	// Don't need to lock - ownership simply changed
	bool bLocked = false;
	auto& DestinationPoolData = Destination.GetPoolAllocatorPrivateData();
	DestinationPoolData.MoveFrom(Source.GetPoolAllocatorPrivateData(), bLocked);
	DestinationPoolData.SetOwner(&Destination);
}

template<MemoryPool::FreeListOrder Order, bool Defrag>
PoolAllocatorPrivateData* PoolAllocator<Order, Defrag>::GetNewAllocationData()
{
	return AllocationDataPool.empty() ? new PoolAllocatorPrivateData() : white::pop(AllocationDataPool);
}

template<MemoryPool::FreeListOrder Order, bool Defrag>
void PoolAllocator<Order, Defrag>::ReleaseAllocationData(PoolAllocatorPrivateData* InData)
{
	InData->Reset();
	if (AllocationDataPool.size() >= 32)
		delete InData;
	else
		AllocationDataPool.push_back(InData);
}

template<MemoryPool::FreeListOrder Order, bool Defrag>
void PoolAllocator<Order, Defrag>::Deallocate(ResourceLocation& ResourceLocation)
{
	wconstraint(IsOwner(ResourceLocation));

	std::unique_lock Lock{ CS };

	auto& AllocationData = ResourceLocation.GetPoolAllocatorPrivateData();
	wconstraint(AllocationData.IsAllocated());

	if (AllocationData.IsLocked())
	{
		for (FrameFencedAllocationData& Operation : FrameFencedOperations)
		{
			if (Operation.AllocationData == &AllocationData)
			{
				wconstraint(Operation.Operation == FrameFencedAllocationData::EOperation::Unlock);
				Operation.Operation = FrameFencedAllocationData::EOperation::Nop;
				Operation.AllocationData = nullptr;
				break;
			}
		}
	}

	int16 PoolIndex = AllocationData.GetPoolIndex();
	auto ReleasedAllocationData = GetNewAllocationData();
	bool bLocked = true;
	ReleasedAllocationData->MoveFrom(AllocationData, bLocked);

	// Clear the allocator data
	ResourceLocation.ClearAllocator();

	// Store fence when last used so we know when to unlock the free data
	auto Adapter = GetParentDevice()->GetParentAdapter();
	auto& FrameFence = Adapter->GetFrameFence();

	FrameFencedOperations.emplace_back(FrameFencedAllocationData{
		.Operation = FrameFencedAllocationData::EOperation::Deallocate,
		.PlacedResource = ResourceLocation.GetResource()->IsPlacedResource() ? ResourceLocation.GetResource() : nullptr,
		.FrameFence = FrameFence.GetCurrentFence(),
		.AllocationData = ReleasedAllocationData,
		});

	// Update the last used frame fence (used during garbage collection)
	Pools[PoolIndex]->UpdateLastUsedFrameFence(FrameFencedOperations.back().FrameFence);
}

template<MemoryPool::FreeListOrder Order, bool Defrag>
void PoolAllocator<Order, Defrag>::DeallocateInternal(PoolAllocatorPrivateData& AllocationData)
{
	wconstraint(AllocationData.IsAllocated());
	wconstraint(AllocationData.GetPoolIndex() < Pools.size());
	Pools[AllocationData.GetPoolIndex()]->Deallocate(AllocationData);
}

template<MemoryPool::FreeListOrder Order, bool Defrag>
ResourceHolder* PoolAllocator<Order, Defrag>::GetBackingResource(ResourceLocation& InResouceLocation) const
{
	wconstraint(IsOwner(InResouceLocation));

	auto& AllocationData = InResouceLocation.GetPoolAllocatorPrivateData();

	return Pools[AllocationData.GetPoolIndex()]->GetBackingResource();
}

using MultiBuddyConstantUploadAllocator = MultiBuddyAllocator<true, true>;

MultiBuddyConstantUploadAllocator::MultiBuddyAllocator(NodeDevice* InParentDevice, GPUMaskType VisibleNodes)
	:DeviceChild(InParentDevice),
	MultiNodeGPUObject(InParentDevice->GetGPUMask(), VisibleNodes)
{}

bool MultiBuddyConstantUploadAllocator::TryAllocate(uint32 SizeInBytes, uint32 Alignment, ResourceLocation& ResourceLocation)
{
	{
		std::shared_lock Lock{ CS };

		for (auto& allocator : Allocators)
		{
			if (allocator->TryAllocate(SizeInBytes, Alignment, ResourceLocation))
				return true;
		}
	}
	
	std::unique_lock Lock{ CS };
	Allocators.push_back(CreateNewAllocator(SizeInBytes));
	return Allocators.back()->TryAllocate(SizeInBytes, Alignment, ResourceLocation);
}

void MultiBuddyConstantUploadAllocator::CleanUpAllocations(uint64 InFrameLag)
{
	std::unique_lock Lock{ CS };

	// Trim empty allocators if not used in last n frames
	auto Adapter = GetParentDevice()->GetParentAdapter();
	auto& FrameFence = Adapter->GetFrameFence();
	const uint64 CompletedFence = FrameFence.UpdateLastCompletedFence();

	for (int32 i = static_cast<int32>(Allocators.size() - 1); i >= 0; i--)
	{
		if (!Allocators[i]->IsDeallocated())
			continue;
		if ((Allocators[i]->GetLastUsedFrameFence() + InFrameLag <= CompletedFence))
		{
			delete(Allocators[i]);
			Allocators.erase(Allocators.begin() + i);
		}
	}
}

MultiBuddyConstantUploadAllocator::ConstantAllocator::ConstantAllocator(NodeDevice* InParentDevice, GPUMaskType VisibleNodes, uint32 InBlockSize)
	:DeviceChild(InParentDevice), 
	MultiNodeGPUObject(InParentDevice->GetGPUMask(), VisibleNodes), 
	BlockSize(InBlockSize), TotalSizeUsed(0), BackingResource(nullptr), DelayCreated(false)
	, RetireFrameFence(-1)
{}

MultiBuddyConstantUploadAllocator::ConstantAllocator::~ConstantAllocator()
{
	delete BackingResource;
}

void MultiBuddyConstantUploadAllocator::ConstantAllocator::Deallocate(ResourceLocation& ResourceLocation)
{
	auto& FrameFence = GetParentDevice()->GetParentAdapter()->GetFrameFence();

	RetireFrameFence = FrameFence.GetCurrentFence();
}

MultiBuddyConstantUploadAllocator::ConstantAllocator* MultiBuddyConstantUploadAllocator::CreateNewAllocator(uint32 InMinSizeInBytes)
{
	uint32 AllocationSize = std::bit_ceil(InMinSizeInBytes);

	return new ConstantAllocator(GetParentDevice(), GetVisibilityMask(), AllocationSize);
}

bool MultiBuddyConstantUploadAllocator::ConstantAllocator::TryAllocate(uint32 SizeInBytes, uint32 Alignment, ResourceLocation& ResourceLocation)
{
	if (TotalSizeUsed == BlockSize)
	{
		return false;
	}

	if (!DelayCreated)
	{
		CreateBackingResource();
		DelayCreated = true;
	}

	uint32 SizeToAllocate = SizeInBytes;
	if (Alignment != 0 && BlockSize % Alignment != 0)
	{
		SizeToAllocate = SizeInBytes + Alignment;
		wassume(false);
	}

	if (BlockSize < SizeToAllocate)
		return false;

	const uint32 AllocSize = BlockSize;
	uint32 Padding = 0;

	const uint32 AlignedOffsetFromResourceBase = 0;

	TotalSizeUsed += AllocSize;


	ResourceLocation.SetType(ResourceLocation::SubAllocation);
	ResourceLocation.SetSubDeAllocator(this);
	ResourceLocation.SetSize(SizeInBytes);
	ResourceLocation.SetOffsetFromBaseOfResource(AlignedOffsetFromResourceBase);
	ResourceLocation.SetResource(BackingResource);
	ResourceLocation.SetGPUVirtualAddress(BackingResource->GetGPUVirtualAddress() + AlignedOffsetFromResourceBase);

	ResourceLocation.SetMappedBaseAddress((uint8*)BackingResource->GetResourceBaseAddress() + AlignedOffsetFromResourceBase);

	return true;
}

void MultiBuddyConstantUploadAllocator::ConstantAllocator::CreateBackingResource()
{
	auto Adapter = GetParentDevice()->GetParentAdapter();

	const D3D12_HEAP_PROPERTIES HeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD, GetGPUMask().GetNative(), GetVisibilityMask().GetNative());

	auto Name = std::format("ConstantAllocator_{:x}", (intptr_t)this);
	Adapter->CreateBuffer(HeapProps, GetGPUMask(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_GENERIC_READ, BlockSize, &BackingResource, Name.c_str(), D3D12_RESOURCE_FLAG_NONE);

	BackingResource->Map();
}

ResourceConfigAllocator::ResourceConfigAllocator(NodeDevice* InParentDevice, GPUMaskType VisibleNodes, const AllocatorConfig& InConfig, const std::string& Name, uint32 InMaxAllocationSize)
	:DeviceChild(InParentDevice), MultiNodeGPUObject(InParentDevice->GetGPUMask(), VisibleNodes),
	InitConfig(InConfig),
	DebugName(Name),
	Initialized(false),
	MaximumAllocationSizeForPooling(InMaxAllocationSize)
{
}

BuddyUploadAllocator::BuddyUploadAllocator(NodeDevice* InParentDevice, GPUMaskType VisibleNodes,
	const AllocatorConfig& InConfig, const std::string& Name, AllocationStrategy InStrategy,
	uint32 InMaxAllocationSize, uint32 InMaxBlockSize, uint32 InMinBlockSize)
	:ResourceConfigAllocator(InParentDevice, VisibleNodes, InConfig, Name, InMaxAllocationSize)
	, MaxBlockSize(InMaxBlockSize)
	, MinBlockSize(InMinBlockSize)
	, Strategy(InStrategy)
	, BackingResource(nullptr)
	, LastUsedFrameFence(0)
	, FreeBlockArray(nullptr)
	, TotalSizeUsed(0)
{
	WAssert((MaxBlockSize / MinBlockSize) * MinBlockSize == MaxBlockSize, " Evenly dividable");

	WAssert(0 == ((MaxBlockSize / MinBlockSize) & ((MaxBlockSize / MinBlockSize) - 1)), " Power of two");


	MaxOrder = UnitSizeToOrder(SizeToUnitSize(MaxBlockSize));

	if (IsCPUAccessible(InitConfig.HeapType))
	{
		WAssert(InMaxAllocationSize < InMaxBlockSize, "allocator reserved memory");
		WAssert(sizeof(void*) * MaxOrder < MaxBlockSize, "allocator reserved memory too small");
	}
}

void BuddyUploadAllocator::Initialize()
{
	auto Adapter = GetParentDevice()->GetParentAdapter();

	if (Strategy == AllocationStrategy::kPlacedResource) {
		//TODO
		wassume(false);
	}
	else {
		const D3D12_HEAP_PROPERTIES HeapProps = CD3DX12_HEAP_PROPERTIES(InitConfig.HeapType, GetGPUMask().GetNative(), GetVisibilityMask().GetNative());

		CheckHResult(Adapter->CreateBuffer(HeapProps, GetGPUMask(),
			InitConfig.InitialResourceState, InitConfig.InitialResourceState, MaxBlockSize, &BackingResource, "BuddyAllocator Underlying Buffer", InitConfig.ResourceFlags));

		if (IsCPUAccessible(InitConfig.HeapType)) {
			BackingResource->Map();

			void* pStart = BackingResource->GetResourceBaseAddress();
			FreeBlockArray = reinterpret_cast<FreeBlock**>(pStart);
			//Ԥ���з�Buddy
			for (uint32 i = 0; i < MaxOrder; ++i)
			{
				FreeBlockArray[i] = reinterpret_cast<FreeBlock*>((byte*)pStart + MinBlockSize * (1 << i));
				FreeBlockArray[i]->Next = nullptr;
			}

			TotalSizeUsed = MinBlockSize;
		}
	}
}

void BuddyUploadAllocator::ReleaseAllResources()
{
	wassume(DeferredDeletionQueue.empty());

	delete BackingResource;
}

uint32 BuddyUploadAllocator::AllocateBlock(uint32 order)
{
	uint32 offset = 0;
	if (order >= MaxOrder)
	{
		WAssert(false, "Can't allocate a block that large  ");
	}

	if (FreeBlockArray[order] == nullptr)
	{
		uint32 left = AllocateBlock(order + 1);

		uint32 size = OrderToUnitSize(order);

		uint32 right = left + size;

		byte* pStart = reinterpret_cast<byte*>(FreeBlockArray);
		FreeBlock* pRight = reinterpret_cast<FreeBlock*>(pStart + right * MinBlockSize);
		pRight->Next = nullptr;
		FreeBlockArray[order] = pRight;

		offset = left;
	}
	else
	{
		byte* pStart = reinterpret_cast<byte*>(FreeBlockArray);
		byte* pEnd = reinterpret_cast<byte*>(FreeBlockArray[order]);

		offset = static_cast<uint32>(pEnd - pStart) / MinBlockSize;
		wconstraint(offset * MinBlockSize == pEnd - pStart);

		FreeBlockArray[order] = FreeBlockArray[order]->Next;
	}
	return offset;
}

void BuddyUploadAllocator::Allocate(uint32 SizeInBytes, uint32 Alignment, ResourceLocation& ResourceLocation)
{
	std::unique_lock Lock{ CS };

	uint32 SizeToAllocate = SizeInBytes;

	// If the alignment doesn't match the block size
	if (Alignment != 0 && MinBlockSize % Alignment != 0)
	{
		SizeToAllocate = SizeInBytes + Alignment;
	}

	// Work out what size block is needed and allocate one
	const uint32 UnitSize = SizeToUnitSize(SizeToAllocate);
	const uint32 Order = UnitSizeToOrder(UnitSize);
	const uint32 Offset = AllocateBlock(Order); // This is the offset in MinBlockSize units

	const uint32 AllocSize = uint32(OrderToUnitSize(Order) * MinBlockSize);
	const uint32 AllocationBlockOffset = uint32(Offset * MinBlockSize);
	uint32 Padding = 0;

	TotalSizeUsed += AllocSize;

	if (Alignment != 0 && AllocationBlockOffset % Alignment != 0)
	{
		uint32 AlignedBlockOffset = AlignArbitrary(AllocationBlockOffset, Alignment);
		Padding = AlignedBlockOffset - AllocationBlockOffset;

		wconstraint((Padding + SizeInBytes) <= AllocSize);
	}

	const uint32 AlignedOffsetFromResourceBase = AllocationBlockOffset + Padding;
	wconstraint((AlignedOffsetFromResourceBase % Alignment) == 0);


	ResourceLocation.SetType(ResourceLocation::SubAllocation);
	ResourceLocation.SetSubDeAllocator(this);
	ResourceLocation.SetSize(SizeInBytes);

	if (Strategy == AllocationStrategy::kManualSubAllocation)
	{
		ResourceLocation.SetOffsetFromBaseOfResource(AlignedOffsetFromResourceBase);
		ResourceLocation.SetResource(BackingResource);
		ResourceLocation.SetGPUVirtualAddress(BackingResource->GetGPUVirtualAddress() + AlignedOffsetFromResourceBase);

		if (IsCPUAccessible(InitConfig.HeapType))
		{
			ResourceLocation.SetMappedBaseAddress((uint8*)BackingResource->GetResourceBaseAddress() + AlignedOffsetFromResourceBase);
		}
	}

	ResourceLocation.GetBuddyAllocatorPrivateData().Offset = Offset;
	ResourceLocation.GetBuddyAllocatorPrivateData().Order = Order;
}

bool BuddyUploadAllocator::TryAllocate(uint32 SizeInBytes, uint32 Alignment, ResourceLocation& ResourceLocation)
{
	std::unique_lock Lock{ CS };

	if (Initialized == false)
	{
		Initialize();
		Initialized = true;
	}

	if (CanAllocate(SizeInBytes, Alignment))
	{
		Allocate(SizeInBytes, Alignment, ResourceLocation);
		return true;
	}
	return false;
}

void BuddyUploadAllocator::Deallocate(ResourceLocation& ResourceLocation)
{
	wconstraint(IsOwner(ResourceLocation));

	auto adpter = GetParentDevice()->GetParentAdapter();
	auto& fence = adpter->GetFrameFence();

	auto& PrivateData = ResourceLocation.GetBuddyAllocatorPrivateData();


	RetiredBlock Block{};
	Block.FrameFence = fence.GetCurrentFence();
	Block.Data = PrivateData;

	{
		std::unique_lock Lock{ CS };
		DeferredDeletionQueue.emplace_back(Block);
	}

	LastUsedFrameFence = std::max(LastUsedFrameFence, Block.FrameFence);
}

void BuddyUploadAllocator::CleanUpAllocations()
{
	std::unique_lock Lock{ CS };

	auto adpter = GetParentDevice()->GetParentAdapter();
	auto& fence = adpter->GetFrameFence();

	uint32 PopCount = 0;
	for (size_t i = 0; i < DeferredDeletionQueue.size(); i++)
	{
		RetiredBlock& Block = DeferredDeletionQueue[i];

		if (fence.IsFenceComplete(Block.FrameFence))
		{
			DeallocateInternal(Block);
			PopCount = static_cast<uint32>(i + 1);
		}
		else
		{
			break;
		}
	}

	if (PopCount)
	{
		// clear out all of the released blocks, don't allow the array to shrink
		DeferredDeletionQueue.erase(DeferredDeletionQueue.begin(), DeferredDeletionQueue.begin() + PopCount);
	}
}

void BuddyUploadAllocator::DeallocateInternal(RetiredBlock& Block)
{
	DeallocateBlock(Block.Data.Offset, Block.Data.Order);

	const uint32 Size = uint32(OrderToUnitSize(Block.Data.Order) * MinBlockSize);

	TotalSizeUsed -= Size;
}

void BuddyUploadAllocator::DeallocateBlock(uint32 offset, uint32 order)
{
	// See if the buddy block is free  
	uint32 size = OrderToUnitSize(order);

	uint32 buddy = GetBuddyOffset(offset, size);

	//find in list
	FreeBlock* it = nullptr;
	auto pSearch = FreeBlockArray[order];
	FreeBlock* pPrev = nullptr;
	while (pSearch != nullptr)
	{
		byte* pStart = reinterpret_cast<byte*>(FreeBlockArray);
		byte* pEnd = reinterpret_cast<byte*>(pSearch);
		if ((pEnd - pStart) == offset * MinBlockSize)
		{
			it = pSearch;
			break;
		}
		pPrev = pSearch;
		pSearch = pSearch->Next;
	}

	if (it != nullptr)
	{
		// Deallocate merged blocks
		DeallocateBlock(std::min(offset, buddy), order + 1);

		if (pPrev == nullptr)
		{
			FreeBlockArray[order] = FreeBlockArray[order]->Next;
		}
		else {

			pPrev->Next = it->Next;
		}
	}
	else {
		byte* pStart = reinterpret_cast<byte*>(FreeBlockArray);
		FreeBlock* pRight = reinterpret_cast<FreeBlock*>(pStart + offset * MinBlockSize);

		pRight->Next = FreeBlockArray[order];
		FreeBlockArray[order] = pRight;
	}
}


bool BuddyUploadAllocator::CanAllocate(uint32 size, uint32 alignment)
{
	if (TotalSizeUsed == MaxBlockSize)
		return false;

	uint32 sizeToAllocate = size;
	// If the alignment doesn't match the block size
	if (alignment != 0 && MinBlockSize % alignment != 0)
	{
		sizeToAllocate = size + alignment;
	}

	uint32 blockSize = MaxBlockSize / 2;

	for (int32 i = MaxOrder - 1; i >= 0; i--)
	{
		if (FreeBlockArray[i] != nullptr && blockSize >= sizeToAllocate)
			return true;

		blockSize /= 2;

		if (blockSize < sizeToAllocate) return false;
	}

	return false;
}

using MultiBuddyUploadAllocator = MultiBuddyAllocator<false, true>;
MultiBuddyUploadAllocator::MultiBuddyAllocator(NodeDevice* InParentDevice, GPUMaskType VisibleNodes, const AllocatorConfig& InConfig, const std::string& Name, AllocationStrategy InStrategy, uint32 InMaxAllocationSize, uint32 InDefaultPoolSize, uint32 InMinBlockSize)
	:ResourceConfigAllocator(InParentDevice, VisibleNodes, InConfig, Name, InMaxAllocationSize)
	, DefaultPoolSize(InDefaultPoolSize)
	, MinBlockSize(InMinBlockSize)
	, Strategy(InStrategy)
{
	wconstraint(std::bit_ceil(MaximumAllocationSizeForPooling) < DefaultPoolSize);
}

bool MultiBuddyUploadAllocator::TryAllocate(uint32 SizeInBytes, uint32 Alignment, ResourceLocation& ResourceLocation)
{
	std::unique_lock Lock{ CS };

	for (size_t i = 0; i < Allocators.size(); i++)
	{
		if (Allocators[i]->TryAllocate(SizeInBytes, Alignment, ResourceLocation))
		{
			return true;
		}
	}

	auto Allocator = Allocators.emplace_back(CreateNewAllocator(SizeInBytes));
	return Allocator->TryAllocate(SizeInBytes, Alignment, ResourceLocation);
}

BuddyUploadAllocator* MultiBuddyUploadAllocator::CreateNewAllocator(uint32 InMinSizeInBytes)
{
	wconstraint(InMinSizeInBytes <= MaximumAllocationSizeForPooling);
	uint32 AllocationSize = (InMinSizeInBytes > DefaultPoolSize) ? std::bit_ceil(InMinSizeInBytes) : DefaultPoolSize;

	return new BuddyUploadAllocator(GetParentDevice(),
		GetVisibilityMask(),
		InitConfig,
		DebugName,
		Strategy,
		MaximumAllocationSizeForPooling,
		AllocationSize,
		MinBlockSize);
}

void MultiBuddyUploadAllocator::Deallocate(ResourceLocation& ResourceLocation)
{
	WAssert(false, "The sub-allocators should handle the deallocation");
}

void MultiBuddyUploadAllocator::CleanUpAllocations(uint64 InFrameLag)
{
	std::unique_lock Lock{ CS };

	for (auto*& Allocator : Allocators)
	{
		Allocator->CleanUpAllocations();
	}

	// Trim empty allocators if not used in last n frames
	auto Adapter = GetParentDevice()->GetParentAdapter();
	auto& FrameFence = Adapter->GetFrameFence();
	const uint64 CompletedFence = FrameFence.UpdateLastCompletedFence();

	for (int32 i = static_cast<int32>(Allocators.size() - 1); i >= 0; i--)
	{
		if (Allocators[i]->IsEmpty() && (Allocators[i]->GetLastUsedFrameFence() + InFrameLag <= CompletedFence))
		{
			delete(Allocators[i]);
			Allocators.erase(Allocators.begin() + i);
		}
	}
}

UploadHeapAllocator::UploadHeapAllocator(D3D12Adapter* InParent, NodeDevice* InParentDevice, const std::string& InName)
	:AdapterChild(InParent), DeviceChild(InParentDevice), MultiNodeGPUObject(InParentDevice->GetGPUMask(),GPUMaskType::AllGPU())
	, SmallBlockAllocator(InParentDevice, GetVisibilityMask(), {}, InName, AllocationStrategy::kManualSubAllocation,
		UploadHeapSmallBlockMaxAllocationSize, UploadHeapSmallBlockPoolSize, 256)
	, BigBlockAllocator(InParentDevice, GetVisibilityMask(), {},InName,AllocationStrategy::kManualSubAllocation,
		UploadHeapBigBlockHeapSize,256, UploadHeapBigBlockMaxAllocationSize)
	, FastConstantAllocator(InParentDevice, GetVisibilityMask())
{
}
void* UploadHeapAllocator::AllocFastConstantAllocationPage(uint32 InSize, uint32 InAlignment, ResourceLocation& ResourceLocation)
{
	ResourceLocation.Clear();

	bool ret = FastConstantAllocator.TryAllocate(InSize, InAlignment, ResourceLocation);
	wassume(ret);

	return ResourceLocation.GetMappedBaseAddress();
}

void* UploadHeapAllocator::AllocUploadResource(uint32 InSize, uint32 InAlignment, ResourceLocation& ResourceLocation)
{
	wassume(InSize > 0);

	//Clean up the release queue of resources which are currently not used by the GPU anymore

	ResourceLocation.Clear();

	// Fit in small block allocator?
	if (InSize <= SmallBlockAllocator.GetMaximumAllocationSizeForPooling())
	{
		bool ret = SmallBlockAllocator.TryAllocate(InSize, InAlignment, ResourceLocation);
		wconstraint(ret);
	}
	else
	{
		// Forward to the big block allocator
		const D3D12_RESOURCE_DESC ResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(InSize, D3D12_RESOURCE_FLAG_NONE);
		BigBlockAllocator.AllocateResource(GetParentDevice()->GetGPUIndex(), D3D12_HEAP_TYPE_UPLOAD, ResourceDesc, InSize, InAlignment, ResourceStateMode::Single, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, nullptr, ResourceLocation);
		ResourceLocation.UnlockPoolData();
	}

	return ResourceLocation.GetMappedBaseAddress();
}

FastConstantAllocator::FastConstantAllocator(NodeDevice* Parent, GPUMaskType InGpuMask)
	:DeviceChild(Parent), MultiNodeGPUObject(InGpuMask, InGpuMask)
	, UnderlyingResource(Parent), Offset(FastConstantAllocatorPageSize), PageSize(FastConstantAllocatorPageSize)
{
}

void UploadHeapAllocator::CleanUpAllocations(uint64 InFrameLag)
{
	SmallBlockAllocator.CleanUpAllocations(InFrameLag);
	BigBlockAllocator.CleanUpAllocations(InFrameLag);
	FastConstantAllocator.CleanUpAllocations(InFrameLag);
}

BufferAllocator::BufferAllocator(NodeDevice* Parent, GPUMaskType VisibleNodes)
	:DeviceChild(Parent),MultiNodeGPUObject(Parent->GetGPUMask(),VisibilityMask)
{
}

void BufferAllocator::CleanUpAllocations(uint64 InFrameLag)
{
	std::shared_lock lock{ Mutex };
	for (auto Pool : DefaultBufferPools)
	{
		if (Pool)
			Pool->CleanUpAllocations(InFrameLag);
	}
}


template<ResourceStateMode mode>
void BufferAllocator::AllocDefaultResource(D3D12_HEAP_TYPE InHeapType, const D3D12_RESOURCE_DESC& InResourceDesc, uint32 InBuffAccess, D3D12_RESOURCE_STATES InCreateState, ResourceLocation& ResourceLocation, uint32 Alignment, const char* Name)
{
	D3D12_RESOURCE_DESC ResourceDesc = InResourceDesc;
	ResourceDesc.Flags = ResourceDesc.Flags & (~D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE);

	if (white::has_anyflags(InBuffAccess, EAccessHint::EA_DrawIndirect))
	{
		//Force indirect args to stand alone allocations instead of pooled
		ResourceLocation.Clear();

		ResourceHolder* NewResource = nullptr;
		const D3D12_HEAP_PROPERTIES HeapProps = CD3DX12_HEAP_PROPERTIES(InHeapType, GetGPUMask().GetNative(), GetVisibilityMask().GetNative());
		D3D12_RESOURCE_DESC Desc = InResourceDesc;
		Desc.Alignment = 0;
		CheckHResult(GetParentDevice()->GetParentAdapter()->CreateCommittedResource(
			Desc, GetGPUMask(), HeapProps, InCreateState, InCreateState, nullptr, &NewResource, Name));

		ResourceLocation.AsStandAlone(NewResource, InResourceDesc.Width);
		return;
	}

	// Do we already have a default pool which support this allocation?
	BufferPool* BufferPool = nullptr;
	{
		std::shared_lock lock{ Mutex };
		for (auto Pool : DefaultBufferPools)
		{
			if (Pool->SupportsAllocation(InHeapType, ResourceDesc.Flags, InBuffAccess, mode))
			{
				BufferPool = Pool;
				break;
			}
		}
	}

	// No pool yet, then create one
	if (BufferPool == nullptr)
	{
		BufferPool = CreateBufferPool(InHeapType, ResourceDesc.Flags, InBuffAccess, mode);
	}

	// Perform actual allocation
	BufferPool->AllocDefaultResource(InHeapType, ResourceDesc, InBuffAccess, mode, InCreateState, Alignment, ResourceLocation,Name);
}

template<ResourceStateMode mode>
D3D12_RESOURCE_STATES BufferAllocator::GetDefaultInitialResourceState(D3D12_HEAP_TYPE InHeapType, uint32 InBufferAccess)
{
	// Validate the create state
	if (InHeapType == D3D12_HEAP_TYPE_READBACK)
	{
		return D3D12_RESOURCE_STATE_COPY_DEST;
	}
	else if (InHeapType == D3D12_HEAP_TYPE_UPLOAD)
	{
		return D3D12_RESOURCE_STATE_GENERIC_READ;
	}
	else if (InBufferAccess == EA_GPUUnordered && mode == ResourceStateMode::Single)
	{
		wconstraint(InHeapType == D3D12_HEAP_TYPE_DEFAULT);
		return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	}
	else if (InBufferAccess & EA_AccelerationStructure)
	{
		wconstraint(InHeapType == D3D12_HEAP_TYPE_DEFAULT);
		return D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
	}
	else
	{
		return D3D12_RESOURCE_STATE_GENERIC_READ;
	}
}

template void BufferAllocator::AllocDefaultResource<ResourceStateMode::Default>(D3D12_HEAP_TYPE InHeapType, const D3D12_RESOURCE_DESC& InResourceDesc, uint32 InBuffAccess, D3D12_RESOURCE_STATES InCreateState, ResourceLocation& ResourceLocation, uint32 Alignment, const char* Name);
template void BufferAllocator::AllocDefaultResource<ResourceStateMode::Single>(D3D12_HEAP_TYPE InHeapType, const D3D12_RESOURCE_DESC& InResourceDesc, uint32 InBuffAccess, D3D12_RESOURCE_STATES InCreateState, ResourceLocation& ResourceLocation, uint32 Alignment, const char* Name);
template void BufferAllocator::AllocDefaultResource<ResourceStateMode::Multi>(D3D12_HEAP_TYPE InHeapType, const D3D12_RESOURCE_DESC& InResourceDesc, uint32 InBuffAccess, D3D12_RESOURCE_STATES InCreateState, ResourceLocation& ResourceLocation, uint32 Alignment, const char* Name);
template D3D12_RESOURCE_STATES BufferAllocator::GetDefaultInitialResourceState<ResourceStateMode::Default>(D3D12_HEAP_TYPE InHeapType, uint32 InBufferAccess);
template D3D12_RESOURCE_STATES BufferAllocator::GetDefaultInitialResourceState<ResourceStateMode::Single>(D3D12_HEAP_TYPE InHeapType, uint32 InBufferAccess);
template D3D12_RESOURCE_STATES BufferAllocator::GetDefaultInitialResourceState<ResourceStateMode::Multi>(D3D12_HEAP_TYPE InHeapType, uint32 InBufferAccess);



BufferPool* BufferAllocator::CreateBufferPool(D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_FLAGS InResourceFlags, uint32 InBufferAccess, ResourceStateMode InResourceStateMode)
{
	auto Device = GetParentDevice();
	auto Config = BufferPool::GetResourceAllocatorInitConfig(InHeapType, InResourceFlags, static_cast<EAccessHint>(InBufferAccess));

	const std::string Name= std::format("{}/Pool/{}" ,InHeapType ,DefaultBufferPools.size());
	auto AllocationStrategy = IPoolAllocator::GetResourceAllocationStrategy(InResourceFlags, InResourceStateMode);
	uint64 PoolSize = InHeapType == D3D12_HEAP_TYPE_READBACK ? READBACK_BUFFER_POOL_DEFAULT_POOL_SIZE : PoolAllocatorBufferPoolSize;
	uint32 PoolAlignment = (AllocationStrategy == AllocationStrategy::kPlacedResource) ? MIN_PLACED_RESOURCE_SIZE : 256;
	uint64 MaxAllocationSize = InHeapType == D3D12_HEAP_TYPE_READBACK ? READBACK_BUFFER_POOL_MAX_ALLOC_SIZE : PoolAllocatorBufferMaxAllocationSize;

	// Disable defrag if not Default memory
	bool bDefragEnabled = (Config.HeapType == D3D12_HEAP_TYPE_DEFAULT);

	BufferPool* NewPool = nullptr;

	if (bDefragEnabled)
		NewPool = new PoolAllocator<MemoryPool::FreeListOrder::SortByOffset, true>(Device, GetVisibilityMask(), Config, Name, AllocationStrategy, PoolSize, PoolAlignment, static_cast<uint32>(MaxAllocationSize));
	else
		NewPool = new PoolAllocator<MemoryPool::FreeListOrder::SortByOffset, false>(Device, GetVisibilityMask(), Config, Name, AllocationStrategy, PoolSize, PoolAlignment, static_cast<uint32>(MaxAllocationSize));

	std::unique_lock lock{ Mutex };
	DefaultBufferPools.emplace_back(NewPool);

	return NewPool;
}


//-----------------------------------------------------------------------------
//	Fast Allocation
//-----------------------------------------------------------------------------

FastAllocator::FastAllocator(NodeDevice* Parent, GPUMaskType VisibiltyMask, D3D12_HEAP_TYPE InHeapType, uint32 PageSize)
	: DeviceChild(Parent), MultiNodeGPUObject(Parent->GetGPUMask(), VisibiltyMask)
	, PagePool(Parent, VisibiltyMask, InHeapType, PageSize)
	, CurrentAllocatorPage(nullptr)
{}

FastAllocator::FastAllocator(NodeDevice* Parent, GPUMaskType VisibiltyMask, const D3D12_HEAP_PROPERTIES& InHeapProperties, uint32 PageSize)
	: DeviceChild(Parent), MultiNodeGPUObject(Parent->GetGPUMask(), VisibiltyMask)
	, PagePool(Parent, VisibiltyMask, InHeapProperties, PageSize)
	, CurrentAllocatorPage(nullptr)
{}

void* FastAllocator::Allocate(uint32 Size, uint32 Alignment, class ResourceLocation* ResourceLocation)
{
	// Check to make sure our assumption that we don't need a ResourceLocation->Clear() here is valid.
	WAssert(!ResourceLocation->IsValid(), "The supplied resource location already has a valid resource. You should Clear() it first or it may leak.");

	if (Size > PagePool.GetPageSize())
	{
		auto Adapter = GetParentDevice()->GetParentAdapter();

		//Allocations are 64k aligned
		if (Alignment)
		{
			Alignment = (kBufferAlignment % Alignment) == 0 ? 0 : Alignment;
		}

		ResourceHolder* Resource = nullptr;

		static std::atomic<int64> ID = 0;
		const int64 UniqueID = ++ID;

		CheckHResult(Adapter->CreateBuffer(PagePool.GetHeapType(), GetGPUMask(), GetVisibilityMask(), Size + Alignment, &Resource, std::format("Stand Alone Fast Allocation {}", UniqueID).c_str()));
		ResourceLocation->AsStandAlone(Resource, Size + Alignment);

		return PagePool.IsCPUWritable() ? Resource->GetResourceBaseAddress() : nullptr;
	}
	else
	{
		std::unique_lock Lock{ CS };

		const uint32 Offset = (CurrentAllocatorPage) ? CurrentAllocatorPage->NextFastAllocOffset : 0;
		uint32 CurrentOffset = AlignArbitrary(Offset, Alignment);

		// See if there is room in the current pool
		if (CurrentAllocatorPage == nullptr || PagePool.GetPageSize() < CurrentOffset + Size)
		{
			if (CurrentAllocatorPage)
			{
				PagePool.ReturnFastAllocatorPage(CurrentAllocatorPage);
			}
			CurrentAllocatorPage = PagePool.RequestFastAllocatorPage();

			CurrentOffset = AlignArbitrary(CurrentAllocatorPage->NextFastAllocOffset, Alignment);
		}

		wconstraint(PagePool.GetPageSize() - Size >= CurrentOffset);

		// Create a FD3D12ResourceLocation representing a sub-section of the pool resource
		ResourceLocation->AsFastAllocation(CurrentAllocatorPage->FastAllocBuffer,
			Size,
			CurrentAllocatorPage->FastAllocBuffer->GetGPUVirtualAddress(),
			CurrentAllocatorPage->FastAllocData,
			0,
			CurrentOffset);

		CurrentAllocatorPage->NextFastAllocOffset = CurrentOffset + Size;
		CurrentAllocatorPage->UpdateFence();

		wconstraint(ResourceLocation->GetMappedBaseAddress());
		return ResourceLocation->GetMappedBaseAddress();
	}
}

void FastAllocator::CleanupPages(uint64 FrameLag)
{
	std::unique_lock Lock{ CS };
	PagePool.CleanupPages(FrameLag);
}

void FastAllocator::Destroy()
{
	std::unique_lock Lock{ CS };
	if (CurrentAllocatorPage)
	{
		PagePool.ReturnFastAllocatorPage(CurrentAllocatorPage);
		CurrentAllocatorPage = nullptr;
	}

	PagePool.Destroy();
}

FastAllocatorPagePool::FastAllocatorPagePool(NodeDevice* Parent, GPUMaskType VisibiltyMask, D3D12_HEAP_TYPE InHeapType, uint32 Size)
	: DeviceChild(Parent), MultiNodeGPUObject(Parent->GetGPUMask(), VisibiltyMask)
	, PageSize(Size)
	, HeapProperties(CD3DX12_HEAP_PROPERTIES(InHeapType, Parent->GetGPUMask().GetNative(), VisibiltyMask.GetNative()))
{};

FastAllocatorPagePool::FastAllocatorPagePool(NodeDevice* Parent, GPUMaskType VisibiltyMask, const D3D12_HEAP_PROPERTIES& InHeapProperties, uint32 Size)
	: DeviceChild(Parent), MultiNodeGPUObject(Parent->GetGPUMask(), VisibiltyMask)
	, PageSize(Size)
	, HeapProperties(InHeapProperties)
{};

FastAllocatorPage* FastAllocatorPagePool::RequestFastAllocatorPage()
{
	auto Device = GetParentDevice();
	auto Adapter = Device->GetParentAdapter();
	auto& Fence = Adapter->GetFrameFence();

	const uint64 CompletedFence = Fence.UpdateLastCompletedFence();

	for (int32 Index = 0; Index < static_cast<int32>(Pool.size()); Index++)
	{
		FastAllocatorPage* Page = Pool[Index];

		//If the GPU is done with it and no-one has a lock on it
		if (Page->FastAllocBuffer->GetRefCount() == 1 &&
			Page->FrameFence <= CompletedFence)
		{
			Page->Reset();
			Pool.erase(Pool.begin() + Index);
			return Page;
		}
	}

	FastAllocatorPage* Page = new FastAllocatorPage(PageSize);

	const D3D12_RESOURCE_STATES InitialState = DetermineInitialResourceState(HeapProperties.Type, &HeapProperties);
	CheckHResult(Adapter->CreateBuffer(HeapProperties, GetGPUMask(), InitialState, InitialState, PageSize, &Page->FastAllocBuffer, "Fast Allocator Page"));

	Page->FastAllocData = Page->FastAllocBuffer->Map();

	return Page;
}

void FastAllocatorPage::UpdateFence()
{
	// Fence value must be updated every time the page is used to service an allocation.
	// Max() is required as fast allocator may be used from Render or RHI thread,
	// which have different fence values. See Fence::GetCurrentFence() implementation.
	auto Adapter = static_cast<D3D12Adapter*>(&GRenderIF->GetDevice());
	FrameFence = std::max(FrameFence, Adapter->GetFrameFence().GetCurrentFence());
}

void FastAllocatorPagePool::ReturnFastAllocatorPage(FastAllocatorPage* Page)
{
	// Extend the lifetime of these resources when in AFR as other nodes might be relying on this
	Page->UpdateFence();
	Pool.emplace_back(Page);
}

void FastAllocatorPagePool::CleanupPages(uint64 FrameLag)
{
	if (Pool.size() <= FastAllocatorMinPagesToRetain)
	{
		return;
	}

	auto Adapter = GetParentDevice()->GetParentAdapter();
	auto& Fence = Adapter->GetFrameFence();

	const uint64 CompletedFence = Fence.UpdateLastCompletedFence();

	// Pages get returned to end of list, so we'll look for pages to delete, starting from the LRU
	for (int32 Index = 0; Index < static_cast<int32>(Pool.size()); Index++)
	{
		FastAllocatorPage* Page = Pool[Index];

		//If the GPU is done with it and no-one has a lock on it
		if (Page->FastAllocBuffer->GetRefCount() == 1 &&
			Page->FrameFence + FrameLag <= CompletedFence)
		{
			Pool.erase(Pool.begin() + Index);
			delete(Page);

			// Only release at most one page per frame			
			return;
		}
	}
}

void FastAllocatorPagePool::Destroy()
{
	for (std::size_t i = 0; i < Pool.size(); i++)
	{
		//check(Pool[i]->FastAllocBuffer->GetRefCount() == 1);
		{
			FastAllocatorPage* Page = Pool[i];
			delete(Page);
			Page = nullptr;
		}
	}

	Pool.clear();
}

void* FastConstantAllocator::Allocate(uint32 Bytes, ResourceLocation& OutLocation)
{
	wassume(Bytes <= PageSize);

	const uint32 AlignedSize = Align(Bytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

	if (Offset + AlignedSize > PageSize)
	{
		Offset = 0;

		auto* Device = GetParentDevice();
		auto* Adapter = Device->GetParentAdapter();

		UploadHeapAllocator& Allocator = Adapter->GetUploadHeapAllocator(Device->GetGPUIndex());
		Allocator.AllocFastConstantAllocationPage(PageSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT, UnderlyingResource);
	}

	OutLocation.AsFastAllocation(
		UnderlyingResource.GetResource(),
		AlignedSize,
		UnderlyingResource.GetGPUVirtualAddress(),
		UnderlyingResource.GetMappedBaseAddress(),
		UnderlyingResource.GetOffsetFromBaseOfResource(), // AllocUploadResource returns a suballocated resource where we're suballocating (again) from
		Offset
	);

	Offset += AlignedSize;

	return OutLocation.GetMappedBaseAddress();
}
