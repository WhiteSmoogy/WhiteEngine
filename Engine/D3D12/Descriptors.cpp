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


DescriptorHeap::DescriptorHeap(NodeDevice* InDevice, COMPtr<ID3D12DescriptorHeap> InHeap, uint32 InNumDescriptors, DescriptorHeapType InType, D3D12_DESCRIPTOR_HEAP_FLAGS InFlags, bool bInIsGlobal)
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
		"Device Global - Online View Heap",
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

static uint32 GetOfflineDescriptorHeapDefaultSize(DescriptorHeapType InHeapType)
{
	switch (InHeapType)
	{
	default: throw unsupported();
	case DescriptorHeapType::Standard:     return 2048;
	case DescriptorHeapType::RenderTarget: return 256;
	case DescriptorHeapType::DepthStencil: return 256;
	case DescriptorHeapType::Sampler:      return 128;
	}
}

OfflineDescriptorManager::OfflineDescriptorManager(NodeDevice* InDevice, DescriptorHeapType InHeapType)
	:DeviceChild(InDevice),
	HeapType(InHeapType)
{
	DescriptorSize = GetParentDevice()->GetDevice()->GetDescriptorHandleIncrementSize(Convert(InHeapType));
	NumDescriptorsPerHeap = GetOfflineDescriptorHeapDefaultSize(InHeapType);
}


OfflineDescriptorManager::~OfflineDescriptorManager() = default;

void OfflineDescriptorManager::AllocateHeap()
{
	WAssert(NumDescriptorsPerHeap != 0, "Init() needs to be called before allocating heaps.");
	COMPtr<DescriptorHeap> Heap = GetParentDevice()->GetDescriptorHeapManager().AllocateHeap(
		"OfflineDescriptorManager",
		HeapType,
		NumDescriptorsPerHeap,
		D3D12_DESCRIPTOR_HEAP_FLAG_NONE
	);

	D3D12_CPU_DESCRIPTOR_HANDLE HeapBase = Heap->GetCPUSlotHandle(0);
	wassume(HeapBase.ptr != 0);

	// Allocate and initialize a single new entry in the map
	const uint32 NewHeapIndex = static_cast<uint32>(Heaps.size());

	Heaps.emplace_back(Heap, HeapBase, NumDescriptorsPerHeap * DescriptorSize);
	FreeHeaps.emplace_back(NewHeapIndex);
}

OfflineDescriptor OfflineDescriptorManager::AllocateHeapSlot()
{
	std::unique_lock Lock{Mutex};
	OfflineDescriptor Result;

	if (FreeHeaps.size() == 0)
	{
		AllocateHeap();
	}

	auto Head = FreeHeaps.begin();
	Result.HeapIndex = *Head;

	auto& HeapEntry = Heaps[Result.HeapIndex];
	wassume(0 != HeapEntry.FreeList.size());
	auto& Range = HeapEntry.FreeList.front();

	Result.ptr = Range.Start;
	Range.Start += DescriptorSize;

	if (Range.Start == Range.End)
	{
		HeapEntry.FreeList.erase(HeapEntry.FreeList.begin());
		if (HeapEntry.FreeList.size() == 0)
		{
			FreeHeaps.erase(Head);
		}
	}

	return Result;
}

void OfflineDescriptorManager::FreeHeapSlot(OfflineDescriptor& Descriptor)
{
	std::unique_lock Lock{ Mutex };
	auto& HeapEntry = Heaps[Descriptor.HeapIndex];

	const OfflineHeapFreeRange NewRange{ Descriptor.ptr, Descriptor.ptr + DescriptorSize };

	bool bFound = false;
	for (auto Node = HeapEntry.FreeList.begin();
		Node != HeapEntry.FreeList.end() && !bFound;
		++Node)
	{
		auto& Range = *Node;
		wassume(Range.Start < Range.End);
		if (Range.Start == Descriptor.ptr + DescriptorSize)
		{
			Range.Start = Descriptor.ptr;
			bFound = true;
		}
		else if (Range.End == Descriptor.ptr)
		{
			Range.End += DescriptorSize;
			bFound = true;
		}
		else
		{
			wassume(Range.End < Descriptor.ptr || Range.Start > Descriptor.ptr);
			if (Range.Start > Descriptor.ptr)
			{
				HeapEntry.FreeList.insert(Node,NewRange);
				bFound = true;
			}
		}
	}

	if (!bFound)
	{
		if (HeapEntry.FreeList.size() == 0)
		{
			FreeHeaps.emplace_back(Descriptor.HeapIndex);
		}
		HeapEntry.FreeList.emplace_back(NewRange);
	}

	Descriptor = {};
}

DescriptorHeapManager::DescriptorHeapManager(NodeDevice* InDevice)
	:DeviceChild(InDevice)
{}

DescriptorHeapManager::~DescriptorHeapManager()
{
	Destroy();
}

static DescriptorHeap* CreateDescriptorHeap(NodeDevice* Device, const char* DebugName, DescriptorHeapType HeapType, uint32 NumDescriptors, D3D12_DESCRIPTOR_HEAP_FLAGS Flags, bool bIsGlobal)
{
	D3D12_DESCRIPTOR_HEAP_DESC Desc{};
	Desc.Type = Convert(HeapType);
	Desc.NumDescriptors = NumDescriptors;
	Desc.Flags = Flags;
	Desc.NodeMask = Device->GetGPUMask().GetNative();

	platform_ex::COMPtr<ID3D12DescriptorHeap> Heap;
	platform_ex::CheckHResult(Device->GetDevice()->CreateDescriptorHeap(&Desc, COMPtr_RefParam(Heap,IID_ID3D12DescriptorHeap)));

	platform_ex::Windows::D3D::Debug(Heap, DebugName);

	return new DescriptorHeap(Device, Heap, NumDescriptors, HeapType, Flags, bIsGlobal);
}

void DescriptorHeapManager::Init(uint32 InNumGlobalResourceDescriptors, uint32 InNumGlobalSamplerDescriptors)
{
	if (InNumGlobalResourceDescriptors > 0)
	{
		auto Heap = CreateDescriptorHeap(
			GetParentDevice(),
			"GlobalResourceHeap",
			DescriptorHeapType::Standard,
			InNumGlobalResourceDescriptors,
			D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
			true /* bIsGlobal */
		);

		GlobalHeaps.emplace_back(GetParentDevice(), Heap);
	}

	if (InNumGlobalSamplerDescriptors > 0)
	{
		auto Heap = CreateDescriptorHeap(
			GetParentDevice(),
			"GlobalSamplerHeap",
			DescriptorHeapType::Sampler,
			InNumGlobalSamplerDescriptors,
			D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
			true /* bIsGlobal */
		);

		GlobalHeaps.emplace_back(GetParentDevice(), Heap);
	}
}

void DescriptorHeapManager::Destroy()
{
}

DescriptorHeap* DescriptorHeapManager::AllocateIndependentHeap(const char* InDebugName, DescriptorHeapType InHeapType, uint32 InNumDescriptors, D3D12_DESCRIPTOR_HEAP_FLAGS InHeapFlags)
{
	return CreateDescriptorHeap(GetParentDevice(), InDebugName, InHeapType, InNumDescriptors, InHeapFlags, false);
}

DescriptorHeap* DescriptorHeapManager::AllocateHeap(const char* InDebugName, DescriptorHeapType InHeapType, uint32 InNumDescriptors, D3D12_DESCRIPTOR_HEAP_FLAGS InHeapFlags)
{
	for (auto& GlobalHeap : GlobalHeaps)
	{
		if (GlobalHeap.HandlesAllocationWithFlags(InHeapType, InHeapFlags))
		{
			uint32 Offset = 0;
			if (GlobalHeap.Allocate(InNumDescriptors, Offset))
			{
				return new DescriptorHeap(GlobalHeap.GetHeap(), Offset, InNumDescriptors);
			}
		}
	}

	return AllocateIndependentHeap(InDebugName, InHeapType, InNumDescriptors, InHeapFlags);
}

void DescriptorHeapManager::DeferredFreeHeap(DescriptorHeap* InHeap)
{
	if (InHeap->IsGlobal())
	{
		for (auto& GlobalHeap : GlobalHeaps)
		{
			if (GlobalHeap.IsHeapAChild(InHeap))
			{
				InHeap->Release();
				return;
			}
		}
	}
	else
	{
		InHeap->Release();
	}
}

void DescriptorHeapManager::ImmediateFreeHeap(DescriptorHeap* InHeap)
{
	if (InHeap->IsGlobal())
	{
		for (auto& GlobalHeap : GlobalHeaps)
		{
			if (GlobalHeap.IsHeapAChild(InHeap))
			{
				GlobalHeap.Free(InHeap->GetOffset(), InHeap->GetNumDescriptors());
				delete InHeap;
				return;
			}
		}
	}
	else
	{
		delete InHeap;
	}
}

