#include "ResourceHolder.h"
#include "NodeDevice.h"
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

		bool ResourceHolder::UpdateResourceBarrier(D3D12_RESOURCE_BARRIER & barrier, D3D12_RESOURCE_STATES target_state)
		{
			if (curr_state == target_state)
				return false;
			else {
				barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

				barrier.Transition.pResource = resource.Get();

				barrier.Transition.StateBefore = curr_state;
				barrier.Transition.StateAfter = target_state;
				curr_state = target_state;
				return true;
			}
		}

		void Windows::D3D12::ResourceHolder::SetName(const char* name)
		{
			D3D::Debug(resource, name);
		}

		ResourceHolder::ResourceHolder()
			:curr_state(D3D12_RESOURCE_STATE_COMMON),GPUVirtualAddress(0)
		{
		}

		ResourceHolder::ResourceHolder(const COMPtr<ID3D12Resource>& pResource, D3D12_RESOURCE_STATES in_state)
			: ResourceHolder(pResource,in_state,pResource->GetDesc())
		{
		}

		ResourceHolder::ResourceHolder(const COMPtr<ID3D12Resource>& pResource, D3D12_RESOURCE_STATES in_state, const D3D12_RESOURCE_DESC& InDesc, D3D12_HEAP_TYPE InHeapType)
			:ResourceHolder(pResource,in_state, InDesc,nullptr,InHeapType)
		{
		}

		ResourceHolder::ResourceHolder(const COMPtr<ID3D12Resource>& pResource, D3D12_RESOURCE_STATES in_state, const D3D12_RESOURCE_DESC& InDesc, HeapHolder* InHeap, D3D12_HEAP_TYPE InHeapType)
			: curr_state(in_state), resource(pResource), desc(InDesc), heap_type(InHeapType)
		{
			bDepthStencil = (desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) != 0;

			if (resource && desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
			{
				GPUVirtualAddress = pResource->GetGPUVirtualAddress();
			}
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

			Source.ClearMembers();
		}

		void ResourceLocation::UnlockPoolData()
		{
			if (GetAllocatorType() == AT_Pool)
			{
				GetPoolAllocatorPrivateData().UnLock();
			}
		}
	}
}
