#include "Allocation.h"
#include "Context.h"
using namespace platform_ex::Windows::D3D12;

int32 FastConstantAllocatorPageSize = 64 * 1024;

ResourceLocation::ResourceLocation(NodeDevice* Parent)
	:DeviceChild(Parent),
	Type(Undefined),
	UnderlyingResource(nullptr),
	MappedBaseAddress(nullptr),
	GPUVirtualAddress(0),
	OffsetFromBaseOfResource(0),
	Size(0),
	Allocator(nullptr)
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

	// Reset members
	Type = Undefined;
	UnderlyingResource = nullptr;
	MappedBaseAddress = nullptr;
	GPUVirtualAddress = 0;
	Size = 0;
	OffsetFromBaseOfResource = 0;

	Allocator = nullptr;
}

void ResourceLocation::ClearResource()
{
	switch (Type)
	{
	case Undefined:
		break;
	case SubAllocation:
	{
		wassume(Allocator != nullptr);
		Allocator->Deallocate(*this);
		break;
	}
	case FastAllocation:
		break;
	default:
		break;
	}
}

void ResourceAllocator::Deallocate(ResourceLocation& ResourceLocation)
{
}


bool FastConstantPageAllocator::TryAllocate(uint32 SizeInBytes, uint32 Alignment, ResourceLocation& ResourceLocation)
{
	for (auto& allocator : Allocators)
	{
		if(allocator->TryAllocate(SizeInBytes,Alignment,ResourceLocation))
			return true;
	}

	Allocators.push_back(CreateNewAllocator(SizeInBytes));
	return Allocators.back()->TryAllocate(SizeInBytes, Alignment, ResourceLocation);
}

void FastConstantPageAllocator::CleanUpAllocations(uint64 InFrameLag)
{
	// Trim empty allocators if not used in last n frames
	auto Adapter = GetParentDevice()->GetParentAdapter();
	auto& FrameFence = Adapter->GetFrameFence();
	const uint64 CompletedFence = FrameFence.UpdateLastCompletedFence();

	for (int32 i = static_cast<int32>(Allocators.size() - 1); i >= 0; i--)
	{
		if ((Allocators[i]->GetLastUsedFrameFence() + InFrameLag <= CompletedFence))
		{
			delete(Allocators[i]);
			Allocators.erase(Allocators.begin()+i);
		}
	}
}

FastConstantPageAllocator::ConstantAllocator::~ConstantAllocator()
{
	delete BackingResource;
}

void FastConstantPageAllocator::ConstantAllocator::Deallocate(ResourceLocation& ResourceLocation)
{
	auto& FrameFence = GetParentDevice()->GetParentAdapter()->GetFrameFence();

	RetireFrameFence = FrameFence.GetCurrentFence();
}

FastConstantPageAllocator::ConstantAllocator* FastConstantPageAllocator::CreateNewAllocator(uint32 InMinSizeInBytes)
{
	uint32 AllocationSize = std::bit_ceil(InMinSizeInBytes);

	return new ConstantAllocator(GetParentDevice(), GetVisibilityMask(),AllocationSize);
}

bool FastConstantPageAllocator::ConstantAllocator::TryAllocate(uint32 SizeInBytes, uint32 Alignment, ResourceLocation& ResourceLocation)
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
	ResourceLocation.SetAllocator(this);
	ResourceLocation.SetSize(SizeInBytes);
	ResourceLocation.SetOffsetFromBaseOfResource(AlignedOffsetFromResourceBase);
	ResourceLocation.SetResource(BackingResource);
	ResourceLocation.SetGPUVirtualAddress(BackingResource->GetGPUVirtualAddress() + AlignedOffsetFromResourceBase);

	ResourceLocation.SetMappedBaseAddress((uint8*)BackingResource->GetResourceBaseAddress() + AlignedOffsetFromResourceBase);

	return true;
}

void FastConstantPageAllocator::ConstantAllocator::CreateBackingResource()
{
	auto Adapter = GetParentDevice()->GetParentAdapter();

	const D3D12_HEAP_PROPERTIES HeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD, GetGPUMask(), GetVisibilityMask());

	Adapter->CreateBuffer(HeapProps, GetGPUMask(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_GENERIC_READ, BlockSize, &BackingResource, "FastConstantPageAllocator::ConstantAllocator", D3D12_RESOURCE_FLAG_NONE);

	BackingResource->Map();
}

UploadHeapAllocator::UploadHeapAllocator(D3D12Adapter* InParent, NodeDevice* InParentDevice, const std::string& InName)
	:AdapterChild(InParent), DeviceChild(InParentDevice), MultiNodeGPUObject(InParentDevice->GetGPUMask(), AllGPU())
	,FastConstantAllocator(InParentDevice,GetVisibilityMask())
{
}
void* UploadHeapAllocator::AllocFastConstantAllocationPage(uint32 InSize, uint32 InAlignment, ResourceLocation& ResourceLocation)
{
	ResourceLocation.Clear();

	bool ret = FastConstantAllocator.TryAllocate(InSize, InAlignment, ResourceLocation);
	wassume(ret);

	return ResourceLocation.GetMappedBaseAddress();
}

FastConstantAllocator::FastConstantAllocator(NodeDevice* Parent, GPUMaskType InGpuMask)
	:DeviceChild(Parent),MultiNodeGPUObject(InGpuMask,InGpuMask)
	,UnderlyingResource(Parent),Offset(FastConstantAllocatorPageSize),PageSize(FastConstantAllocatorPageSize)
{
}

void UploadHeapAllocator::Destroy()
{
}

void UploadHeapAllocator::CleanUpAllocations(uint64 InFrameLag)
{
	FastConstantAllocator.CleanUpAllocations(InFrameLag);
}

void* platform_ex::Windows::D3D12::FastConstantAllocator::Allocate(uint32 Bytes, ResourceLocation& OutLocation)
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
