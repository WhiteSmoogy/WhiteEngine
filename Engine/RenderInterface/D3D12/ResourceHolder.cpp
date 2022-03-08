#include "ResourceHolder.h"
#include "NodeDevice.h"
namespace platform_ex::Windows {
	namespace D3D12 {
		ImplDeDtor(ResourceHolder)
		ImplDeDtor(HeapHolder)

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
			:curr_state(D3D12_RESOURCE_STATE_COMMON)
		{
		}

		ResourceHolder::ResourceHolder(const COMPtr<ID3D12Resource>& pResource, D3D12_RESOURCE_STATES in_state)
			: ResourceHolder(pResource,in_state,pResource->GetDesc())
		{
		}

		ResourceHolder::ResourceHolder(const COMPtr<ID3D12Resource>& pResource, D3D12_RESOURCE_STATES in_state, const D3D12_RESOURCE_DESC& InDesc, D3D12_HEAP_TYPE InHeapType)
			: curr_state(in_state), resource(pResource),desc(InDesc),heap_type(InHeapType)
		{
			bDepthStencil = (desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) != 0;
		}

		ResourceHolder::ResourceHolder(const COMPtr<ID3D12Resource>& pResource, D3D12_RESOURCE_STATES in_state, const D3D12_RESOURCE_DESC& InDesc, HeapHolder* InHeap, D3D12_HEAP_TYPE InHeapType)
			: curr_state(in_state), resource(pResource), desc(InDesc), heap_type(InHeapType),heap(InHeap)
		{
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
				wassume(GetSubDeAllocator() != nullptr);
				GetSubDeAllocator()->Deallocate(*this);
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
	}
}
