#pragma once

#include "Common.h"
#include "ResourceHolder.h"
#include "RenderInterface/IFormat.hpp"
#include <shared_mutex>

namespace platform_ex::Windows::D3D12
{
	using platform::Render::EAccessHint;
	class NodeDevice;

	constexpr uint32 kBufferAlignment = 64 * 1024;

	constexpr uint32 kManualSubAllocationAlignment = 256;

	class MemoryPool :public DeviceChild, public MultiNodeGPUObject
	{
	public:
		enum class FreeListOrder
		{
			SortBySize,
			SortByOffset,
		};

		enum class PoolResouceTypes
		{
			Buffers = 0x1,
		};

		MemoryPool(NodeDevice* InParentDevice, GPUMaskType VisibleNodes,
			const AllocatorConfig& InConfig,
			const std::string& Name,
			AllocationStrategy InAllocationStrategy,
			uint16 InPoolIndex,
			uint64 InPoolSize, uint32 InPoolAlignment,
			PoolResouceTypes InAllocationResourceType, FreeListOrder InFreeListOrder);


		ResourceHolder* GetBackingResource() const { return BackingResource.Get(); }

		HeapHolder* GetBackingHeap() const { return BackingHeap.Get(); }

		uint64 GetUsedSize() const { return PoolSize - FreeSize; }

		uint64 GetLastUsedFrameFence() const { return LastUsedFrameFence; }

		int16 GetPoolIndex() const { return PoolIndex; }

		uint64 GetPoolSize() const { return PoolSize; }

		bool IsEmpty() const { return GetUsedSize() == 0; }

		bool IsResourceTypeSupported(MemoryPool::PoolResouceTypes InAllocationResourceType) { return InAllocationResourceType == SupportResouceTypes; }

		bool IsFull() { return FreeSize == 0; }

		bool TryAllocate(uint32 InSizeInBytes, uint32 InAllocationAlignment, PoolResouceTypes InAllocationResourceType, PoolAllocatorPrivateData& AllocationData);

		int32 FindFreeBlock(uint32 InSizeInBytes, uint32 InAllocationAlignment);

		void Deallocate(PoolAllocatorPrivateData& InData);

		PoolAllocatorPrivateData* AddToFreeBlocks(PoolAllocatorPrivateData* InFreeBlock);

		PoolAllocatorPrivateData* GetNewAllocationData();
		void ReleaseAllocationData(PoolAllocatorPrivateData* InData);

		void RemoveFromFreeBlocks(PoolAllocatorPrivateData* InFreeBlock);

		void UpdateLastUsedFrameFence(uint64 InFrameFence) { LastUsedFrameFence = std::max(LastUsedFrameFence, InFrameFence); }

	private:
		int16 PoolIndex;
		uint64 PoolSize;
		uint32 PoolAlignment;
		PoolResouceTypes SupportResouceTypes;
		FreeListOrder ListOrder;
		uint64 FreeSize;
		std::vector<PoolAllocatorPrivateData*> FreeBlocks;
		std::vector< PoolAllocatorPrivateData*> AllocationDataPool;

		PoolAllocatorPrivateData HeadBlock;

		const AllocatorConfig InitConfig;
		const std::string Name;
		AllocationStrategy Strategy;
		uint64 LastUsedFrameFence;

		COMPtr<HeapHolder> BackingHeap;
		COMPtr<ResourceHolder> BackingResource;
	};

	struct HeapAndOffset
	{
		HeapHolder* Heap;
		uint64 Offset;
	};



	template<MemoryPool::FreeListOrder Order, bool Defrag = false>
	class PoolAllocator :public DeviceChild, public MultiNodeGPUObject, public IPoolAllocator
	{
	public:
		PoolAllocator(NodeDevice* InParentDevice, GPUMaskType VisibleNodes,
			const AllocatorConfig& InConfig,
			const std::string& Name,
			AllocationStrategy InAllocationStrategy, uint64 InDefaultPoolSize, uint32 InPoolAlignment, uint32 InMaxAllocationSize);

		~PoolAllocator();

		void CleanUpAllocations(uint64 InFrameLag);

		bool  SupportsAllocation(D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_FLAGS InResourceFlags, uint32 InBufferAccess, ResourceStateMode InResourceStateMode) const final;

		void AllocDefaultResource(D3D12_HEAP_TYPE InHeapType, const D3D12_RESOURCE_DESC& InDesc, uint32 InBufferAccess, ResourceStateMode InResourceStateMode,
			D3D12_RESOURCE_STATES InCreateState, uint32 InAlignment, ResourceLocation& ResourceLocation, const char* InName) final;

		void AllocateResource(uint32 GPUIndex, D3D12_HEAP_TYPE InHeapType, const D3D12_RESOURCE_DESC& InDesc, uint64 InSize, uint32 InAllocationAlignment, ResourceStateMode InResourceStateMode,
			D3D12_RESOURCE_STATES InCreateState, const D3D12_CLEAR_VALUE* InClearValue, const char* InName, ResourceLocation& ResourceLocation);

		template<MemoryPool::PoolResouceTypes InAllocationResourceType>
		bool TryAllocateInternal(uint32 InSizeInBytes, uint32 InAllocationAlignment, PoolAllocatorPrivateData& AllocationData);

		ResourceHolder* GetBackingResource(ResourceLocation& InResouceLocation) const;

		bool IsOwner(ResourceLocation& ResourceLocation) const { return ResourceLocation.GetPoolAllocator() == static_cast<const IPoolAllocator*>(this); }

		ResourceHolder* CreatePlacedResource(
			const PoolAllocatorPrivateData& InAllocationData,
			const D3D12_RESOURCE_DESC& InDesc,
			D3D12_RESOURCE_STATES InCreateState,
			ResourceStateMode InResourceStateMode,
			const D3D12_CLEAR_VALUE* InClearValue,
			const char* InName);

		HeapAndOffset GetBackingHeapAndAllocationOffsetInBytes(const PoolAllocatorPrivateData& InAllocationData) const;

		MemoryPool* CreateNewPool(int16 InPoolIndex, uint32 InMinimumAllocationSize, MemoryPool::PoolResouceTypes InAllocationResourceType);

		void Deallocate(ResourceLocation& ResourceLocation) override;

		void TransferOwnership(ResourceLocation& Destination, ResourceLocation& Source) override;
	protected:
		struct FrameFencedAllocationData
		{
			enum class EOperation
			{
				Invalid,
				Deallocate,
				Unlock,
				Nop,
			};

			EOperation Operation = EOperation::Invalid;
			ResourceHolder* PlacedResource = nullptr;
			uint64 FrameFence = 0;
			PoolAllocatorPrivateData* AllocationData = nullptr;
		};
	private:
		const uint64 DefaultPoolSize;
		const uint32 PoolAlignment;
		const uint64 MaxAllocationSize;

		std::vector<MemoryPool*> Pools;

		std::vector<uint32> PoolAllocationOrder;

		const AllocatorConfig InitConfig;
		const std::string Name;
		AllocationStrategy Strategy;

		// All operations which need to happen on specific frame fences
		std::vector<FrameFencedAllocationData> FrameFencedOperations;

		std::shared_mutex CS;

		std::vector< PoolAllocatorPrivateData*> AllocationDataPool;

		PoolAllocatorPrivateData* GetNewAllocationData();
		void ReleaseAllocationData(PoolAllocatorPrivateData* InData);

		void DeallocateInternal(PoolAllocatorPrivateData& InData);
	};



	class ResourceConfigAllocator :public DeviceChild, public MultiNodeGPUObject
	{
	public:
		ResourceConfigAllocator(NodeDevice* InParentDevice, GPUMaskType VisibleNodes,
			const AllocatorConfig& InConfig,
			const std::string& Name,
			uint32 InMaxAllocationSize);

		const AllocatorConfig& GetInitConfig() const { return InitConfig; }
		const uint32 GetMaximumAllocationSizeForPooling() const { return MaximumAllocationSizeForPooling; }
	protected:
		const AllocatorConfig InitConfig;
		const std::string DebugName;
		bool Initialized;

		// Any allocation larger than this just gets straight up allocated (i.e. not pooled).
		// These large allocations should be infrequent so the CPU overhead should be minimal
		const uint32 MaximumAllocationSizeForPooling;

		std::recursive_mutex CS;
	};

	class BuddyUploadAllocator :public ResourceConfigAllocator, public ISubDeAllocator
	{
	public:
		BuddyUploadAllocator(NodeDevice* InParentDevice, GPUMaskType VisibleNodes,
			const AllocatorConfig& InConfig,
			const std::string& Name,
			AllocationStrategy InStrategy,
			uint32 InMaxAllocationSize,
			uint32 InMaxBlockSize,
			uint32 InMinBlockSize);

		void Initialize();

		bool TryAllocate(uint32 SizeInBytes, uint32 Alignment, ResourceLocation& ResourceLocation);

		void Deallocate(ResourceLocation& ResourceLocation) final;

		inline bool IsOwner(ResourceLocation& ResourceLocation)
		{
			return ResourceLocation.GetSubDeAllocator() == (ISubDeAllocator*)this;
		}

		void CleanUpAllocations();

		inline uint64 GetLastUsedFrameFence() const { return LastUsedFrameFence; }


		bool IsEmpty() const { return TotalSizeUsed == MinBlockSize; }

		~BuddyUploadAllocator()
		{
			ReleaseAllResources();
		}
		void ReleaseAllResources();
	protected:
		const uint32 MaxBlockSize;
		const uint32 MinBlockSize;
		const AllocationStrategy Strategy;

		ResourceHolder* BackingResource;
	private:
		struct RetiredBlock
		{
			uint64 FrameFence;

			BuddyAllocatorPrivateData Data;
		};

		std::vector<RetiredBlock> DeferredDeletionQueue;

		uint64 LastUsedFrameFence;

		uint32 MaxOrder;

		uint32 TotalSizeUsed;

		struct FreeBlock
		{
			FreeBlock* Next;
		};

		FreeBlock** FreeBlockArray;

		inline uint32 SizeToUnitSize(uint32 size) const
		{
			return (size + (MinBlockSize - 1)) / MinBlockSize;
		}

		inline uint32 UnitSizeToOrder(uint32 size) const
		{
			unsigned long Result;
			_BitScanReverse(&Result, size + size - 1); // ceil(log2(size))
			return Result;
		}

		uint32 OrderToUnitSize(uint32 order) const { return ((uint32)1) << order; }

		inline uint32 GetBuddyOffset(const uint32& offset, const uint32& size)
		{
			return offset ^ size;
		}

		uint32 AllocateBlock(uint32 order);

		bool CanAllocate(uint32 size, uint32 aligment);

		void Allocate(uint32 SizeInBytes, uint32 Alignment, ResourceLocation& ResourceLocation);

		void DeallocateInternal(RetiredBlock& Block);
		void DeallocateBlock(uint32 Offset, uint32 Order);
	};

	template<bool Constant, bool Upload>
	class MultiBuddyAllocator;

	template<>
	class MultiBuddyAllocator<false, true> :public ResourceConfigAllocator, public ISubDeAllocator
	{
	public:
		MultiBuddyAllocator(
			NodeDevice* InParentDevice, GPUMaskType VisibleNodes,
			const AllocatorConfig& InConfig,
			const std::string& Name,
			AllocationStrategy InStrategy,
			uint32 InMaxAllocationSize,
			uint32 InDefaultPoolSize,
			uint32 InMinBlockSize
		);

		bool TryAllocate(uint32 SizeInBytes, uint32 Alignment, ResourceLocation& ResourceLocation);

		void Deallocate(ResourceLocation& ResourceLocation) final;

		void CleanUpAllocations(uint64 InFrameLag);
	protected:

		BuddyUploadAllocator* CreateNewAllocator(uint32 InMinSizeInBytes);

		const AllocationStrategy Strategy;
		const uint32 MinBlockSize;
		const uint32 DefaultPoolSize;

		std::vector<BuddyUploadAllocator*> Allocators;
	};

	template<>
	class MultiBuddyAllocator<true, true> :public DeviceChild, public MultiNodeGPUObject
	{
	public:
		MultiBuddyAllocator(NodeDevice* InParentDevice, GPUMaskType VisibleNodes);

		bool TryAllocate(uint32 SizeInBytes, uint32 Alignment, ResourceLocation& ResourceLocation);

		void CleanUpAllocations(uint64 InFrameLag);
	private:

		class ConstantAllocator :public DeviceChild, public MultiNodeGPUObject, public ISubDeAllocator {
		public:
			ConstantAllocator(NodeDevice* InParentDevice, GPUMaskType VisibleNodes, uint32 InBlockSize);

			~ConstantAllocator();

			bool TryAllocate(uint32 SizeInBytes, uint32 Alignment, ResourceLocation& ResourceLocation);

			void Deallocate(ResourceLocation& ResourceLocation);

			uint64 GetLastUsedFrameFence() const { return RetireFrameFence; }

			bool IsDeallocated() const { return RetireFrameFence != -1; }
		private:
			void CreateBackingResource();

			const uint32 BlockSize;

			uint32 TotalSizeUsed;

			ResourceHolder* BackingResource;

			uint64 RetireFrameFence;

			bool DelayCreated;
		};

		ConstantAllocator* CreateNewAllocator(uint32 InMinSizeInBytes);

		std::vector<ConstantAllocator*> Allocators;

		std::mutex CS;
	};


	// This is designed for allocation of scratch memory such as temporary staging buffers
	// or shadow buffers for dynamic resources.
	class UploadHeapAllocator : public AdapterChild, public DeviceChild, public MultiNodeGPUObject
	{
	public:
		UploadHeapAllocator(D3D12Adapter* InParent, NodeDevice* InParentDevice, const std::string& InName);

		void* AllocFastConstantAllocationPage(uint32 InSize, uint32 InAlignment, ResourceLocation& ResourceLocation);

		void* AllocUploadResource(uint32 InSize, uint32 InAlignment, ResourceLocation& ResourceLocation);

		void CleanUpAllocations(uint64 InFrameLag);

	private:
		MultiBuddyAllocator<false, true> SmallBlockAllocator;
		// Pool allocator for all bigger allocations - less fast but less alignment waste
		PoolAllocator<MemoryPool::FreeListOrder::SortByOffset, false>  BigBlockAllocator;
		// Seperate allocator used for the fast constant allocator pages which get always freed within the same frame by default
		// (different allocator to avoid fragmentation with the other pools - always the same size allocations)
		MultiBuddyAllocator<true, true> FastConstantAllocator;
	};

	using BufferPool = IPoolAllocator;

	class BufferAllocator :public DeviceChild, public MultiNodeGPUObject
	{
	public:
		BufferAllocator(NodeDevice* Parent, GPUMaskType VisibleNodes);

		template<ResourceStateMode mode>
		void AllocDefaultResource(D3D12_HEAP_TYPE InHeapType, const D3D12_RESOURCE_DESC& pDesc, uint32 InBufferAccess, D3D12_RESOURCE_STATES InCreateState, ResourceLocation& ResourceLocation, uint32 Alignment, const char* Name);

		void CleanUpAllocations(uint64 InFrameLag);

		template<ResourceStateMode mode>
		static bool IsPlacedResource(D3D12_RESOURCE_FLAGS InResourceFlags);

		template<ResourceStateMode mode>
		static D3D12_RESOURCE_STATES GetDefaultInitialResourceState(D3D12_HEAP_TYPE InHeapType, uint32 InBufferAccess);
	private:
		BufferPool* CreateBufferPool(D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_FLAGS InResourceFlags, uint32 InBufferAccess, ResourceStateMode InResourceStateMode);


		std::vector< BufferPool*> DefaultBufferPools;
	};

	struct FastAllocatorPage
	{
		FastAllocatorPage()
			: PageSize(0)
			, NextFastAllocOffset(0)
			, FastAllocData(nullptr)
			, FrameFence(0) {};

		FastAllocatorPage(uint32 Size)
			: PageSize(Size)
			, NextFastAllocOffset(0)
			, FastAllocData(nullptr)
			, FrameFence(0) {};

		~FastAllocatorPage()
		{
			if (FastAllocBuffer)
				FastAllocBuffer->Release();
		}

		void Reset()
		{
			NextFastAllocOffset = 0;
			FrameFence = 0;
		}

		void UpdateFence();

		const uint32 PageSize;
		ResourceHolder* FastAllocBuffer;
		uint32 NextFastAllocOffset;
		void* FastAllocData;
		uint64 FrameFence;
	};

	class FastAllocatorPagePool : public DeviceChild, public MultiNodeGPUObject
	{
	public:
		FastAllocatorPagePool(NodeDevice* Parent, GPUMaskType InGpuMask, D3D12_HEAP_TYPE InHeapType, uint32 Size);

		FastAllocatorPagePool(NodeDevice* Parent, GPUMaskType InGpuMask, const D3D12_HEAP_PROPERTIES& InHeapProperties, uint32 Size);

		FastAllocatorPage* RequestFastAllocatorPage();
		void ReturnFastAllocatorPage(FastAllocatorPage* Page);
		void CleanupPages(uint64 FrameLag);

		inline uint32 GetPageSize() const { return PageSize; }

		inline D3D12_HEAP_TYPE GetHeapType() const { return HeapProperties.Type; }
		inline bool IsCPUWritable() const { return platform_ex::Windows::D3D12::IsCPUWritable(GetHeapType(), &HeapProperties); }

		void Destroy();

	protected:

		const uint32 PageSize;
		const D3D12_HEAP_PROPERTIES HeapProperties;

		std::vector<FastAllocatorPage*> Pool;
	};

	class FastAllocator :public DeviceChild, public MultiNodeGPUObject
	{
	public:
		FastAllocator(NodeDevice* Parent, GPUMaskType InGpuMask, D3D12_HEAP_TYPE InHeapType, uint32 PageSize);
		FastAllocator(NodeDevice* Parent, GPUMaskType InGpuMask, const D3D12_HEAP_PROPERTIES& InHeapProperties, uint32 PageSize);

		void* Allocate(uint32 Size, uint32 Alignment, class ResourceLocation* ResourceLocation);
		void Destroy();

		void CleanupPages(uint64 FrameLag);

	protected:
		FastAllocatorPagePool PagePool;

		FastAllocatorPage* CurrentAllocatorPage;

		std::shared_mutex CS;
	};

	class FastConstantAllocator :public DeviceChild, public MultiNodeGPUObject
	{
	public:
		FastConstantAllocator(NodeDevice* Parent, GPUMaskType InGpuMask);

		void* Allocate(uint32 Bytes, ResourceLocation& OutLocation);
	private:
		ResourceLocation UnderlyingResource;

		uint32 Offset;
		uint32 PageSize;
	};
}