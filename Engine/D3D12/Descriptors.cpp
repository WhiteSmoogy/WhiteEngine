#include "Descriptors.h"
#include "NodeDevice.h"

using namespace platform_ex::Windows::D3D12;

void platform_ex::Windows::D3D12::CopyDescriptor(NodeDevice* Device, DescriptorHeap* TargetHeap, DescriptorHandle Handle, D3D12_CPU_DESCRIPTOR_HANDLE SourceCpuHandle)
{
	if (Handle.IsValid())
	{
		const D3D12_CPU_DESCRIPTOR_HANDLE DestCpuHandle = TargetHeap->GetCPUSlotHandle(Handle.GetIndex());
		const D3D12_DESCRIPTOR_HEAP_TYPE D3DHeapType = Convert(Handle.GetType());

		Device->GetDevice()->CopyDescriptorsSimple(1, DestCpuHandle, SourceCpuHandle, D3DHeapType);
	}
}


DescriptorHeap::DescriptorHeap(NodeDevice* InDevice, ID3D12DescriptorHeap* InHeap, uint32 InNumDescriptors, DescriptorHeapType InType, D3D12_DESCRIPTOR_HEAP_FLAGS InFlags, bool bInIsGlobal)
	:DeviceChild(InDevice),
	Heap(InHeap),
	CpuBase(InHeap->GetCPUDescriptorHandleForHeapStart()),
	GpuBase((InFlags& D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE) ? InHeap->GetGPUDescriptorHandleForHeapStart() : D3D12_GPU_DESCRIPTOR_HANDLE()),
	Offset(0),
	NumDescriptors(InNumDescriptors),
	DescriptorSize(InDevice->GetDevice()->GetDescriptorHandleIncrementSize(Convert(InType))),
	Type(InType),
	Flags(InFlags),
	bIsGlobal(bInIsGlobal),
	bIsSuballocation(false)
{
}

DescriptorHeap::DescriptorHeap(DescriptorHeap* SubAllocateSourceHeap, uint32 InOffset, uint32 InNumDescriptors)
	: DeviceChild(SubAllocateSourceHeap->GetParentDevice())
	, Heap(SubAllocateSourceHeap->Heap)
	, CpuBase(SubAllocateSourceHeap->CpuBase, InOffset, SubAllocateSourceHeap->DescriptorSize)
	, GpuBase(SubAllocateSourceHeap->GpuBase, InOffset, SubAllocateSourceHeap->DescriptorSize)
	, Offset(InOffset)
	, NumDescriptors(InNumDescriptors)
	, DescriptorSize(SubAllocateSourceHeap->DescriptorSize)
	, Type(SubAllocateSourceHeap->GetType())
	, Flags(SubAllocateSourceHeap->GetFlags())
	, bIsGlobal(SubAllocateSourceHeap->IsGlobal())
	, bIsSuballocation(true)
{
}

DescriptorHeap::~DescriptorHeap() = default;

DescriptorManager::DescriptorManager(NodeDevice* Device, DescriptorHeap* InHeap)
	:DeviceChild(Device),
	HeapDescriptorAllocator(InHeap->GetType(), InHeap->GetNumDescriptors()),
	Heap(InHeap)
{
}

DescriptorManager::~DescriptorManager() = default;

void DescriptorManager::UpdateImmediately(DescriptorHandle InHandle, D3D12_CPU_DESCRIPTOR_HANDLE InSourceCpuHandle)
{
	CopyDescriptor(GetParentDevice(), GetHeap(), InHandle, InSourceCpuHandle);
}

OnlineDescriptorManager::OnlineDescriptorManager(NodeDevice* Device)
	:DeviceChild(Device)
{}

OnlineDescriptorManager::~OnlineDescriptorManager() = default;

void OnlineDescriptorManager::Init(uint32 InTotalSize, uint32 InBlockSize)
{
	Heap = GetParentDevice()->GetDescriptorHeapManager().AllocateHeap(
		TEXT("Device Global - Online View Heap"),
		DescriptorHeapType::Standard,
		InTotalSize,
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

	// Compute amount of free blocks
	uint32 BlockCount = InTotalSize / InBlockSize;

	// Allocate the free blocks
	uint32 CurrentBaseSlot = 0;
	for (uint32 BlockIndex = 0; BlockIndex < BlockCount; ++BlockIndex)
	{
		// Last entry take the rest
		uint32 ActualBlockSize = (BlockIndex == (BlockCount - 1)) ? InTotalSize - CurrentBaseSlot : InBlockSize;
		FreeBlocks.emplace(new OnlineDescriptorBlock(CurrentBaseSlot, ActualBlockSize));
		CurrentBaseSlot += ActualBlockSize;
	}
}

// Allocate a new heap block
OnlineDescriptorBlock* OnlineDescriptorManager::AllocateHeapBlock()
{
	std::unique_lock Lock{Mutex};

	// Free block
	OnlineDescriptorBlock* Result = FreeBlocks.front();
	FreeBlocks.pop();

	return Result;
}

void OnlineDescriptorManager::FreeHeapBlock(OnlineDescriptorBlock* InHeapBlock)
{
	InHeapBlock->Release();
}

void OnlineDescriptorManager::Recycle(OnlineDescriptorBlock* Block)
{
	Block->SizeUsed = 0;
	FreeBlocks.emplace(Block);
}
