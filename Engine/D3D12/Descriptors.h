#pragma once

#include "Convert.h"
#include "ResourceHolder.h"
#include <mutex>
#include <queue>

namespace platform_ex::Windows::D3D12 {
	using platform::Render::DescriptorHeapType;

	struct DescriptorHeap : public DeviceChild, public platform::Render::RObject
	{
	public:
		DescriptorHeap() = delete;

		// Heap created with its own D3D heap object.
		DescriptorHeap(NodeDevice* InDevice, COMPtr<ID3D12DescriptorHeap> InHeap, uint32 InNumDescriptors, DescriptorHeapType InType, D3D12_DESCRIPTOR_HEAP_FLAGS InFlags, bool bInIsGlobal);

		// Heap created as a suballocation of another heap.
		DescriptorHeap(DescriptorHeap* SubAllocateSourceHeap, uint32 InOffset, uint32 InNumDescriptors);

		~DescriptorHeap();

		inline ID3D12DescriptorHeap* GetHeap()  const { return Heap.Get(); }
		inline DescriptorHeapType      GetType()  const { return Type; }
		inline D3D12_DESCRIPTOR_HEAP_FLAGS GetFlags() const { return Flags; }


		inline uint32 GetOffset()         const { return Offset; }
		inline uint32 GetNumDescriptors() const { return NumDescriptors; }
		inline uint32 GetDescriptorSize() const { return DescriptorSize; }
		inline bool   IsGlobal()          const { return bIsGlobal; }
		inline bool   IsSuballocation()   const { return bIsSuballocation; }

		inline uint32 GetMemorySize() const { return DescriptorSize * NumDescriptors; }

		inline D3D12_CPU_DESCRIPTOR_HANDLE GetCPUSlotHandle(uint32 Slot) const { return CD3DX12_CPU_DESCRIPTOR_HANDLE(CpuBase, Slot, DescriptorSize); }
		inline D3D12_GPU_DESCRIPTOR_HANDLE GetGPUSlotHandle(uint32 Slot) const { return CD3DX12_GPU_DESCRIPTOR_HANDLE(GpuBase, Slot, DescriptorSize); }

	private:
		COMPtr<ID3D12DescriptorHeap> Heap;

		const CD3DX12_CPU_DESCRIPTOR_HANDLE CpuBase;
		const CD3DX12_GPU_DESCRIPTOR_HANDLE GpuBase;

		// Offset in descriptors into the heap, only used when heap is suballocated.
		const uint32 Offset = 0;

		// Total number of descriptors in this heap.
		const uint32 NumDescriptors;

		// Device provided size of each descriptor in this heap.
		const uint32 DescriptorSize;

		const DescriptorHeapType Type;
		const D3D12_DESCRIPTOR_HEAP_FLAGS Flags;

		// Enabled if this heap is the "global" heap.
		const bool bIsGlobal;

		// Enabled if this heap was allocated inside another heap.
		const bool bIsSuballocation;
	};

	struct DescriptorHandle
	{
		const constexpr static uint32 MaxIndex = (1 << 30) - 1;

		DescriptorHandle() = default;
		DescriptorHandle(DescriptorHeapType InType, uint32 InIndex)
			: Index(InIndex)
			, Type((uint8)InType)
		{
		}
		DescriptorHandle(uint8 InType, uint32 InIndex)
			: Index(InIndex)
			, Type(InType)
		{
		}

		inline uint32                 GetIndex() const { return Index; }
		inline DescriptorHeapType GetType() const { return (DescriptorHeapType)Type; }
		inline uint8                  GetRawType() const { return Type; }

		inline bool IsValid() const { return Index != MaxIndex && Type != (uint8)DescriptorHeapType::Count; }

	private:
		uint32    Index : 29{ MaxIndex };
		uint32    Type  : 3{ (uint8)DescriptorHeapType::Count };
	};

	struct DescriptorAllocatorRange
	{
		uint32 First;
		uint32 Last;
	};

	class DescriptorAllocator
	{
	public:
		DescriptorAllocator()
		{}
		DescriptorAllocator(uint32 InNumDescriptors)
		{
			Init(InNumDescriptors);
		}
		 ~DescriptorAllocator()
		 {}

		 void Init(uint32 InNumDescriptors)
		 {
			 Capacity = InNumDescriptors;
			 Ranges.emplace_back(0, InNumDescriptors - 1);
		 }
		 void Shutdown()
		 {
			 Ranges.clear();
			 Capacity = 0;
		 }

		 DescriptorHandle Allocate(DescriptorHeapType InType)
		 {
			 uint32 Index{};
			 if (Allocate(1, Index))
			 {
				 return DescriptorHandle(InType, Index);
			 }
			 return DescriptorHandle();
		 }
		 void Free(DescriptorHandle InHandle)
		 {
			 if (InHandle.IsValid())
			 {
				 Free(InHandle.GetIndex(), 1);
			 }
		 }

		 bool Allocate(uint32 NumDescriptors, uint32& OutSlot)
		 {
			 std::unique_lock Lock{ Mutex };

			 if (const uint32 NumRanges = static_cast<uint32>(Ranges.size()); NumRanges > 0)
			 {
				 uint32 Index = 0;
				 do
				 {
					 auto& CurrentRange = Ranges[Index];
					 const uint32 Size = 1 + CurrentRange.Last - CurrentRange.First;
					 if (NumDescriptors <= Size)
					 {
						 uint32 First = CurrentRange.First;
						 if (NumDescriptors == Size && Index + 1 < NumRanges)
						 {
							 // Range is full and a new range exists, so move on to that one
							 Ranges.erase(Ranges.begin() + Index);
						 }
						 else
						 {
							 CurrentRange.First += NumDescriptors;
						 }
						 OutSlot = First;

						 return true;
					 }
					 ++Index;
				 } while (Index < NumRanges);
			 }

			 OutSlot = UINT_MAX;
			 return false;
		 }
		 void Free(uint32 Offset, uint32 NumDescriptors)
		 {
			 if (Offset == UINT_MAX || NumDescriptors == 0)
			 {
				 return;
			 }

			 std::unique_lock Lock{ Mutex };

			 const uint32 End = Offset + NumDescriptors;
			 // Binary search of the range list
			 uint32 Index0 = 0;
			 uint32 Index1 = static_cast<uint32>(Ranges.size()) - 1;
			 for (;;)
			 {
				 const uint32 Index = (Index0 + Index1) / 2;
				 if (Offset < Ranges[Index].First)
				 {
					 // Before current range, check if neighboring
					 if (End >= Ranges[Index].First)
					 {
						 wassume(End == Ranges[Index].First); // Can't overlap a range of free IDs
						 // Neighbor id, check if neighboring previous range too
						 if (Index > Index0 && Offset - 1 == Ranges[Index - 1].Last)
						 {
							 // Merge with previous range
							 Ranges[Index - 1].Last = Ranges[Index].Last;
							 Ranges.erase(Ranges.begin() + Index);
						 }
						 else
						 {
							 // Just grow range
							 Ranges[Index].First = Offset;
						 }

						 return;
					 }
					 else
					 {
						 // Non-neighbor id
						 if (Index != Index0)
						 {
							 // Cull upper half of list
							 Index1 = Index - 1;
						 }
						 else
						 {
							 // Found our position in the list, insert the deleted range here
							 Ranges.emplace(Ranges.begin() + Index, Offset, End - 1);

							 return;
						 }
					 }
				 }
				 else if (Offset > Ranges[Index].Last)
				 {
					 // After current range, check if neighboring
					 if (Offset - 1 == Ranges[Index].Last)
					 {
						 // Neighbor id, check if neighboring next range too
						 if (Index < Index1 && End == Ranges[Index + 1].First)
						 {
							 // Merge with next range
							 Ranges[Index].Last = Ranges[Index + 1].Last;
							 Ranges.erase(Ranges.begin() + Index + 1);
						 }
						 else
						 {
							 // Just grow range
							 Ranges[Index].Last += NumDescriptors;
						 }

						 return;
					 }
					 else
					 {
						 // Non-neighbor id
						 if (Index != Index1)
						 {
							 // Cull bottom half of list
							 Index0 = Index + 1;
						 }
						 else
						 {
							 // Found our position in the list, insert the deleted range here
							 Ranges.emplace(Ranges.begin() + Index + 1, Offset, End - 1);

							 return;
						 }
					 }
				 }
				 else
				 {
					 // Inside a free block, not a valid offset
					 throw white::unsupported();
				 }
			 }
		 }

		inline uint32 GetCapacity() const { return Capacity; }

	private:
		std::vector<DescriptorAllocatorRange> Ranges;
		uint32 Capacity = 0;

		std::mutex Mutex;
	};

	class HeapDescriptorAllocator : protected DescriptorAllocator
	{
	public:
		HeapDescriptorAllocator() = delete;
		 HeapDescriptorAllocator(DescriptorHeapType InType, uint32 InDescriptorCount)
			 :DescriptorAllocator(InDescriptorCount),
			 Type(InType)
		 {}

		 DescriptorHandle Allocate()
		 {
			 return DescriptorAllocator::Allocate(Type);
		 }
		 void Free(DescriptorHandle InHandle)
		 {
			 if (InHandle.IsValid())
			 {
				 wassume(InHandle.GetType() == Type);
				 DescriptorAllocator::Free(InHandle);
			 }
		 }

		 bool Allocate(uint32 NumDescriptors, uint32& OutSlot)
		 {
			 return DescriptorAllocator::Allocate(NumDescriptors, OutSlot);
		 }
		 void Free(uint32 Slot, uint32 NumDescriptors)
		 {
			 DescriptorAllocator::Free(Slot, NumDescriptors);
		 }

		using DescriptorAllocator::GetCapacity;
		inline DescriptorHeapType GetType() const { return Type; }

		inline bool HandlesAllocation(DescriptorHeapType InType) const { return GetType() == InType; }

	private:
		DescriptorHeapType Type;
	};

	/** Manager for resource descriptor allocations. */
	class DescriptorManager : public DeviceChild, public HeapDescriptorAllocator
	{
	public:
		DescriptorManager() = delete;
		DescriptorManager(NodeDevice* Device, DescriptorHeap* InHeap);
		~DescriptorManager();

		void UpdateImmediately(DescriptorHandle InHandle, D3D12_CPU_DESCRIPTOR_HANDLE InSourceCpuHandle);

		inline       DescriptorHeap* GetHeap() { return Heap.Get(); }
		inline const DescriptorHeap* GetHeap() const { return Heap.Get(); }

		inline bool HandlesAllocationWithFlags(DescriptorHeapType InHeapType, D3D12_DESCRIPTOR_HEAP_FLAGS InHeapFlags) const
		{
			return HandlesAllocation(InHeapType) && Heap->GetFlags() == InHeapFlags;
		}

		inline bool IsHeapAChild(const DescriptorHeap* InHeap)
		{
			return InHeap->GetHeap() == Heap->GetHeap();
		}

	private:
		COMPtr<DescriptorHeap> Heap;
	};

	/** Heap sub block of an online heap */
	struct OnlineDescriptorBlock : platform::Render::RObject
	{
		OnlineDescriptorBlock(uint32 InBaseSlot, uint32 InSize)
			: BaseSlot(InBaseSlot)
			, Size(InSize)
		{
		}

		uint32 BaseSlot;
		uint32 Size;
		uint32 SizeUsed = 0;
	};

	/** Primary online heap from which sub blocks can be allocated and freed. Used when allocating blocks of descriptors for tables. */
	class OnlineDescriptorManager : public DeviceChild
	{
	public:
		OnlineDescriptorManager(NodeDevice* Device);
		~OnlineDescriptorManager();

		// Setup the actual heap
		void Init(uint32 InTotalSize, uint32 InBlockSize);

		// Allocate an available sub heap block from the global heap
		OnlineDescriptorBlock* AllocateHeapBlock();
		void FreeHeapBlock(OnlineDescriptorBlock* InHeapBlock);

		ID3D12DescriptorHeap* GetHeap() { return Heap->GetHeap(); }
		DescriptorHeap* GetDescriptorHeap() { return Heap.Get(); }

		D3D12_CPU_DESCRIPTOR_HANDLE GetCPUSlotHandle(OnlineDescriptorBlock* InBlock) const { return Heap->GetCPUSlotHandle(InBlock->BaseSlot); }
		D3D12_GPU_DESCRIPTOR_HANDLE GetGPUSlotHandle(OnlineDescriptorBlock* InBlock) const { return Heap->GetGPUSlotHandle(InBlock->BaseSlot); }

		void Recycle(OnlineDescriptorBlock* Block);

	private:
		COMPtr<DescriptorHeap> Heap;

		std::queue<OnlineDescriptorBlock*> FreeBlocks;

		std::mutex Mutex;
	};

	struct OfflineHeapFreeRange
	{
		size_t Start;
		size_t End;
	};

	struct OfflineHeapEntry
	{
		OfflineHeapEntry(DescriptorHeap* InHeap, const D3D12_CPU_DESCRIPTOR_HANDLE& InHeapBase, size_t InSize)
			: Heap(InHeap)
		{
			FreeList.emplace_back(InHeapBase.ptr, InHeapBase.ptr + InSize );
		}

		COMPtr<DescriptorHeap> Heap;
		std::list<OfflineHeapFreeRange> FreeList;
	};

	struct OfflineDescriptor : public D3D12_CPU_DESCRIPTOR_HANDLE
	{
	public:
		operator bool() const { return ptr != 0; }
		OfflineDescriptor() { ptr = 0; }

		// Descriptor version can be used to invalidate any caches that are based on D3D12_CPU_DESCRIPTOR_HANDLE,
		// for example to detect when a view has been updated following a resource rename.
		void IncrementVersion() { ++Version; }
		uint32 GetVersion() const { return Version; }

	private:
		friend class OfflineDescriptorManager;
		uint32 HeapIndex = 0;
		uint32 Version = 0;
	};

	/** Manages and allows allocations of CPU descriptors only. Creates small heaps on demand to satisfy allocations. */
	class OfflineDescriptorManager : public DeviceChild
	{
	public:
		OfflineDescriptorManager() = delete;
		OfflineDescriptorManager(NodeDevice* InDevice, DescriptorHeapType InHeapType);
		~OfflineDescriptorManager();

		inline DescriptorHeapType GetHeapType() const { return HeapType; }

		OfflineDescriptor AllocateHeapSlot();
		void FreeHeapSlot(OfflineDescriptor& Descriptor);

	private:
		void AllocateHeap();

		std::vector<OfflineHeapEntry> Heaps;
		std::list<uint32> FreeHeaps;

		DescriptorHeapType HeapType;
		uint32 NumDescriptorsPerHeap{};
		uint32 DescriptorSize{};

		std::mutex Mutex;
	};

	/** Primary descriptor heap and descriptor manager. All heap allocations come from here.
		All GPU visible resource heap allocations will be sub-allocated from a single heap in this manager. */
	class DescriptorHeapManager : public DeviceChild
	{
	public:
		DescriptorHeapManager(NodeDevice* InDevice);
		~DescriptorHeapManager();

		void Init(uint32 InNumGlobalResourceDescriptors, uint32 InNumGlobalSamplerDescriptors);
		void Destroy();

		DescriptorHeap* AllocateIndependentHeap(const char* InDebugName, DescriptorHeapType InHeapType, uint32 InNumDescriptors, D3D12_DESCRIPTOR_HEAP_FLAGS InHeapFlags);
		DescriptorHeap* AllocateHeap(const char* InDebugName, DescriptorHeapType InHeapType, uint32 InNumDescriptors, D3D12_DESCRIPTOR_HEAP_FLAGS InHeapFlags);
		void DeferredFreeHeap(DescriptorHeap* InHeap);
		void ImmediateFreeHeap(DescriptorHeap* InHeap);
	private:
		std::vector<DescriptorManager> GlobalHeaps;
	};


	void CopyDescriptor(NodeDevice* Device, DescriptorHeap* TargetHeap, DescriptorHandle Handle, D3D12_CPU_DESCRIPTOR_HANDLE SourceCpuHandle);
}