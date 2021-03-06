/*! \file Engine\Render\D3D12\Resource.h
\ingroup Engine
\brief ??Դ??????
*/

#ifndef WE_RENDER_D3D12_Resource_h
#define WE_RENDER_D3D12_Resource_h 1

#include "Common.h"
#include "d3d12_dxgi.h"
#include <WBase/enum.hpp>

namespace platform_ex::Windows {
	namespace D3D12 {
		class NodeDevice;

		class ResourceLocation;

		class RefCountBase
		{
		private:
			unsigned long Uses = 1;
		public:
			virtual ~RefCountBase()
			{
			}

			unsigned long AddRef()
			{
				return _InterlockedIncrement(reinterpret_cast<volatile long*>(&Uses));
			}

			unsigned long Release()
			{
				uint32 NewValue = _InterlockedDecrement(reinterpret_cast<volatile long*>(&Uses));

				if (NewValue == 0)
					delete this;

				return NewValue;
			}

			uint32 GetRefCount() const
			{
				return Uses;
			}
		};

		enum class ResourceStateMode
		{
			Default,
			Single,
		};

		class HeapHolder :public RefCountBase, public DeviceChild, public MultiNodeGPUObject
		{
		public:
			HeapHolder(NodeDevice* InParentDevice, GPUMaskType VisibleNodes);
			~HeapHolder();


			ID3D12Heap* GetHeap() const{ return Heap.Get(); }
			void SetHeap(ID3D12Heap* HeapIn) { Heap.GetRef() = HeapIn; }
		private:
			COMPtr<ID3D12Heap> Heap;
		};

		class ResourceHolder :public RefCountBase {
		public:
			virtual ~ResourceHolder();

			bool UpdateResourceBarrier(D3D12_RESOURCE_BARRIER& barrier, D3D12_RESOURCE_STATES target_state);

			ID3D12Resource* Resource() const {
				return resource.Get();
			}

			D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const {
				return GPUVirtualAddress;
			}

			void* GetResourceBaseAddress() const { wconstraint(ResourceBaseAddress); return ResourceBaseAddress; }

			void SetName(const char* name);

			D3D12_RESOURCE_DESC GetDesc() const {
				return desc;
			}

			bool IsDepthStencilResource() const { return bDepthStencil; }

			//TODO
			inline uint64 GetOffsetFromBaseOfResource() const { return 0; }

			bool IsTransitionNeeded(D3D12_RESOURCE_STATES target_state)
			{
				return target_state != curr_state;
			}

			D3D12_RESOURCE_STATES GetResourceState() const
			{
				return curr_state;
			}

			D3D12_HEAP_TYPE GetHeapType() const
			{
				return heap_type;
			}

			void SetResourceState(D3D12_RESOURCE_STATES state)
			{
				curr_state = state;
			}

			inline void* Map(const D3D12_RANGE* ReadRange = nullptr)
			{
				wconstraint(resource);
				wconstraint(ResourceBaseAddress == nullptr);
				CheckHResult(resource->Map(0, ReadRange, &ResourceBaseAddress));

				return ResourceBaseAddress;
			}

			bool IsPlacedResource() const { return heap != nullptr; }

		protected:
			ResourceHolder();

			ResourceHolder(const COMPtr<ID3D12Resource>& pResource, D3D12_RESOURCE_STATES in_state = D3D12_RESOURCE_STATE_COMMON);

			ResourceHolder(const COMPtr<ID3D12Resource>& pResource, D3D12_RESOURCE_STATES in_state, const D3D12_RESOURCE_DESC& InDesc, D3D12_HEAP_TYPE InHeapType = D3D12_HEAP_TYPE_DEFAULT);

			ResourceHolder(const COMPtr<ID3D12Resource>& pResource, D3D12_RESOURCE_STATES in_state, const D3D12_RESOURCE_DESC& InDesc, HeapHolder* InHeap, D3D12_HEAP_TYPE InHeapType);

			friend class Device;
			friend struct BaseShaderResource;
			friend class Texture;
		protected:
			D3D12_RESOURCE_STATES curr_state;

			D3D12_HEAP_TYPE heap_type;

			COMPtr<ID3D12Resource> resource;
			COMPtr<HeapHolder> heap;

			D3D12_RESOURCE_DESC desc;

			bool bDepthStencil;

			void* ResourceBaseAddress;
			D3D12_GPU_VIRTUAL_ADDRESS GPUVirtualAddress;
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

		struct AllocatorConfig
		{
			D3D12_HEAP_TYPE HeapType = D3D12_HEAP_TYPE_UPLOAD;
			D3D12_HEAP_FLAGS HeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
			D3D12_RESOURCE_FLAGS ResourceFlags = D3D12_RESOURCE_FLAG_NONE;
			D3D12_RESOURCE_STATES InitialResourceState = D3D12_RESOURCE_STATE_GENERIC_READ;

			bool operator==(const AllocatorConfig&) const = default;
		};


		struct BuddyAllocatorPrivateData
		{
			uint32 Offset;
			uint32 Order;
		};

		class PoolAllocatorPrivateData
		{
		public:
			int16 GetPoolIndex()const { return PoolIndex; }

			uint32 GetOffset() const { return Offset; }

			void SetOwner(ResourceLocation* InOwner) { Owner = InOwner; }

			uint32 GetSize() const { return Size; }

			void InitAsAllocated(uint32 InSize, uint32 InAlignment, PoolAllocatorPrivateData* FreeBlock);
			void InitAsHead(int16 InPoolIndex);
			void InitAsFree(int16 InPoolIndex, uint32 InSize, uint32 InAlignment, uint32 InOffset);
			void MarkFree(uint32 Alignment);
			
			void MoveFrom(PoolAllocatorPrivateData& InAllocated, bool InLocked)
			{
				wconstraint(InAllocated.IsAllocated());

				Reset();

				Size = InAllocated.Size;
				Alignment = InAllocated.Alignment;
				Type = InAllocated.Type;
				Offset = InAllocated.Offset;
				PoolIndex = InAllocated.PoolIndex;
				Locked = InLocked;

				// Update linked list
				InAllocated.PreviousAllocation->NextAllocation = this;
				InAllocated.NextAllocation->PreviousAllocation = this;
				PreviousAllocation = InAllocated.PreviousAllocation;
				NextAllocation = InAllocated.NextAllocation;

				InAllocated.Reset();
			}

			void Reset()
			{
				Size = 0;
				Alignment = 0;
				SetAllocationType(EAllocationType::Unknown);
				Offset = 0;
				PoolIndex = -1;
				Locked = false;

				Owner = nullptr;
				PreviousAllocation = NextAllocation = nullptr;
			}

			bool IsAllocated() const {return Type == white::underlying(EAllocationType::Allocated);}

			bool IsFree() const{ return Type == white::underlying(EAllocationType::Free); }

			bool IsLocked() const { return (Locked == 1); }

			void UnLock() { wconstraint(IsLocked()); Locked = 0; }

			static uint32 GetAlignedSize(uint32 InSizeInBytes, uint32 InPoolAlignment, uint32 InAllocationAlignment)
			{
				wconstraint(InAllocationAlignment <= InPoolAlignment);

				// Compute the size, taking the pool and allocation alignmend into account (conservative size)
				uint32 Size = ((InPoolAlignment % InAllocationAlignment) != 0) ? InSizeInBytes + InAllocationAlignment : InSizeInBytes;
				return AlignArbitrary(Size, InPoolAlignment);
			}

			static uint32 GetAlignedOffset(uint32 InOffset, uint32 InPoolAlignment, uint32 InAllocationAlignment)
			{
				uint32 AlignedOffset = InOffset;

				// fix the offset with the requested alignment if needed
				if ((InPoolAlignment % InAllocationAlignment) != 0)
				{
					uint32 AlignmentRest = AlignedOffset % InAllocationAlignment;
					if (AlignmentRest > 0)
					{
						AlignedOffset += (InAllocationAlignment - AlignmentRest);
					}
				}

				return AlignedOffset;
			}

			PoolAllocatorPrivateData* GetPrev() const{ return PreviousAllocation; }
			PoolAllocatorPrivateData* GetNext() const { return NextAllocation; }

			void Merge(PoolAllocatorPrivateData* InOther)
			{
				wconstraint(IsFree() && InOther->IsFree());
				wconstraint((Offset + Size) == InOther->Offset);
				wconstraint(PoolIndex == InOther->GetPoolIndex());

				Size += InOther->Size;

				InOther->RemoveFromLinkedList();
				InOther->Reset();
			}

			void RemoveFromLinkedList()
			{
				wconstraint(IsFree());
				PreviousAllocation->NextAllocation = NextAllocation;
				NextAllocation->PreviousAllocation = PreviousAllocation;
			}

			void AddBefore(PoolAllocatorPrivateData* InOther);

			void AddAfter(PoolAllocatorPrivateData* InOther)
			{
				NextAllocation->PreviousAllocation = InOther;
				InOther->NextAllocation = NextAllocation;

				NextAllocation = InOther;
				InOther->PreviousAllocation = this;
			}
		private:
			enum class EAllocationType : uint8
			{
				Unknown,
				Free,
				Allocated,
				Head,
			};

			void SetAllocationType(EAllocationType type) { Type = white::underlying(type); }
			EAllocationType GetAllocationType() const { return static_cast<EAllocationType>(Type); }

			uint32 Size;
			uint32 Alignment;
			uint32 Offset;


			uint32 PoolIndex:16;
			uint32 Type : 4;
			uint32 Locked:1;
			uint32 Unused : 11;
			PoolAllocatorPrivateData* PreviousAllocation;
			PoolAllocatorPrivateData* NextAllocation;

			ResourceLocation* Owner;
		};

		struct ISubDeAllocator
		{
			virtual void Deallocate(ResourceLocation& ResourceLocation) = 0;
		};

		struct IPoolAllocator
		{
			virtual bool  SupportsAllocation(D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_FLAGS InResourceFlags, uint32 InBufferAccess, ResourceStateMode InResourceStateMode) const = 0;

			virtual void AllocDefaultResource(D3D12_HEAP_TYPE InHeapType, const D3D12_RESOURCE_DESC& InDesc, uint32 InBufferAccess, ResourceStateMode InResourceStateMode,
				D3D12_RESOURCE_STATES InCreateState, uint32 InAlignment, ResourceLocation& ResourceLocation, const char* InName) = 0;

			static AllocatorConfig GetResourceAllocatorInitConfig(D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_FLAGS InResourceFlags, uint32 InBufferAccess);

			static AllocationStrategy GetResourceAllocationStrategy(D3D12_RESOURCE_FLAGS InResourceFlags, ResourceStateMode InResourceStateMode)
			{
				// Does the resource need state tracking and transitions
				auto ResourceStateMode = white::underlying(InResourceStateMode);
				if (ResourceStateMode == white::underlying(ResourceStateMode::Default))
				{
					ResourceStateMode = (InResourceFlags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) ? 0xFF : 0xF;
				}

				// multi state resource need to placed because each allocation can be in a different state
				return (ResourceStateMode == 0xFF) ? AllocationStrategy::kPlacedResource : AllocationStrategy::kManualSubAllocation;
			}

			virtual void Deallocate(ResourceLocation& ResourceLocation) = 0;
			
			virtual void TransferOwnership(ResourceLocation& Destination, ResourceLocation& Source) = 0;

			virtual void CleanUpAllocations(uint64 InFrameLag) = 0;
		};

		class ResourceLocation :public DeviceChild, public white::noncopyable
		{
		public:
			enum LocationType
			{
				Undefined,
				SubAllocation,
				FastAllocation,
				StandAlone,
			};

			ResourceLocation(NodeDevice* Parent);
			~ResourceLocation();

			void SetResource(ResourceHolder* Value);
			inline void SetType(LocationType Value) { Type = Value; }
			inline void SetMappedBaseAddress(void* Value) { MappedBaseAddress = Value; }
			inline void SetGPUVirtualAddress(D3D12_GPU_VIRTUAL_ADDRESS Value) { GPUVirtualAddress = Value; }
			inline void SetOffsetFromBaseOfResource(uint64 Value) { OffsetFromBaseOfResource = Value; }
			inline void SetSize(uint64 Value) { Size = Value; }
			inline void SetSubDeAllocator(ISubDeAllocator* Value) { SetAllocatorPtr(Value); }
			inline void SetPoolAllocator(IPoolAllocator* Value) { SetAllocatorPtr(Value); }
			inline void ClearAllocator() { DeAllocator = nullptr; }


			inline ResourceHolder* GetResource() const { return UnderlyingResource; }
			inline void* GetMappedBaseAddress() const { return MappedBaseAddress; }
			inline D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const { return GPUVirtualAddress; }
			inline uint64 GetOffsetFromBaseOfResource() const { return OffsetFromBaseOfResource; }
			inline uint64 GetSize() const { return Size; }
			inline ISubDeAllocator* GetSubDeAllocator() const { return GetAllocatorPtr<ISubDeAllocator>(); ; }
			inline IPoolAllocator* GetPoolAllocator() const { return GetAllocatorPtr<IPoolAllocator>(); }

			inline BuddyAllocatorPrivateData& GetBuddyAllocatorPrivateData() { return AllocatorData.BuddyPrivateData; }
			inline PoolAllocatorPrivateData& GetPoolAllocatorPrivateData() { return AllocatorData.PoolPrivateData; }


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

			void AsStandAlone(ResourceHolder* Resource, uint64 InSize, bool bInIsTransient = false)
			{
				SetType(StandAlone);
				SetResource(Resource);
				SetSize(InSize);

				if (IsCPUAccessible(Resource->GetHeapType()))
				{
					D3D12_RANGE range = { 0, IsCPUWritable(Resource->GetHeapType()) ? 0 : InSize };
					SetMappedBaseAddress(Resource->Map(&range));
				}
				SetGPUVirtualAddress(Resource->GetGPUVirtualAddress());
				//SetTransient(bInIsTransient);
			}


			void Clear();

			static void TransferOwnership(ResourceLocation& Destination, ResourceLocation& Source);

			const inline bool IsValid() const {
				return Type != Undefined;
			}

			void UnlockPoolData();

		private:
			void ClearResource();
			void ClearMembers();

			LocationType Type;

			ResourceHolder* UnderlyingResource;

			enum {
				AT_SubDe = 0b1,
				AT_Pool = 0b11,
				At_Unknown = 0,
			};
			union
			{
				ISubDeAllocator* DeAllocator;
				IPoolAllocator* PollAllocator;
			};

			template<typename _type>
			_type* GetAllocatorPtr() const
			{
				if constexpr (std::is_same_v<_type, ISubDeAllocator>)
				{
					wconstraint((reinterpret_cast<uintptr_t>(DeAllocator) & 0x3) == AT_SubDe);
				}
				else if constexpr (std::is_same_v<_type, IPoolAllocator>)
				{
					wconstraint((reinterpret_cast<uintptr_t>(PollAllocator) & 0x3) == AT_Pool);
				}

				return reinterpret_cast<_type*>(reinterpret_cast<uintptr_t>(DeAllocator) & ~((uintptr_t)0x3));
			}

			template<typename _type>
			void SetAllocatorPtr(_type* Value)
			{
				wconstraint((reinterpret_cast<uintptr_t>(Value) & 0x3) == At_Unknown);

				if constexpr (std::is_same_v<_type, ISubDeAllocator>)
				{
					DeAllocator = reinterpret_cast<_type*>(reinterpret_cast<uintptr_t>(Value) | AT_SubDe);
				}
				else if constexpr (std::is_same_v<_type, IPoolAllocator>)
				{
					PollAllocator = reinterpret_cast<_type*>(reinterpret_cast<uintptr_t>(Value) | AT_Pool);
				}
			}

			int GetAllocatorType() const
			{
				return static_cast<int>(reinterpret_cast<uintptr_t>(DeAllocator) & 0x3);
			}

			union PrivateAllocatorData
			{
				BuddyAllocatorPrivateData BuddyPrivateData;
				PoolAllocatorPrivateData  PoolPrivateData;
			} AllocatorData;

			void* MappedBaseAddress;
			D3D12_GPU_VIRTUAL_ADDRESS GPUVirtualAddress;
			uint64 OffsetFromBaseOfResource;

			// The size the application asked for
			uint64 Size;
		};

		//track View to ResourceLocation
		struct BaseShaderResource
		{
			BaseShaderResource(NodeDevice* Parent)
				:Location(Parent)
			{}

			ResourceHolder* Resource() const { return Location.GetResource(); }

			ID3D12Resource* D3DResource() const {
				return Resource()->Resource();
			}

			void SetName(const char* name)
			{
				Resource()->SetName(name);
			}

			D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const
			{
				return Location.GetGPUVirtualAddress();
			}

			const inline bool IsValid() const {
				return Location.IsValid();
			}

			ResourceLocation Location;
		};

		class FastClearResource
		{
		public:
			inline void GetWriteMaskProperties(void*& OutData, uint32& OutSize)
			{
				OutData = nullptr;
				OutSize = 0;
			}
		};

		class ResourceBarrierBatcher
		{
		public:
			explicit ResourceBarrierBatcher()
			{};

			// Add a UAV barrier to the batch. Ignoring the actual resource for now.
			void AddUAV()
			{
				Barriers.resize(Barriers.size() + 1);
				D3D12_RESOURCE_BARRIER& Barrier = Barriers.back();
				Barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
				Barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
				Barrier.UAV.pResource = nullptr;	// Ignore the resource ptr for now. HW doesn't do anything with it.
			}

			// Add a transition resource barrier to the batch. Returns the number of barriers added, which may be negative if an existing barrier was cancelled.
			int32 AddTransition(ID3D12Resource* pResource, D3D12_RESOURCE_STATES Before, D3D12_RESOURCE_STATES After, uint32 Subresource)
			{
				wconstraint(Before != After);

				if (!Barriers.empty())
				{
					// Check if we are simply reverting the last transition. In that case, we can just remove both transitions.
					// This happens fairly frequently due to resource pooling since different RHI buffers can point to the same underlying D3D buffer.
					// Instead of ping-ponging that underlying resource between COPY_DEST and GENERIC_READ, several copies can happen without a ResourceBarrier() in between.
					// Doing this check also eliminates a D3D debug layer warning about multiple transitions of the same subresource.
					const D3D12_RESOURCE_BARRIER& Last = Barriers.back();
					if (pResource == Last.Transition.pResource &&
						Subresource == Last.Transition.Subresource &&
						Before == Last.Transition.StateAfter &&
						After == Last.Transition.StateBefore &&
						Last.Type == D3D12_RESOURCE_BARRIER_TYPE_TRANSITION)
					{
						Barriers.pop_back();
						return -1;
					}
				}

				Barriers.resize(Barriers.size() + 1);
				D3D12_RESOURCE_BARRIER& Barrier = Barriers.back();
				Barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				Barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
				Barrier.Transition.StateBefore = Before;
				Barrier.Transition.StateAfter = After;
				Barrier.Transition.Subresource = Subresource;
				Barrier.Transition.pResource = pResource;
				return 1;
			}

			void AddAliasingBarrier(ID3D12Resource* pResource)
			{
				Barriers.resize(Barriers.size() + 1);
				D3D12_RESOURCE_BARRIER& Barrier = Barriers.back();
				Barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
				Barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
				Barrier.Aliasing.pResourceBefore = NULL;
				Barrier.Aliasing.pResourceAfter = pResource;
			}

			// Flush the batch to the specified command list then reset.
			void Flush(ID3D12GraphicsCommandList* pCommandList)
			{
				if (!Barriers.empty())
				{
					wconstraint(pCommandList);
					{
						pCommandList->ResourceBarrier(static_cast<UINT>(Barriers.size()), Barriers.data());
					}
					Reset();
				}
			}

			// Clears the batch.
			void Reset()
			{
				Barriers.resize(0);	// Reset the array without shrinking (Does not destruct items, does not de-allocate memory).
			}

			const std::vector<D3D12_RESOURCE_BARRIER>& GetBarriers() const
			{
				return Barriers;
			}

		private:
			std::vector<D3D12_RESOURCE_BARRIER> Barriers;
		};

	}
}

#endif