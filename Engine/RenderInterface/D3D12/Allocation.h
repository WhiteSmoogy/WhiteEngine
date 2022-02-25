#pragma once

#include "NodeDevice.h"

namespace platform_ex::Windows::D3D12
{
	class ResourceAllocator;

	struct BuddyAllocatorPrivateData
	{
		uint32 Offset;
		uint32 Order;
	};

	class ResourceLocation :public DeviceChild,public white::noncopyable
	{
	public:
		enum LocationType
		{
			Undefined,
			SubAllocation,
			FastAllocation,
		};

		ResourceLocation(NodeDevice* Parent);
		~ResourceLocation();

		void SetResource(ResourceHolder* Value);
		inline void SetType(LocationType Value) { Type = Value; }
		inline void SetMappedBaseAddress(void* Value) { MappedBaseAddress = Value; }
		inline void SetGPUVirtualAddress(D3D12_GPU_VIRTUAL_ADDRESS Value) { GPUVirtualAddress = Value; }
		inline void SetOffsetFromBaseOfResource(uint64 Value) { OffsetFromBaseOfResource = Value; }
		inline void SetSize(uint64 Value) { Size = Value; }
		inline void SetAllocator(ResourceAllocator* Value) { Allocator = Value; }

		inline ResourceHolder* GetResource() const { return UnderlyingResource; }
		inline void* GetMappedBaseAddress() const { return MappedBaseAddress; }
		inline D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const { return GPUVirtualAddress; }
		inline uint64 GetOffsetFromBaseOfResource() const { return OffsetFromBaseOfResource; }
		inline uint64 GetSize() const { return Size; }
		inline ResourceAllocator* GetAllocator() const {return Allocator; }

		inline BuddyAllocatorPrivateData& GetBuddyAllocatorPrivateData() { return AllocatorData.BuddyPrivateData; }


		void AsFastAllocation(ResourceHolder* Resource, uint32 BufferSize, D3D12_GPU_VIRTUAL_ADDRESS GPUBase, void* CPUBase, uint64 ResourceOffsetBase, uint64 Offset, bool bMultiFrame = false)
		{
			SetType(FastAllocation);
			SetResource(Resource);
			SetSize(BufferSize);
			SetOffsetFromBaseOfResource(ResourceOffsetBase + Offset);

			if (CPUBase != nullptr)
			{
				SetMappedBaseAddress((uint8*)CPUBase + Offset);
			}
			SetGPUVirtualAddress(GPUBase + Offset);
		}

		void Clear();

		static void TransferOwnership(ResourceLocation& Destination, ResourceLocation& Source);
	private:
		void ClearResource();
		void ClearMembers();

		LocationType Type;

		ResourceHolder* UnderlyingResource;

		union
		{
			ResourceAllocator* Allocator;
		};

		union PrivateAllocatorData
		{
			BuddyAllocatorPrivateData BuddyPrivateData;
		} AllocatorData;

		void* MappedBaseAddress;
		D3D12_GPU_VIRTUAL_ADDRESS GPUVirtualAddress;
		uint64 OffsetFromBaseOfResource;

		// The size the application asked for
		uint64 Size;
	};

	class ResourceAllocator :public DeviceChild, public MultiNodeGPUObject
	{
	public:
		ResourceAllocator(NodeDevice* InParentDevice, GPUMaskType VisibleNodes)
			:DeviceChild(InParentDevice),MultiNodeGPUObject(InParentDevice->GetGPUMask(),VisibleNodes)
		{}

		virtual void Deallocate(ResourceLocation& ResourceLocation);
	};

	class FastConstantPageAllocator :public ResourceAllocator
	{
	public:
		FastConstantPageAllocator(NodeDevice* InParentDevice, GPUMaskType VisibleNodes)
			:ResourceAllocator(InParentDevice, VisibleNodes)
		{}

		bool TryAllocate(uint32 SizeInBytes, uint32 Alignment, ResourceLocation& ResourceLocation);

		void CleanUpAllocations(uint64 InFrameLag);
	private:
		using ResourceAllocator::ResourceAllocator;

		class ConstantAllocator :public ResourceAllocator{
		public:
			ConstantAllocator(NodeDevice* InParentDevice, GPUMaskType VisibleNodes, uint32 InBlockSize)
				:ResourceAllocator(InParentDevice,VisibleNodes),BlockSize(InBlockSize), TotalSizeUsed(0),BackingResource(nullptr), DelayCreated(false)
				, RetireFrameFence(-1)
			{}

			~ConstantAllocator();

			bool TryAllocate(uint32 SizeInBytes, uint32 Alignment, ResourceLocation& ResourceLocation);

			void Deallocate(ResourceLocation & ResourceLocation);

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

	struct AllocatorConfig
	{
		D3D12_HEAP_TYPE HeapType = D3D12_HEAP_TYPE_UPLOAD;
		D3D12_HEAP_FLAGS HeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
		D3D12_RESOURCE_FLAGS ResourceFlags = D3D12_RESOURCE_FLAG_NONE;
		D3D12_RESOURCE_STATES InitialResourceState = D3D12_RESOURCE_STATE_GENERIC_READ;
	};

	enum class AllocationStrategy
	{
		// This strategy uses Placed Resources to sub-allocate a buffer out of an underlying ID3D12Heap.
	// The benefit of this is that each buffer can have it's own resource state and can be treated
	// as any other buffer. The downside of this strategy is the API limitation which enforces
	// the minimum buffer size to 64k leading to large internal fragmentation in the allocator
		kPlacedResource,
		// The alternative is to manually sub-allocate out of a single large buffer which allows block
		// allocation granularity down to 1 byte. However, this strategy is only really valid for buffers which
		// will be treated as read-only after their creation (i.e. most Index and Vertex buffers). This 
		// is because the underlying resource can only have one state at a time.
		kManualSubAllocation
	};

	class ResourceConfigAllocator :public ResourceAllocator
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

	class BuddyAllocator :public ResourceConfigAllocator
	{
	public:
		BuddyAllocator(NodeDevice* InParentDevice, GPUMaskType VisibleNodes,
			const AllocatorConfig& InConfig,
			const std::string& Name,
			AllocationStrategy InStrategy,
			uint32 InMaxAllocationSize,
			uint32 InMaxBlockSize,
			uint32 InMinBlockSize);

		void Initialize();

		bool TryAllocate(uint32 SizeInBytes, uint32 Alignment,ResourceLocation& ResourceLocation);

		void Deallocate(ResourceLocation& ResourceLocation) final;

		inline bool IsOwner(ResourceLocation& ResourceLocation)
		{
			return ResourceLocation.GetAllocator() == (ResourceAllocator*)this;
		}

		void CleanUpAllocations();

		inline uint64 GetLastUsedFrameFence() const { return LastUsedFrameFence; }


		bool IsEmpty() const { return TotalSizeUsed == MinBlockSize; }

		~BuddyAllocator()
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

	class MultiBuddyAllocator :public ResourceConfigAllocator
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

		BuddyAllocator* CreateNewAllocator(uint32 InMinSizeInBytes);

		const AllocationStrategy Strategy;
		const uint32 MinBlockSize;
		const uint32 DefaultPoolSize;

		std::vector<BuddyAllocator*> Allocators;
	};

	// This is designed for allocation of scratch memory such as temporary staging buffers
	// or shadow buffers for dynamic resources.
	class UploadHeapAllocator : public AdapterChild, public DeviceChild, public MultiNodeGPUObject
	{
	public:
		UploadHeapAllocator(D3D12Adapter* InParent, NodeDevice* InParentDevice, const std::string& InName);

		void Destroy();

		void* AllocFastConstantAllocationPage(uint32 InSize, uint32 InAlignment,ResourceLocation& ResourceLocation);

		void* AllocUploadResource(uint32 InSize, uint32 InAlignment, ResourceLocation& ResourceLocation);

		void CleanUpAllocations(uint64 InFrameLag);

	private:
		MultiBuddyAllocator SmallBlockAllocator;

		// Seperate allocator used for the fast constant allocator pages which get always freed within the same frame by default
		// (different allocator to avoid fragmentation with the other pools - always the same size allocations)
		FastConstantPageAllocator FastConstantAllocator;
	};

	class FastConstantAllocator :public DeviceChild,public MultiNodeGPUObject
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