#include "Allocation.h"
#include "Context.h"
#include <spdlog/spdlog.h>
using namespace platform_ex::Windows::D3D12;

int32 FastConstantAllocatorPageSize = 64 * 1024;
int32 UploadHeapSmallBlockMaxAllocationSize = 64 * 1024;
int32 UploadHeapSmallBlockPoolSize = 4 * 1024 * 1024;


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

void ResourceLocation::TransferOwnership(ResourceLocation& Destination, ResourceLocation& Source)
{
	Destination.Clear();

	std::memmove(&Destination, &Source, sizeof(ResourceLocation));

	//Transfer Allocator

	Source.ClearMembers();
}

void ResourceAllocator::Deallocate(ResourceLocation& ResourceLocation)
{
}


bool FastConstantPageAllocator::TryAllocate(uint32 SizeInBytes, uint32 Alignment, ResourceLocation& ResourceLocation)
{
	std::unique_lock Lock{ CS };
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
		if (!Allocators[i]->IsDeallocated())
			continue;
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

ResourceConfigAllocator::ResourceConfigAllocator(NodeDevice* InParentDevice, GPUMaskType VisibleNodes, const AllocatorConfig& InConfig, const std::string& Name, uint32 InMaxAllocationSize)
	:ResourceAllocator(InParentDevice,VisibleNodes),
	InitConfig(InConfig),
	DebugName(Name),
	Initialized(false),
	MaximumAllocationSizeForPooling(InMaxAllocationSize)
{
}

BuddyAllocator::BuddyAllocator(NodeDevice* InParentDevice, GPUMaskType VisibleNodes,
	const AllocatorConfig& InConfig, const std::string& Name, AllocationStrategy InStrategy, 
	uint32 InMaxAllocationSize, uint32 InMaxBlockSize, uint32 InMinBlockSize)
	:ResourceConfigAllocator(InParentDevice,VisibleNodes,InConfig,Name,InMaxAllocationSize)
	,MaxBlockSize(InMaxBlockSize)
	,MinBlockSize(InMinBlockSize)
	,Strategy(InStrategy)
	,BackingResource(nullptr)
	, LastUsedFrameFence(0)
	, FreeBlockArray(nullptr)
	, TotalSizeUsed(0)
{
	WAssert((MaxBlockSize / MinBlockSize) * MinBlockSize == MaxBlockSize," Evenly dividable");

	WAssert(0 == ((MaxBlockSize / MinBlockSize) & ((MaxBlockSize / MinBlockSize) - 1))," Power of two"); 


	MaxOrder = UnitSizeToOrder(SizeToUnitSize(MaxBlockSize));

	if (IsCPUAccessible(InitConfig.HeapType))
	{
		WAssert(InMaxAllocationSize < InMaxBlockSize,"allocator reserved memory");
		WAssert(sizeof(void*) * MaxOrder < MaxBlockSize, "allocator reserved memory too small");
	}
}

void BuddyAllocator::Initialize()
{
	auto Adapter = GetParentDevice()->GetParentAdapter();

	if (Strategy == AllocationStrategy::kPlacedResource) {
		//TODO
		wassume(false);
	}
	else {
		const D3D12_HEAP_PROPERTIES HeapProps = CD3DX12_HEAP_PROPERTIES(InitConfig.HeapType, GetGPUMask(), GetVisibilityMask());

		CheckHResult(Adapter->CreateBuffer(HeapProps, GetGPUMask(),
			InitConfig.InitialResourceState, InitConfig.InitialResourceState, MaxBlockSize, &BackingResource, "BuddyAllocator Underlying Buffer", InitConfig.ResourceFlags));

		if (IsCPUAccessible(InitConfig.HeapType)) {
			BackingResource->Map();

			void* pStart = BackingResource->GetResourceBaseAddress();
			FreeBlockArray = reinterpret_cast<FreeBlock**>(pStart);
			//‘§œ»«–∑÷Buddy
			for (uint32 i = 0; i < MaxOrder; ++i)
			{
				FreeBlockArray[i] = reinterpret_cast<FreeBlock*>((byte*)pStart + MinBlockSize*(1<<i));
				FreeBlockArray[i]->Next = nullptr;
			}

			TotalSizeUsed = MinBlockSize;
		}
	}
}

uint32 BuddyAllocator::AllocateBlock(uint32 order)
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
		FreeBlockArray[order] = pRight;

		offset = left;
	}
	else
	{
		byte* pStart =reinterpret_cast<byte*>(FreeBlockArray);
		byte* pEnd = reinterpret_cast<byte*>(FreeBlockArray[order]);

		offset = static_cast<uint32>(pEnd - pStart) / MinBlockSize;
		wconstraint(offset * MinBlockSize == pEnd - pStart);

		FreeBlockArray[order] = FreeBlockArray[order]->Next;
	}
	return offset;
}

void BuddyAllocator::Allocate(uint32 SizeInBytes, uint32 Alignment, ResourceLocation& ResourceLocation)
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
	ResourceLocation.SetAllocator(this);
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
}

bool BuddyAllocator::TryAllocate(uint32 SizeInBytes, uint32 Alignment, ResourceLocation& ResourceLocation)
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

void BuddyAllocator::Deallocate(ResourceLocation& ResourceLocation)
{
	WAssert(false, "TODO");
}

bool BuddyAllocator::CanAllocate(uint32 size, uint32 alignment)
{
	if (TotalSizeUsed == MaxBlockSize)
		return false;

	uint32 sizeToAllocate = size;
	// If the alignment doesn't match the block size
	if (alignment != 0 && MinBlockSize % alignment != 0)
	{
		sizeToAllocate = size + alignment;
	}

	uint32 blockSize = MaxBlockSize /2;

	for (int32 i = MaxOrder - 1; i >= 0; i--)
	{
		if (FreeBlockArray[i] != nullptr && blockSize >= sizeToAllocate)
			return true;

		blockSize /= 2;

		if (blockSize < sizeToAllocate) return false;
	}

	return false;
}

MultiBuddyAllocator::MultiBuddyAllocator(NodeDevice* InParentDevice, GPUMaskType VisibleNodes, const AllocatorConfig& InConfig, const std::string& Name, AllocationStrategy InStrategy, uint32 InMaxAllocationSize, uint32 InDefaultPoolSize, uint32 InMinBlockSize)
	:ResourceConfigAllocator(InParentDevice, VisibleNodes, InConfig, Name, InMaxAllocationSize)
	, DefaultPoolSize(InDefaultPoolSize)
	, MinBlockSize(InMinBlockSize)
	, Strategy(InStrategy)
{
	wconstraint(std::bit_ceil(MaximumAllocationSizeForPooling) < DefaultPoolSize);
}

bool platform_ex::Windows::D3D12::MultiBuddyAllocator::TryAllocate(uint32 SizeInBytes, uint32 Alignment, ResourceLocation& ResourceLocation)
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

BuddyAllocator* MultiBuddyAllocator::CreateNewAllocator(uint32 InMinSizeInBytes)
{
	wconstraint(InMinSizeInBytes <= MaximumAllocationSizeForPooling);
	uint32 AllocationSize = (InMinSizeInBytes > DefaultPoolSize) ?std::bit_ceil(InMinSizeInBytes) : DefaultPoolSize;

	return new BuddyAllocator(GetParentDevice(),
		GetVisibilityMask(),
		InitConfig,
		DebugName,
		Strategy,
		MaximumAllocationSizeForPooling,
		AllocationSize,
		MinBlockSize);
}

void MultiBuddyAllocator::Deallocate(ResourceLocation& ResourceLocation)
{
	WAssert(false, "The sub-allocators should handle the deallocation");
}

UploadHeapAllocator::UploadHeapAllocator(D3D12Adapter* InParent, NodeDevice* InParentDevice, const std::string& InName)
	:AdapterChild(InParent), DeviceChild(InParentDevice), MultiNodeGPUObject(InParentDevice->GetGPUMask(), AllGPU())
	,SmallBlockAllocator(InParentDevice, GetVisibilityMask(), {},InName,AllocationStrategy::kManualSubAllocation, 
		UploadHeapSmallBlockMaxAllocationSize, UploadHeapSmallBlockPoolSize,256)
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
		//TODO
	}

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
