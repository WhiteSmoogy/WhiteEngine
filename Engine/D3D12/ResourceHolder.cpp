#include "ResourceHolder.h"
#include "NodeDevice.h"
#include "spdlog/spdlog.h"
namespace platform_ex::Windows {
	namespace D3D12 {
		ImplDeDtor(ResourceHolder)
		ImplDeDtor(HeapHolder)

		void PoolAllocatorPrivateData::InitAsAllocated(uint32 InSize, uint32 InAlignment, PoolAllocatorPrivateData* InFree)
		{
			wconstraint(InFree->IsFree());

			Reset();

			Size = InSize;
			Alignment = InAlignment;
			SetAllocationType(EAllocationType::Allocated);
			Offset = GetAlignedOffset(InFree->Offset, InFree->Alignment, InAlignment);
			PoolIndex = InFree->PoolIndex;
			Locked = 1;

			uint32 AlignedSize = AlignArbitrary(InSize, InFree->Alignment);
			InFree->Size -= AlignedSize;
			InFree->Offset += AlignedSize;
			InFree->AddBefore(this);
		}

		void PoolAllocatorPrivateData::InitAsHead(int16 InPoolIndex)
		{
			Reset();

			SetAllocationType(EAllocationType::Head);
			NextAllocation = this;
			PreviousAllocation = this;
			PoolIndex = InPoolIndex;
		}

		void PoolAllocatorPrivateData::InitAsFree(int16 InPoolIndex, uint32 InSize, uint32 InAlignment, uint32 InOffset)
		{
			Reset();

			Size = InSize;
			Alignment = InAlignment;
			SetAllocationType(EAllocationType::Free);
			Offset = InOffset;
			PoolIndex = InPoolIndex;
		}

		void PoolAllocatorPrivateData::MarkFree(uint32 InAlignment)
		{
			wconstraint(IsAllocated());

			// Mark as free and update the size, offset and alignment according to the new requested alignment
			SetAllocationType(EAllocationType::Free);
			Size = Align(Size, InAlignment);
			Offset = AlignDown(Offset, InAlignment);
			Alignment = InAlignment;
		}

		void PoolAllocatorPrivateData::AddBefore(PoolAllocatorPrivateData* InOther)
		{
			PreviousAllocation->NextAllocation = InOther;
			InOther->PreviousAllocation = PreviousAllocation;

			PreviousAllocation = InOther;
			InOther->NextAllocation = this;
		}

		void Windows::D3D12::ResourceHolder::SetName(const char* name)
		{
			D3D::Debug(resource, name);
		}

		ResourceHolder::ResourceHolder(const COMPtr<ID3D12Resource>& pResource,
			D3D12_RESOURCE_STATES InInitialResourceState,
			const D3D12_RESOURCE_DESC& InDesc,
			HeapHolder* InHeap, D3D12_HEAP_TYPE InHeapType)
			:ResourceHolder(pResource, 
				InInitialResourceState, ResourceStateMode::Default, D3D12_RESOURCE_STATE_TBD, 
				InDesc,
				InHeap, InHeapType)
		{
		}

		ResourceHolder::ResourceHolder(const COMPtr<ID3D12Resource>& pResource,
			D3D12_RESOURCE_STATES InInitialState, ResourceStateMode InResourceStateMode, D3D12_RESOURCE_STATES InDefaultResourceState,
			const D3D12_RESOURCE_DESC& InDesc,
			HeapHolder* InHeap, D3D12_HEAP_TYPE InHeapType)
			: resource(pResource), heap(InHeap), heap_type(InHeapType), desc(InDesc), bRequiresResourceStateTracking(true)
		{
			bDepthStencil = (desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) != 0;

			if (resource && desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
			{
				GPUVirtualAddress = pResource->GetGPUVirtualAddress();
			}

			InitalizeResourceState(InInitialState, InResourceStateMode, InDefaultResourceState);
		}

		HeapHolder::HeapHolder(NodeDevice* InParentDevice, GPUMaskType VisibleNodes)
			:DeviceChild(InParentDevice),MultiNodeGPUObject(InParentDevice->GetGPUMask(),VisibleNodes)
		{
		}

		ResourceLocation::ResourceLocation(NodeDevice* Parent)
			: DeviceChild(Parent),
			Type(Undefined),
			UnderlyingResource(nullptr),
			MappedBaseAddress(nullptr),
			GPUVirtualAddress(0),
			OffsetFromBaseOfResource(0),
			Size(0),
			DeAllocator(nullptr)
		{
		}

		ResourceLocation::~ResourceLocation()
		{
			ClearResource();
		}

		void ResourceLocation::SetResource(ResourceHolder* Value)
		{
			UnderlyingResource = Value;

			GPUVirtualAddress = UnderlyingResource->GetGPUVirtualAddress();
		}

		void ResourceLocation::Clear()
		{
			ClearResource();

			ClearMembers();
		}

		void ResourceLocation::ClearMembers()
		{
			// Reset members
			Type = Undefined;
			UnderlyingResource = nullptr;
			MappedBaseAddress = nullptr;
			GPUVirtualAddress = 0;
			Size = 0;
			OffsetFromBaseOfResource = 0;

			DeAllocator = nullptr;
		}

		void ResourceLocation::ClearResource()
		{
			switch (Type)
			{
			case Undefined:
				break;
			case SubAllocation:
			{
				switch (GetAllocatorType())
				{
				case AT_SubDe:
					GetSubDeAllocator()->Deallocate(*this);
					break;
				case AT_Pool:
					GetPoolAllocator()->Deallocate(*this);
				}
				break;
			}
			case FastAllocation:
				break;
			case StandAlone:
				UnderlyingResource->Release();
				break;
			default:
				break;
			}
		}

		void ResourceLocation::TransferOwnership(ResourceLocation& Destination, ResourceLocation& Source)
		{
			Destination.Clear();

			std::memmove(&Destination, &Source, sizeof(ResourceLocation));

			//Transfer Allocator
			if (Source.GetAllocatorType() == AT_Pool)
			{
				Source.GetPoolAllocator()->TransferOwnership(Destination, Source);
			}

			Source.ClearMembers();
		}

		void ResourceLocation::UnlockPoolData()
		{
			if (GetAllocatorType() == AT_Pool)
			{
				GetPoolAllocatorPrivateData().UnLock();
			}
		}

		void CResourceState::Initialize(uint32 SubresourceCount)
		{
			// Allocate space for per-subresource tracking structures
			wassume(SubresourceCount > 0);
			m_SubresourceState.resize(SubresourceCount);

			// No internal transition yet
			bHasInternalTransition = 0;

			// All subresources start out in an unknown state
			SetResourceState(D3D12_RESOURCE_STATE_TBD);

			// Unknown hidden resource state
			SetUAVHiddenResourceState(D3D12_RESOURCE_STATE_TBD);
		}

		bool CResourceState::AreAllSubresourcesSame() const
		{
			return m_AllSubresourcesSame && (m_ResourceState != D3D12_RESOURCE_STATE_TBD);
		}

		bool CResourceState::CheckResourceState(D3D12_RESOURCE_STATES State) const
		{
			if (m_AllSubresourcesSame)
			{
				return State == m_ResourceState;
			}
			else
			{
				// All subresources must be individually checked
				const uint32 numSubresourceStates = static_cast<uint32>(m_SubresourceState.size());
				for (uint32 i = 0; i < numSubresourceStates; i++)
				{
					if (m_SubresourceState[i] != State)
					{
						return false;
					}
				}

				return true;
			}
		}

		bool CResourceState::CheckResourceStateInitalized() const
		{
			return m_SubresourceState.size() > 0;
		}

		D3D12_RESOURCE_STATES CResourceState::GetSubresourceState(uint32 SubresourceIndex) const
		{
			if (m_AllSubresourcesSame)
			{
				return static_cast<D3D12_RESOURCE_STATES>(m_ResourceState);
			}
			else
			{
				wassume(SubresourceIndex <m_SubresourceState.size());
				return m_SubresourceState[SubresourceIndex];
			}
		}

		bool CResourceState::CheckAllSubresourceSame()
		{
			// already marked same?
			if (m_AllSubresourcesSame)
			{
				return true;
			}
			else
			{
				D3D12_RESOURCE_STATES State = m_SubresourceState[0];

				// All subresources must be individually checked
				const uint32 numSubresourceStates = static_cast<uint32>(m_SubresourceState.size());
				for (uint32 i = 1; i < numSubresourceStates; i++)
				{
					if (m_SubresourceState[i] != State)
					{
						return false;
					}
				}

				SetResourceState(State);

				return true;
			}
		}

		void CResourceState::SetResourceState(D3D12_RESOURCE_STATES State)
		{
			m_AllSubresourcesSame = 1;

			// m_ResourceState is restricted to 31 bits.  Ensure State can be properly represented.
			wassume((State & (1 << 31)) == 0);

			m_ResourceState = *reinterpret_cast<uint32*>(&State);

			// State is now tracked per-resource, so m_SubresourceState should not be read.
#ifndef NDEBUG
			const uint32 numSubresourceStates = static_cast<uint32>(m_SubresourceState.size());
			for (uint32 i = 0; i < numSubresourceStates; i++)
			{
				m_SubresourceState[i] = D3D12_RESOURCE_STATE_CORRUPT;
			}
#endif
		}

		void CResourceState::SetSubresourceState(uint32 SubresourceIndex, D3D12_RESOURCE_STATES State)
		{
			// If setting all subresources, or the resource only has a single subresource, set the per-resource state
			if (SubresourceIndex == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES ||
				m_SubresourceState.size() == 1)
			{
				SetResourceState(State);
			}
			else
			{
				wassume(SubresourceIndex < static_cast<uint32>(m_SubresourceState.size()));

				// If state was previously tracked on a per-resource level, then transition to per-subresource tracking
				if (m_AllSubresourcesSame)
				{
					const uint32 numSubresourceStates = static_cast<uint32>(m_SubresourceState.size());
					for (uint32 i = 0; i < numSubresourceStates; i++)
					{
						m_SubresourceState[i] = static_cast<D3D12_RESOURCE_STATES>(m_ResourceState);
					}

					m_AllSubresourcesSame = 0;

					// State is now tracked per-subresource, so m_ResourceState should not be read.
#ifndef NDEBUG
					m_ResourceState = D3D12_RESOURCE_STATE_CORRUPT;
#endif
				}

				m_SubresourceState[SubresourceIndex] = State;
			}
		}
	
		int32 ResourceBarrierBatcher::AddTransition(ID3D12Resource* pResource, D3D12_RESOURCE_STATES Before, D3D12_RESOURCE_STATES After, uint32 Subresource)
		{
			wconstraint(Before != After);

			if (!Barriers.empty())
			{
				//Revert A->B->A
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

				//Combine A->B->C => A->C
				if (pResource == Last.Transition.pResource &&
					Subresource == Last.Transition.Subresource &&
					Before == Last.Transition.StateAfter && Last.Type == D3D12_RESOURCE_BARRIER_TYPE_TRANSITION)
				{
					Before = Last.Transition.StateBefore;
					Barriers.pop_back();
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

}
}
