/*! \file Engine\Render\D3D12\Resource.h
\ingroup Engine
\brief ×ÊÔ´ÃèÊö¡£
*/

#ifndef WE_RENDER_D3D12_Resource_h
#define WE_RENDER_D3D12_Resource_h 1

#include "Common.h"
#include "Utility.h"
#include "RenderInterface/IFormat.hpp"
#include <WBase/enum.hpp>
#include <WBase/smart_ptr.hpp>

namespace platform_ex::Windows {
	namespace D3D12 {
		using ::platform::Render::EAccessHint;

		class NodeDevice;

		class ResourceLocation;

	
		enum class ResourceStateMode
		{
			Default,
			Single,
			Multi,
		};

		constexpr D3D12_RESOURCE_STATES BackBufferBarrierWriteTransitionTargets = D3D12_RESOURCE_STATES(
			uint32(D3D12_RESOURCE_STATE_RENDER_TARGET) |
			uint32(D3D12_RESOURCE_STATE_UNORDERED_ACCESS) |
			uint32(D3D12_RESOURCE_STATE_STREAM_OUT) |
			uint32(D3D12_RESOURCE_STATE_COPY_DEST) |
			uint32(D3D12_RESOURCE_STATE_RESOLVE_DEST));

		class CResourceState
		{
		public:
			void Initialize(uint32 SubresourceCount);

			bool AreAllSubresourcesSame() const;
			bool CheckResourceState(D3D12_RESOURCE_STATES State) const;
			bool CheckResourceStateInitalized() const;
			D3D12_RESOURCE_STATES GetSubresourceState(uint32 SubresourceIndex) const;
			bool CheckAllSubresourceSame();
			void SetResourceState(D3D12_RESOURCE_STATES State);
			void SetSubresourceState(uint32 SubresourceIndex, D3D12_RESOURCE_STATES State);

			D3D12_RESOURCE_STATES GetUAVHiddenResourceState() const
			{
				return static_cast<D3D12_RESOURCE_STATES>(UAVHiddenResourceState);
			}

			void SetUAVHiddenResourceState(D3D12_RESOURCE_STATES InUAVHiddenResourceState)
			{
				// The hidden state can never include UAV
				wassume(InUAVHiddenResourceState == D3D12_RESOURCE_STATE_TBD || !white::has_anyflags(InUAVHiddenResourceState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
				wassume((InUAVHiddenResourceState & (1 << 31)) == 0);

				UAVHiddenResourceState = (uint32)InUAVHiddenResourceState;
			}

			void SetHasInternalTransition()
			{
				bHasInternalTransition = 1;
			}
			bool HasInternalTransition() const
			{
				return bHasInternalTransition != 0;
			}

		private:
			// Only used if m_AllSubresourcesSame is 1.
			// Bits defining the state of the full resource, bits are from D3D12_RESOURCE_STATES
			uint32 m_ResourceState : 31;

			// Set to 1 if m_ResourceState is valid.  In this case, all subresources have the same state
			// Set to 0 if m_SubresourceState is valid.  In this case, each subresources may have a different state (or may be unknown)
			uint32 m_AllSubresourcesSame : 1;

			// Special resource state to track previous state before resource transitioned to UAV when the resource
			// has a UAV aliasing resource so correct previous state can be found (only single state allowed)
			uint32 UAVHiddenResourceState : 31;

			// Was the resource used for another transition than the pending transition
			uint32 bHasInternalTransition : 1;

			// Only used if m_AllSubresourcesSame is 0.
			// The state of each subresources.  Bits are from D3D12_RESOURCE_STATES.
			std::vector<D3D12_RESOURCE_STATES> m_SubresourceState;
		};

		class HeapHolder :public white::ref_count_base, public DeviceChild, public MultiNodeGPUObject
		{
		public:
			HeapHolder(NodeDevice* InParentDevice, GPUMaskType VisibleNodes);
			~HeapHolder();


			ID3D12Heap* GetHeap() const { return Heap.Get(); }
			void SetHeap(ID3D12Heap* HeapIn) { Heap.GetRef() = HeapIn; }
		private:
			COMPtr<ID3D12Heap> Heap;
		};

		inline D3D12_RESOURCE_STATES GetD3D12ResourceState(EAccessHint InRHIAccess, bool InIsAsyncCompute)
		{
			// Add switch for common states (should cover all writeable states)
			switch (InRHIAccess)
			{
				// all single write states
			case EAccessHint::EA_RTV:					return D3D12_RESOURCE_STATE_RENDER_TARGET;
			case EAccessHint::EA_GPUUnordered:			return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			case EAccessHint::EA_DSV:				return D3D12_RESOURCE_STATE_DEPTH_WRITE;
			case EAccessHint::EA_GPUWrite:				return D3D12_RESOURCE_STATE_COPY_DEST;
			case EAccessHint::EA_Present:				return D3D12_RESOURCE_STATE_PRESENT;

				// Generic read for mask read states
			case EAccessHint::EA_GPURead:
				return D3D12_RESOURCE_STATE_GENERIC_READ;
			default:
			{
				// Special case for DSV read & write (Depth write allows depth read as well in D3D)
				if (InRHIAccess == EAccessHint::EA_DSV)
				{
					return D3D12_RESOURCE_STATE_DEPTH_WRITE;
				}
				else
				{
					D3D12_RESOURCE_STATES State = D3D12_RESOURCE_STATE_COMMON;

					// Translate the requested after state to a D3D state
					if (white::has_anyflags(InRHIAccess, EAccessHint::EA_SRV) && !InIsAsyncCompute)
					{
						State |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
					}
					if (white::has_anyflags(InRHIAccess, EAccessHint::EA_Compute))
					{
						State |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
					}
					if (white::has_anyflags(InRHIAccess, EAccessHint::EA_VertexOrIndexBuffer))
					{
						State |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | D3D12_RESOURCE_STATE_INDEX_BUFFER;
					}
					if (white::has_anyflags(InRHIAccess, EAccessHint::EA_CopySrc))
					{
						State |= D3D12_RESOURCE_STATE_COPY_SOURCE;
					}
					if (white::has_anyflags(InRHIAccess, EAccessHint::EA_DrawIndirect))
					{
						State |= D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
					}
					if (white::has_anyflags(InRHIAccess, EAccessHint::EA_ResolveSrc))
					{
						State |= D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
					}
					if (white::has_anyflags(InRHIAccess, EAccessHint::EA_DSVRead))
					{
						State |= D3D12_RESOURCE_STATE_DEPTH_READ;
					}
					if (white::has_anyflags(InRHIAccess, EAccessHint::EA_ShadingRateSource))
					{
						State |= D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE;
					}

					// Should have at least one valid state
					wassume(State != D3D12_RESOURCE_STATE_COMMON);

					return State;
				}
			}
			}

			// unreachable code
			return D3D12_RESOURCE_STATE_COMMON;
		}

		class ResourceHolder :public white::ref_count_base {
		public:
			virtual ~ResourceHolder();

			ID3D12Resource* Resource() const {
				return resource.Get();
			}

			ID3D12Resource* GetUAVAccessResource() const { return nullptr; }

			D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const {
				return GPUVirtualAddress;
			}

			void* GetResourceBaseAddress() const { wconstraint(ResourceBaseAddress); return ResourceBaseAddress; }

			void SetName(const char* name);

			D3D12_RESOURCE_DESC GetDesc() const {
				return desc;
			}

			bool IsDepthStencilResource() const { return bDepthStencil; }

			bool IsBackBuffer() const { return bIsBackBuffer; }
			void SetIsBackBuffer(bool bBackBufferIn) { bIsBackBuffer = bBackBufferIn; }

			D3D12_HEAP_TYPE GetHeapType() const
			{
				return heap_type;
			}

			inline void* Map(const D3D12_RANGE* ReadRange = nullptr)
			{
				wconstraint(resource);
				wconstraint(ResourceBaseAddress == nullptr);
				CheckHResult(resource->Map(0, ReadRange, &ResourceBaseAddress));

				return ResourceBaseAddress;
			}

			bool IsPlacedResource() const { return heap != nullptr; }

			uint16 GetMipLevels() const { return desc.MipLevels; }
			uint16 GetArraySize() const { return (desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D) ? 1 : desc.DepthOrArraySize; }
			uint16 GetPlaneCount() const { return  platform_ex::Windows::D3D12::GetPlaneCount(desc.Format); }

			uint16 GetSubresourceCount() const {return SubresourceCount;}

			bool RequiresResourceStateTracking() const { return bRequiresResourceStateTracking; }

			CResourceState& GetResourceState_OnResource()
			{
				wassume(bRequiresResourceStateTracking);
				// This state is used as the resource's "global" state between command lists. It's only needed for resources that
				// require state tracking.
				return ResourceState;
			}
			D3D12_RESOURCE_STATES GetDefaultResourceState() const { wassume(!bRequiresResourceStateTracking); return DefaultResourceState; }
			D3D12_RESOURCE_STATES GetWritableState() const { return WritableState; }
			D3D12_RESOURCE_STATES GetReadableState() const { return ReadableState; }

			struct ResourceTypeHelper
			{
				ResourceTypeHelper(const D3D12_RESOURCE_DESC& Desc, D3D12_HEAP_TYPE HeapType) :
					bSRV(!white::has_anyflags(Desc.Flags, D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE)),
					bDSV(white::has_anyflags(Desc.Flags, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)),
					bRTV(white::has_anyflags(Desc.Flags, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)),
					bUAV(white::has_anyflags(Desc.Flags, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)),
					bWritable(bDSV || bRTV || bUAV),
					bSRVOnly(bSRV && !bWritable),
					bBuffer(Desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER),
					bReadBackResource(HeapType == D3D12_HEAP_TYPE_READBACK)
				{}

				const D3D12_RESOURCE_STATES GetOptimalInitialState(EAccessHint InResourceState, bool bAccurateWriteableStates) const
				{
					// Ignore the requested resource state for non tracked resource because RHI will assume it's always in default resource 
					// state then when a transition is required (will transition via scoped push/pop to requested state)
					if (!bSRVOnly && InResourceState != 0 && InResourceState != EAccessHint::EA_Discard)
					{
						bool bAsyncCompute = false;
						return GetD3D12ResourceState(InResourceState, bAsyncCompute);
					}
					else
					{
						if (bSRVOnly)
						{
							return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
						}
						else if (bBuffer && !bUAV)
						{
							return (bReadBackResource) ? D3D12_RESOURCE_STATE_COPY_DEST : D3D12_RESOURCE_STATE_GENERIC_READ;
						}
						else if (bWritable)
						{
							if (bAccurateWriteableStates)
							{
								if (bDSV)
								{
									return D3D12_RESOURCE_STATE_DEPTH_WRITE;
								}
								else if (bRTV)
								{
									return D3D12_RESOURCE_STATE_RENDER_TARGET;
								}
								else if (bUAV)
								{
									return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
								}
							}
							else
							{
								// This things require tracking anyway
								return D3D12_RESOURCE_STATE_COMMON;
							}
						}

						return D3D12_RESOURCE_STATE_COMMON;
					}
				}

				const uint32 bSRV : 1;
				const uint32 bDSV : 1;
				const uint32 bRTV : 1;
				const uint32 bUAV : 1;
				const uint32 bWritable : 1;
				const uint32 bSRVOnly : 1;
				const uint32 bBuffer : 1;
				const uint32 bReadBackResource : 1;
			};
		public:
			ResourceHolder() = delete;

			ResourceHolder(
				const COMPtr<ID3D12Resource>& pResource,
				D3D12_RESOURCE_STATES InInitialResourceState,
				const D3D12_RESOURCE_DESC& InDesc,
				HeapHolder* InHeap = nullptr, D3D12_HEAP_TYPE InHeapType = D3D12_HEAP_TYPE_DEFAULT);

			ResourceHolder(
				const COMPtr<ID3D12Resource>& pResource,
				D3D12_RESOURCE_STATES InInitialResourceState, ResourceStateMode InResourceStateMode, D3D12_RESOURCE_STATES InDefaultResourceState,
				const D3D12_RESOURCE_DESC& InDesc,
				HeapHolder* InHeap = nullptr, D3D12_HEAP_TYPE InHeapType = D3D12_HEAP_TYPE_DEFAULT);

			friend class Device;
			friend struct BaseShaderResource;
			friend class Texture;
		protected:
			COMPtr<ID3D12Resource> resource;
			white::ref_ptr<HeapHolder> heap;
			D3D12_HEAP_TYPE heap_type;

			D3D12_RESOURCE_DESC desc;

			void* ResourceBaseAddress = nullptr;
			D3D12_GPU_VIRTUAL_ADDRESS GPUVirtualAddress = 0;

			CResourceState ResourceState;
			D3D12_RESOURCE_STATES DefaultResourceState{ D3D12_RESOURCE_STATE_TBD };
			D3D12_RESOURCE_STATES ReadableState{ D3D12_RESOURCE_STATE_CORRUPT };
			D3D12_RESOURCE_STATES WritableState{ D3D12_RESOURCE_STATE_CORRUPT };

			bool bDepthStencil : 1;
			bool bRequiresResourceStateTracking : 1;
			bool bIsBackBuffer : 1;

			uint16 SubresourceCount = 0;
		private:
			void InitalizeResourceState(D3D12_RESOURCE_STATES InInitialState, ResourceStateMode InResourceStateMode, D3D12_RESOURCE_STATES InDefaultState)
			{
				SubresourceCount = GetMipLevels() * GetArraySize() * GetPlaneCount();

				if (InResourceStateMode == ResourceStateMode::Single)
				{
					// make sure a valid default state is set
					DefaultResourceState = InDefaultState;
					WritableState = D3D12_RESOURCE_STATE_CORRUPT;
					ReadableState = D3D12_RESOURCE_STATE_CORRUPT;
					bRequiresResourceStateTracking = false;
				}
				else
				{
					DetermineResourceStates(InDefaultState, InResourceStateMode);
				}

				if (bRequiresResourceStateTracking)
				{
					// No state tracking for acceleration structures because they can't have another state
					wassume(InDefaultState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE && InInitialState != D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);

					// Only a few resources (~1%) actually need resource state tracking
					ResourceState.Initialize(SubresourceCount);
					ResourceState.SetResourceState(InInitialState);
				}
			}

			void DetermineResourceStates(D3D12_RESOURCE_STATES InDefaultState, ResourceStateMode InResourceStateMode)
			{
				const ResourceTypeHelper Type(desc, heap_type);

				bDepthStencil = Type.bDSV;

				if (Type.bWritable || InResourceStateMode == ResourceStateMode::Multi)
				{
					// Determine the resource's write/read states.
					if (Type.bRTV)
					{
						// Note: The resource could also be used as a UAV however we don't store that writable state. UAV's are handled in a separate RHITransitionResources() specially for UAVs so we know the writeable state in that case should be UAV.
						wassume(!Type.bDSV && !Type.bBuffer);
						WritableState = D3D12_RESOURCE_STATE_RENDER_TARGET;
						ReadableState = Type.bSRV ? D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE : D3D12_RESOURCE_STATE_CORRUPT;
					}
					else if (Type.bDSV)
					{
						WritableState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
						ReadableState = Type.bSRV ? D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE : D3D12_RESOURCE_STATE_DEPTH_READ;
					}
					else
					{
						WritableState = Type.bUAV ? D3D12_RESOURCE_STATE_UNORDERED_ACCESS : D3D12_RESOURCE_STATE_CORRUPT;
						ReadableState = Type.bSRV ? D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE : D3D12_RESOURCE_STATE_CORRUPT;
					}
				}
				else
				{
					bRequiresResourceStateTracking = false;

					if (InDefaultState != D3D12_RESOURCE_STATE_TBD)
					{
						DefaultResourceState = InDefaultState;
					}
					else if (Type.bBuffer)
					{
						DefaultResourceState = (heap_type == D3D12_HEAP_TYPE_READBACK) ? D3D12_RESOURCE_STATE_COPY_DEST : D3D12_RESOURCE_STATE_GENERIC_READ;
					}
					else
					{
						DefaultResourceState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
					}
				}
			}
		};

		class CommandList;

		class PendingResourceBarrier
		{
		public:
			ResourceHolder* Resource;
			D3D12_RESOURCE_STATES State;
			uint32                SubResource;

			PendingResourceBarrier(ResourceHolder* Resource, D3D12_RESOURCE_STATES State, uint32 SubResource)
				: Resource(Resource)
				, State(State)
				, SubResource(SubResource)
			{}
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

			bool IsAllocated() const { return Type == white::underlying(EAllocationType::Allocated); }

			bool IsFree() const { return Type == white::underlying(EAllocationType::Free); }

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

			PoolAllocatorPrivateData* GetPrev() const { return PreviousAllocation; }
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


			uint32 PoolIndex : 16;
			uint32 Type : 4;
			uint32 Locked : 1;
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
				auto ResourceStateMode = InResourceStateMode;
				if (ResourceStateMode == ResourceStateMode::Default)
				{
					ResourceStateMode = (InResourceFlags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) ? ResourceStateMode::Multi : ResourceStateMode::Single;
				}

				// multi state resource need to placed because each allocation can be in a different state
				return (ResourceStateMode == ResourceStateMode::Multi) ? AllocationStrategy::kPlacedResource : AllocationStrategy::kManualSubAllocation;
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

			ResourceHolder* GetResource() const { return Location.GetResource(); }

			ID3D12Resource* D3DResource() const {
				return GetResource()->Resource();
			}

			void SetName(const char* name)
			{
				GetResource()->SetName(name);
			}

			D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const
			{
				return Location.GetGPUVirtualAddress();
			}

			inline uint64 GetOffsetFromBaseOfResource() const {
				return Location.GetOffsetFromBaseOfResource();
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

			int32 AddTransition(ResourceHolder* pResource, D3D12_RESOURCE_STATES Before, D3D12_RESOURCE_STATES After, uint32 Subresource)
			{
				return AddTransition(pResource->Resource(), Before, After, Subresource);
			}

			// Add a transition resource barrier to the batch. Returns the number of barriers added, which may be negative if an existing barrier was cancelled.
			int32 AddTransition(ID3D12Resource* pResource, D3D12_RESOURCE_STATES Before, D3D12_RESOURCE_STATES After, uint32 Subresource);

			void AddAliasingBarrier(ID3D12Resource* pResource)
			{
				Barriers.resize(Barriers.size() + 1);
				D3D12_RESOURCE_BARRIER& Barrier = Barriers.back();
				Barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
				Barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
				Barrier.Aliasing.pResourceBefore = NULL;
				Barrier.Aliasing.pResourceAfter = pResource;
			}

			void AddAliasingBarrier(ID3D12Resource* InResourceBefore, ID3D12Resource* pResource)
			{
				Barriers.resize(Barriers.size() + 1);
				D3D12_RESOURCE_BARRIER& Barrier = Barriers.back();
				Barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
				Barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
				Barrier.Aliasing.pResourceBefore = InResourceBefore;
				Barrier.Aliasing.pResourceAfter = pResource;
			}

			// Flush the batch to the specified command list then reset.
			void Flush(CommandList& pCommandList);

			// Clears the batch.
			void Reset()
			{
				Barriers.resize(0);	// Reset the array without shrinking (Does not destruct items, does not de-allocate memory).
			}

			const std::vector<D3D12_RESOURCE_BARRIER>& GetBarriers() const
			{
				return Barriers;
			}

			bool IsEmpty()
			{
				return Barriers.empty();
			}

		private:
			std::vector<D3D12_RESOURCE_BARRIER> Barriers;
		};

	}
}

#endif