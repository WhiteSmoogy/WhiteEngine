#include <WFramework/WCLib/Logger.h>
#include "NodeDevice.h"
#include "DescriptorCache.h"
#include "ContextStateCache.h"
#include "CommandContext.h"
#include "RootSignature.h"
#include "SamplerState.h"
#include "System/Win32/WindowsPlatformMath.h"

using namespace platform_ex::Windows::D3D12;

DescriptorCache::DescriptorCache(GPUMaskType Node)
	:DeviceChild(nullptr)
	,SingleNodeGPUObject(Node)
	, pNullSRV(nullptr)
	, pNullRTV(nullptr)
	, pNullUAV(nullptr)
	, pDefaultSampler(nullptr)
	, pPreviousViewHeap(nullptr)
	, pPreviousSamplerHeap(nullptr)
	, CurrentViewHeap(nullptr)
	, CurrentSamplerHeap(nullptr)
	, LocalViewHeap(nullptr)
	, LocalSamplerHeap(nullptr, Node, this)
	, SubAllocatedViewHeap(nullptr, Node, this)
	, SamplerMap(271) // Prime numbers for better hashing
	, bUsingGlobalSamplerHeap(false)
	, NumLocalViewDescriptors(0)
	,CmdContext(nullptr)
{
}



void DescriptorCache::Init(NodeDevice* InParent, CommandContext* InCmdContext, uint32 InNumLocalViewDescriptors, uint32 InNumSamplerDescriptors, SubAllocatedOnlineHeap::SubAllocationDesc& SubHeapDesc)
{
	Parent = InParent;
	CmdContext = InCmdContext;

	SubAllocatedViewHeap.SetParent(this);
	LocalSamplerHeap.SetParent(this);

	SubAllocatedViewHeap.SetParentDevice(InParent);
	LocalSamplerHeap.SetParentDevice(InParent);

	SubAllocatedViewHeap.Init(SubHeapDesc);
	
	// Always Init a local sampler heap as the high level cache will always miss initialy
	// so we need something to fall back on (The view heap never rolls over so we init that one
	// lazily as a backup to save memory)
	LocalSamplerHeap.Init(InNumSamplerDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

	NumLocalViewDescriptors = InNumLocalViewDescriptors;

	CurrentViewHeap = &SubAllocatedViewHeap; //Begin with the global heap
	CurrentSamplerHeap = &LocalSamplerHeap;
	bUsingGlobalSamplerHeap = false;

	// Create default views
	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	SRVDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	SRVDesc.Texture2D.MipLevels = 1;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	pNullSRV = new DescriptorHandleSRV(GetParentDevice());
	pNullSRV->CreateView(SRVDesc, nullptr);

	D3D12_RENDER_TARGET_VIEW_DESC RTVDesc = {};
	RTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	RTVDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	RTVDesc.Texture2D.MipSlice = 0;
	pNullRTV = new DescriptorHandleRTV(GetParentDevice());
	pNullRTV->CreateView(RTVDesc, nullptr);

	D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
	UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	UAVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	UAVDesc.Texture2D.MipSlice = 0;
	pNullUAV = new DescriptorHandleUAV(GetParentDevice());
	pNullUAV->CreateViewWithCounter(UAVDesc, nullptr, nullptr);

	platform::Render::TextureSampleDesc SamplerDesc;
	SamplerDesc.address_mode_u = SamplerDesc.address_mode_v = SamplerDesc.address_mode_w = platform::Render::TexAddressingMode::Clamp;
	SamplerDesc.filtering = platform::Render::TexFilterOp::Min_Mag_Mip_Linear;
	SamplerDesc.max_anisotropy = 0;

	pDefaultSampler = InParent->CreateSampler(Convert(SamplerDesc));
}

void DescriptorCache::Clear()
{
	delete pNullRTV;
	delete pNullSRV;
	delete pNullUAV;

	pNullRTV = nullptr;
	pNullSRV = nullptr;
	pNullUAV = nullptr;
}

void DescriptorCache::BeginFrame()
{
	auto& DeviceSamplerHeap = GetParentDevice()->GetGlobalSamplerHeap();

	{
		std::unique_lock Lock(DeviceSamplerHeap.GetCriticalSection());
		if (DeviceSamplerHeap.DescriptorTablesDirty())
		{
			LocalSamplerSet = DeviceSamplerHeap.GetUniqueDescriptorTables();
		}
	}

	SwitchToGlobalSamplerHeap();
}

void DescriptorCache::EndFrame()
{
	if (!UniqueTables.empty())
	{
		GatherUniqueSamplerTables();
	}
}

void DescriptorCache::GatherUniqueSamplerTables()
{
	auto& DeviceSamplerHeap = GetParentDevice()->GetGlobalSamplerHeap();

	std::unique_lock Lock(DeviceSamplerHeap.GetCriticalSection());

	auto& TableSet = DeviceSamplerHeap.GetUniqueDescriptorTables();

	for (auto& Table : UniqueTables)
	{
		if (TableSet.count(Table) == 0)
		{
			if (DeviceSamplerHeap.CanReserveSlots(Table.Key.Count))
			{
				uint32 HeapSlot = DeviceSamplerHeap.ReserveSlots(Table.Key.Count);

				if (HeapSlot != GlobalOnlineHeap::HeapExhaustedValue)
				{
					D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor = DeviceSamplerHeap.GetCPUSlotHandle(HeapSlot);

					GetParentDevice()->GetDevice()->CopyDescriptors(
						1, &DestDescriptor, &Table.Key.Count,
						Table.Key.Count, Table.CPUTable, nullptr /* sizes */,
						D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

					Table.GPUHandle = DeviceSamplerHeap.GetGPUSlotHandle(HeapSlot);
					TableSet.emplace(Table);

					DeviceSamplerHeap.ToggleDescriptorTablesDirtyFlag(true);
				}
			}
		}
	}

	// Reset the tables as the next frame should inherit them from the global heap
	UniqueTables.clear();
}

bool DescriptorCache::SetDescriptorHeaps()
{
	// Sometimes there is no underlying command list for the context.
	// In that case, there is nothing to do and that's ok since we'll call this function again later when a command list is opened.
	if (CmdContext->CommandListHandle == nullptr)
	{
		return false;
	}

	// See if the descriptor heaps changed.
	bool bHeapChanged = false;
	ID3D12DescriptorHeap* const pCurrentViewHeap = CurrentViewHeap->GetHeap();
	if (pPreviousViewHeap != pCurrentViewHeap)
	{
		// The view heap changed, so dirty the descriptor tables.
		bHeapChanged = true;
		CmdContext->StateCache.DirtyViewDescriptorTables();
	}

	ID3D12DescriptorHeap* const pCurrentSamplerHeap = CurrentSamplerHeap->GetHeap();
	if (pPreviousSamplerHeap != pCurrentSamplerHeap)
	{
		// The sampler heap changed, so dirty the descriptor tables.
		bHeapChanged = true;
		CmdContext->StateCache.DirtySamplerDescriptorTables();

		// Reset the sampler map since it will have invalid entries for the new heap.
		SamplerMap.Reset();
	}

	// Set the descriptor heaps.
	if (bHeapChanged)
	{
		ID3D12DescriptorHeap* /*const*/ ppHeaps[] = { pCurrentViewHeap, pCurrentSamplerHeap };
		CmdContext->CommandListHandle->SetDescriptorHeaps(white::size(ppHeaps), ppHeaps);

		pPreviousViewHeap = pCurrentViewHeap;
		pPreviousSamplerHeap = pCurrentSamplerHeap;
	}

	wconstraint(pPreviousSamplerHeap == pCurrentSamplerHeap);
	wconstraint(pPreviousViewHeap == pCurrentViewHeap);
	return bHeapChanged;
}

void DescriptorCache::NotifyCurrentCommandList(CommandListHandle& CommandListHandle)
{
	// Clear the previous heap pointers (since it's a new command list) and then set the current descriptor heaps.
	pPreviousViewHeap = nullptr;
	pPreviousSamplerHeap = nullptr;
	SetDescriptorHeaps();

	CurrentViewHeap->NotifyCurrentCommandList(CommandListHandle);

	// The global sampler heap doesn't care about the current command list
	LocalSamplerHeap.NotifyCurrentCommandList(CommandListHandle);
}

void DescriptorCache::SetVertexBuffers(VertexBufferCache& Cache)
{
	const uint32 Count = Cache.MaxBoundVertexBufferIndex + 1;
	if (Count == 0)
	{
		return; // No-op
	}

	CmdContext->CommandListHandle->IASetVertexBuffers(0, Count, Cache.CurrentVertexBufferViews);
}



void DescriptorCache::SetRenderTargets(RenderTargetView** RenderTargetViewArray, uint32 Count, DepthStencilView* DepthStencilTarget)
{
	// NOTE: For this function, setting zero render targets might not be a no-op, since this is also used
	//       sometimes for only setting a depth stencil.

	D3D12_CPU_DESCRIPTOR_HANDLE RTVDescriptors[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];

	auto& CommandList = CmdContext->CommandListHandle;

	// Fill heap slots
	for (uint32 i = 0; i < Count; i++)
	{
		if (RenderTargetViewArray[i] != NULL)
		{
			// RTV should already be in the correct state. It is transitioned in RHISetRenderTargets.
			TransitionResource(CommandList, RenderTargetViewArray[i], D3D12_RESOURCE_STATE_RENDER_TARGET);
			RTVDescriptors[i] = RenderTargetViewArray[i]->GetView();
		}
		else
		{
			RTVDescriptors[i] = pNullRTV->GetHandle();
		}
	}

	if (DepthStencilTarget != nullptr)
	{
		TransitionResource(CommandList, DepthStencilTarget);

		const D3D12_CPU_DESCRIPTOR_HANDLE DSVDescriptor = DepthStencilTarget->GetView();
		CommandList->OMSetRenderTargets(Count, RTVDescriptors, 0, &DSVDescriptor);
	}
	else
	{
		CommandList->OMSetRenderTargets(Count, RTVDescriptors, 0, nullptr);
	}
}



template <ShaderType ShaderStage>
void DescriptorCache::SetUAVs(const RootSignature* RootSignature, UnorderedAccessViewCache& Cache, const UAVSlotMask& SlotsNeededMask, uint32 SlotsNeeded, uint32& HeapSlot)
{
	static_assert(ShaderStage < ShaderType::NumStandardType, "Unexpected shader frequency.");

	UAVSlotMask& CurrentDirtySlotMask = Cache.DirtySlotMask[ShaderStage];
	wconstraint(CurrentDirtySlotMask != 0);	// All dirty slots for the current shader stage.
	wconstraint(SlotsNeededMask != 0);		// All dirty slots for the current shader stage AND used by the current shader stage.
	wconstraint(SlotsNeeded != 0);

	// Reserve heap slots
	// Note: SlotsNeeded already accounts for the UAVStartSlot. For example, if a shader has 4 UAVs starting at slot 2 then SlotsNeeded will be 6 (because the root descriptor table currently starts at slot 0).
	uint32 FirstSlotIndex = HeapSlot;
	HeapSlot += SlotsNeeded;

	CD3DX12_CPU_DESCRIPTOR_HANDLE DestDescriptor(CurrentViewHeap->GetCPUSlotHandle(FirstSlotIndex));
	CD3DX12_GPU_DESCRIPTOR_HANDLE BindDescriptor(CurrentViewHeap->GetGPUSlotHandle(FirstSlotIndex));
	CD3DX12_CPU_DESCRIPTOR_HANDLE SrcDescriptors[MAX_UAVS];

	auto& CommandList = CmdContext->CommandListHandle;

	const uint32 UAVStartSlot = Cache.StartSlot[ShaderStage];
	auto& UAVs = Cache.Views[ShaderStage];

	// Fill heap slots
	for (uint32 SlotIndex = 0; SlotIndex < SlotsNeeded; SlotIndex++)
	{
		if ((SlotIndex < UAVStartSlot) || (UAVs[SlotIndex] == nullptr))
		{
			SrcDescriptors[SlotIndex] = pNullUAV->GetHandle();
		}
		else
		{
			SrcDescriptors[SlotIndex] = UAVs[SlotIndex]->GetView();

			TransitionResource(CommandList, UAVs[SlotIndex], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		}
	}
	UnorderedAccessViewCache::CleanSlots(CurrentDirtySlotMask, SlotsNeeded);

	wconstraint((CurrentDirtySlotMask & SlotsNeededMask) == 0);	// Check all slots that needed to be set, were set.

	// Gather the descriptors from the offline heaps to the online heap
	GetParentDevice()->GetDevice()->CopyDescriptors(
		1, &DestDescriptor, &SlotsNeeded,
		SlotsNeeded, SrcDescriptors, nullptr /* sizes */,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	if (ShaderStage == ShaderType::PixelShader)
	{
		const uint32 RDTIndex = RootSignature->UAVRDTBindSlot(ShaderStage);
		CommandList->SetGraphicsRootDescriptorTable(RDTIndex, BindDescriptor);
	}
	else
	{
		wconstraint(ShaderStage == ShaderType::ComputeShader);
		const uint32 RDTIndex = RootSignature->UAVRDTBindSlot(ShaderStage);
		CommandList->SetComputeRootDescriptorTable(RDTIndex, BindDescriptor);
	}

	// We changed the descriptor table, so all resources bound to slots outside of the table's range are now dirty.
	// If a shader needs to use resources bound to these slots later, we need to set the descriptor table again to ensure those
	// descriptors are valid.
	const UAVSlotMask OutsideCurrentTableRegisterMask = ~(((UAVSlotMask)1 << SlotsNeeded) - (UAVSlotMask)1);
	Cache.Dirty(ShaderStage, OutsideCurrentTableRegisterMask);
}

template void DescriptorCache::SetUAVs<ShaderType::PixelShader>(const RootSignature* RootSignature, UnorderedAccessViewCache& Cache, const UAVSlotMask& SlotsNeededMask, uint32 SlotsNeeded, uint32& HeapSlot);
template void DescriptorCache::SetUAVs<ShaderType::ComputeShader>(const RootSignature* RootSignature, UnorderedAccessViewCache& Cache, const UAVSlotMask& SlotsNeededMask, uint32 SlotsNeeded, uint32& HeapSlot);

template <ShaderType ShaderStage>
void DescriptorCache::SetSRVs(const RootSignature* RootSignature, ShaderResourceViewCache& Cache, const SRVSlotMask& SlotsNeededMask, uint32 SlotsNeeded, uint32& HeapSlot)
{
	static_assert(ShaderStage < ShaderType::NumStandardType, "Unexpected shader frequency.");

	SRVSlotMask& CurrentDirtySlotMask = Cache.DirtySlotMask[ShaderStage];
	wconstraint(CurrentDirtySlotMask != 0);	// All dirty slots for the current shader stage.
	wconstraint(SlotsNeededMask != 0);		// All dirty slots for the current shader stage AND used by the current shader stage.
	wconstraint(SlotsNeeded != 0);

	auto& CommandList = CmdContext->CommandListHandle;

	auto& SRVs = Cache.Views[ShaderStage];

	// Reserve heap slots
	uint32 FirstSlotIndex = HeapSlot;
	HeapSlot += SlotsNeeded;

	D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor = CurrentViewHeap->GetCPUSlotHandle(FirstSlotIndex);
	D3D12_CPU_DESCRIPTOR_HANDLE SrcDescriptors[MAX_SRVS];

	for (uint32 SlotIndex = 0; SlotIndex < SlotsNeeded; SlotIndex++)
	{
		if (SRVs[SlotIndex] != nullptr)
		{
			SrcDescriptors[SlotIndex] = SRVs[SlotIndex]->GetView();

			if (SRVs[SlotIndex]->IsDepthStencilResource())
			{
				TransitionResource(CommandList, SRVs[SlotIndex], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_DEPTH_READ);
			}
			/*else if (SRVs[SlotIndex]->GetSkipFastClearFinalize())
			{
				TransitionResource(CommandList, SRVs[SlotIndex], CmdContext->SkipFastClearEliminateState);
			}*/
			else
			{
				TransitionResource(CommandList, SRVs[SlotIndex], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			}
		}
		else
		{
			SrcDescriptors[SlotIndex] = pNullSRV->GetHandle();
		}
		wconstraint(SrcDescriptors[SlotIndex].ptr != 0);
	}
	ShaderResourceViewCache::CleanSlots(CurrentDirtySlotMask, SlotsNeeded);

	ID3D12Device* Device = GetParentDevice()->GetDevice();
	Device->CopyDescriptors(1, &DestDescriptor, &SlotsNeeded, SlotsNeeded, SrcDescriptors, nullptr, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);


	wconstraint((CurrentDirtySlotMask & SlotsNeededMask) == 0);	// Check all slots that needed to be set, were set.

	const D3D12_GPU_DESCRIPTOR_HANDLE BindDescriptor = CurrentViewHeap->GetGPUSlotHandle(FirstSlotIndex);

	if (ShaderStage == ShaderType::ComputeShader)
	{
		const uint32 RDTIndex = RootSignature->SRVRDTBindSlot(ShaderStage);
		CommandList->SetComputeRootDescriptorTable(RDTIndex, BindDescriptor);
	}
	else
	{
		const uint32 RDTIndex = RootSignature->SRVRDTBindSlot(ShaderStage);
		CommandList->SetGraphicsRootDescriptorTable(RDTIndex, BindDescriptor);
	}

	// We changed the descriptor table, so all resources bound to slots outside of the table's range are now dirty.
	// If a shader needs to use resources bound to these slots later, we need to set the descriptor table again to ensure those
	// descriptors are valid.
	const SRVSlotMask OutsideCurrentTableRegisterMask = ~(((SRVSlotMask)1 << SlotsNeeded) - (SRVSlotMask)1);
	Cache.Dirty(ShaderStage, OutsideCurrentTableRegisterMask);
}

template void DescriptorCache::SetSRVs<ShaderType::VertexShader>(const RootSignature* RootSignature, ShaderResourceViewCache& Cache, const SRVSlotMask& SlotsNeededMask, uint32 SlotsNeeded, uint32& HeapSlot);
template void DescriptorCache::SetSRVs<ShaderType::HullShader>(const RootSignature* RootSignature, ShaderResourceViewCache& Cache, const SRVSlotMask& SlotsNeededMask, uint32 SlotsNeeded, uint32& HeapSlot);
template void DescriptorCache::SetSRVs<ShaderType::DomainShader>(const RootSignature* RootSignature, ShaderResourceViewCache& Cache, const SRVSlotMask& SlotsNeededMask, uint32 SlotsNeeded, uint32& HeapSlot);
template void DescriptorCache::SetSRVs<ShaderType::GeometryShader>(const RootSignature* RootSignature, ShaderResourceViewCache& Cache, const SRVSlotMask& SlotsNeededMask, uint32 SlotsNeeded, uint32& HeapSlot);
template void DescriptorCache::SetSRVs<ShaderType::PixelShader>(const RootSignature* RootSignature, ShaderResourceViewCache& Cache, const SRVSlotMask& SlotsNeededMask, uint32 SlotsNeeded, uint32& HeapSlot);
template void DescriptorCache::SetSRVs<ShaderType::ComputeShader>(const RootSignature* RootSignature, ShaderResourceViewCache& Cache, const SRVSlotMask& SlotsNeededMask, uint32 SlotsNeeded, uint32& HeapSlot);

template <ShaderType ShaderStage>
void DescriptorCache::SetConstantBuffers(const RootSignature* RootSignature, ConstantBufferCache& Cache, const CBVSlotMask& SlotsNeededMask)
{
	static_assert(ShaderStage < ShaderType::NumStandardType, "Unexpected shader frequency.");

	CBVSlotMask& CurrentDirtySlotMask = Cache.DirtySlotMask[ShaderStage];
	wconstraint(CurrentDirtySlotMask != 0);	// All dirty slots for the current shader stage.
	wconstraint(SlotsNeededMask != 0);		// All dirty slots for the current shader stage AND used by the current shader stage.

	auto& CommandList = CmdContext->CommandListHandle;
	ID3D12Device* Device = GetParentDevice()->GetDevice();

	// Process root CBV
	const CBVSlotMask RDCBVSlotsNeededMask = GRootCBVSlotMask & SlotsNeededMask;
	wconstraint(RDCBVSlotsNeededMask); // Check this wasn't a wasted call.

	//Set root descriptors.
	// At least one needed root descriptor is dirty.
	const uint32 BaseIndex = RootSignature->CBVRDBaseBindSlot(ShaderStage);
	wconstraint(BaseIndex != 255);
	const uint32 RDCBVsNeeded = FloorLog2(RDCBVSlotsNeededMask) + 1;	// Get the index of the most significant bit that's set.
	wconstraint(RDCBVsNeeded <= MAX_ROOT_CBVS);
	for (uint32 SlotIndex = 0; SlotIndex < RDCBVsNeeded; SlotIndex++)
	{
		// Only set the root descriptor if it's dirty and we need to set it (it can be used by the shader).
		if (ConstantBufferCache::IsSlotDirty(RDCBVSlotsNeededMask, SlotIndex))
		{
			const D3D12_GPU_VIRTUAL_ADDRESS& CurrentGPUVirtualAddress = Cache.CurrentGPUVirtualAddress[ShaderStage][SlotIndex];
			wconstraint(CurrentGPUVirtualAddress != 0);
			if (ShaderStage == ShaderType::ComputeShader)
			{
				CommandList->SetComputeRootConstantBufferView(BaseIndex + SlotIndex, CurrentGPUVirtualAddress);
			}
			else
			{
				CommandList->SetGraphicsRootConstantBufferView(BaseIndex + SlotIndex, CurrentGPUVirtualAddress);
			}

			// Clear the dirty bit.
			ConstantBufferCache::CleanSlot(CurrentDirtySlotMask, SlotIndex);
		}
	}
	wconstraint((CurrentDirtySlotMask & RDCBVSlotsNeededMask) == 0);	// Check all slots that needed to be set, were set.

	static_assert(GDescriptorTableCBVSlotMask == 0, "DescriptorCache::SetConstantBuffers needs to be updated to handle descriptor tables.");	// Check that all CBVs slots are controlled by root descriptors.
}

template void DescriptorCache::SetConstantBuffers<ShaderType::VertexShader>(const RootSignature* RootSignature, ConstantBufferCache& Cache, const CBVSlotMask& SlotsNeededMask);
template void DescriptorCache::SetConstantBuffers<ShaderType::HullShader>(const RootSignature* RootSignature, ConstantBufferCache& Cache, const CBVSlotMask& SlotsNeededMask);
template void DescriptorCache::SetConstantBuffers<ShaderType::DomainShader>(const RootSignature* RootSignature, ConstantBufferCache& Cache, const CBVSlotMask& SlotsNeededMask);
template void DescriptorCache::SetConstantBuffers<ShaderType::GeometryShader>(const RootSignature* RootSignature, ConstantBufferCache& Cache, const CBVSlotMask& SlotsNeededMask);
template void DescriptorCache::SetConstantBuffers<ShaderType::PixelShader>(const RootSignature* RootSignature, ConstantBufferCache& Cache, const CBVSlotMask& SlotsNeededMask);
template void DescriptorCache::SetConstantBuffers<ShaderType::ComputeShader>(const RootSignature* RootSignature, ConstantBufferCache& Cache, const CBVSlotMask& SlotsNeededMask);

template <ShaderType ShaderStage>
void DescriptorCache::SetSamplers(const RootSignature* RootSignature, SamplerStateCache& Cache, const SamplerSlotMask& SlotsNeededMask, uint32 SlotsNeeded, uint32& HeapSlot)
{
	static_assert(ShaderStage < ShaderType::NumStandardType, "Unexpected shader frequency.");

	wconstraint(CurrentSamplerHeap != &GetParentDevice()->GetGlobalSamplerHeap());
	wconstraint(bUsingGlobalSamplerHeap == false);

	SamplerSlotMask& CurrentDirtySlotMask = Cache.DirtySlotMask[ShaderStage];
	wconstraint(CurrentDirtySlotMask != 0);	// All dirty slots for the current shader stage.
	wconstraint(SlotsNeededMask != 0);		// All dirty slots for the current shader stage AND used by the current shader stage.
	wconstraint(SlotsNeeded != 0);

	auto& Samplers = Cache.States[ShaderStage];

	D3D12_GPU_DESCRIPTOR_HANDLE BindDescriptor = { 0 };
	bool CacheHit = false;

	// Check to see if the sampler configuration is already in the sampler heap
	SamplerArrayDesc Desc = {};
	if (SlotsNeeded <= white::size(Desc.SamplerID))
	{
		Desc.Count = SlotsNeeded;

		SamplerSlotMask CacheDirtySlotMask = CurrentDirtySlotMask;	// Temp mask
		for (uint32 SlotIndex = 0; SlotIndex < SlotsNeeded; SlotIndex++)
		{
			Desc.SamplerID[SlotIndex] = Samplers[SlotIndex] ? Samplers[SlotIndex]->ID : 0;
		}
		SamplerStateCache::CleanSlots(CacheDirtySlotMask, SlotsNeeded);

		// The hash uses all of the bits
		for (uint32 SlotIndex = SlotsNeeded; SlotIndex <white::size(Desc.SamplerID); SlotIndex++)
		{
			Desc.SamplerID[SlotIndex] = 0;
		}

		D3D12_GPU_DESCRIPTOR_HANDLE* FoundDescriptor = SamplerMap.Find(Desc);
		if (FoundDescriptor)
		{
			wconstraint(IsHeapSet(LocalSamplerHeap.GetHeap()));
			BindDescriptor = *FoundDescriptor;
			CacheHit = true;
			CurrentDirtySlotMask = CacheDirtySlotMask;
		}
	}

	if (!CacheHit)
	{
		// Reserve heap slots
		const uint32 FirstSlotIndex = HeapSlot;
		HeapSlot += SlotsNeeded;
		D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor = CurrentSamplerHeap->GetCPUSlotHandle(FirstSlotIndex);
		BindDescriptor = CurrentSamplerHeap->GetGPUSlotHandle(FirstSlotIndex);

		wconstraint(SlotsNeeded <= MAX_SAMPLERS);

		// Fill heap slots
		CD3DX12_CPU_DESCRIPTOR_HANDLE SrcDescriptors[MAX_SAMPLERS];
		for (uint32 SlotIndex = 0; SlotIndex < SlotsNeeded; SlotIndex++)
		{
			if (Samplers[SlotIndex] != nullptr)
			{
				SrcDescriptors[SlotIndex] = Samplers[SlotIndex]->Descriptor;
			}
			else
			{
				SrcDescriptors[SlotIndex] = pDefaultSampler->Descriptor;
			}
		}
		SamplerStateCache::CleanSlots(CurrentDirtySlotMask, SlotsNeeded);

		GetParentDevice()->GetDevice()->CopyDescriptors(
			1, &DestDescriptor, &SlotsNeeded,
			SlotsNeeded, SrcDescriptors, nullptr /* sizes */,
			D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

		// Remember the locations of the samplers in the sampler map
		if (SlotsNeeded <= white::size(Desc.SamplerID))
		{
			UniqueTables.emplace_back(Desc, SrcDescriptors);

			SamplerMap.Add(Desc, BindDescriptor);
		}
	}

	auto& CommandList = CmdContext->CommandListHandle;

	if (ShaderStage == ShaderType::ComputeShader)
	{
		const uint32 RDTIndex = RootSignature->SamplerRDTBindSlot(ShaderStage);
		CommandList->SetComputeRootDescriptorTable(RDTIndex, BindDescriptor);
	}
	else
	{
		const uint32 RDTIndex = RootSignature->SamplerRDTBindSlot(ShaderStage);
		CommandList->SetGraphicsRootDescriptorTable(RDTIndex, BindDescriptor);
	}

	// We changed the descriptor table, so all resources bound to slots outside of the table's range are now dirty.
	// If a shader needs to use resources bound to these slots later, we need to set the descriptor table again to ensure those
	// descriptors are valid.
	const SamplerSlotMask OutsideCurrentTableRegisterMask = ~(((SamplerSlotMask)1 << SlotsNeeded) - (SamplerSlotMask)1);
	Cache.Dirty(ShaderStage, OutsideCurrentTableRegisterMask);
}

template void DescriptorCache::SetSamplers<ShaderType::VertexShader>(const RootSignature* RootSignature, SamplerStateCache& Cache, const SamplerSlotMask& SlotsNeededMask, uint32 Count, uint32& HeapSlot);
template void DescriptorCache::SetSamplers<ShaderType::HullShader>(const RootSignature* RootSignature, SamplerStateCache& Cache, const SamplerSlotMask& SlotsNeededMask, uint32 Count, uint32& HeapSlot);
template void DescriptorCache::SetSamplers<ShaderType::DomainShader>(const RootSignature* RootSignature, SamplerStateCache& Cache, const SamplerSlotMask& SlotsNeededMask, uint32 Count, uint32& HeapSlot);
template void DescriptorCache::SetSamplers<ShaderType::GeometryShader>(const RootSignature* RootSignature, SamplerStateCache& Cache, const SamplerSlotMask& SlotsNeededMask, uint32 Count, uint32& HeapSlot);
template void DescriptorCache::SetSamplers<ShaderType::PixelShader>(const RootSignature* RootSignature, SamplerStateCache& Cache, const SamplerSlotMask& SlotsNeededMask, uint32 Count, uint32& HeapSlot);
template void DescriptorCache::SetSamplers<ShaderType::ComputeShader>(const RootSignature* RootSignature, SamplerStateCache& Cache, const SamplerSlotMask& SlotsNeededMask, uint32 Count, uint32& HeapSlot);

void DescriptorCache::SetStreamOutTargets(ResourceHolder** Buffers, uint32 Count, const uint32* Offsets)
{
	// Determine how many slots are really needed, since the Count passed in is a pre-defined maximum
	uint32 SlotsNeeded = 0;
	for (int32 i = Count - 1; i >= 0; i--)
	{
		if (Buffers[i] != NULL)
		{
			SlotsNeeded = i + 1;
			break;
		}
	}

	if (0 == SlotsNeeded)
	{
		return; // No-op
	}

	D3D12_STREAM_OUTPUT_BUFFER_VIEW SOViews[D3D12_SO_BUFFER_SLOT_COUNT] = { };

	auto& CommandList = CmdContext->CommandListHandle;

	// Fill heap slots
	for (uint32 i = 0; i < SlotsNeeded; i++)
	{
		if (Buffers[i])
		{
		}

		D3D12_STREAM_OUTPUT_BUFFER_VIEW& currentView = SOViews[i];
		currentView.BufferLocation = (Buffers[i] != nullptr) ? Buffers[i]->GetGPUVirtualAddress() : 0;

		// MS - The following view members are not correct
		wconstraint(0);
		currentView.BufferFilledSizeLocation = 0;
		currentView.SizeInBytes = -1;

		if (Buffers[i] != nullptr)
		{
			TransitionResource(CommandList, Buffers[i], D3D12_RESOURCE_STATE_STREAM_OUT, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
		}
	}

	CommandList->SOSetTargets(0, SlotsNeeded, SOViews);
}

bool DescriptorCache::HeapRolledOver(D3D12_DESCRIPTOR_HEAP_TYPE Type)
{
	// A heap rolled over, so set the descriptor heaps again and return if the heaps actually changed.
	return SetDescriptorHeaps();
}

void DescriptorCache::HeapLoopedAround(D3D12_DESCRIPTOR_HEAP_TYPE Type)
{
	if (Type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
	{
		SamplerMap.Reset();
	}
}

bool DescriptorCache::SwitchToContextLocalViewHeap(CommandListHandle& CommandListHandle)
{
	if (LocalViewHeap == nullptr)
	{
		WF_Trace(platform::Descriptions::Informative,"This should only happen in the Editor where it doesn't matter as much. If it happens in game you should increase the device global heap size!");

		// Allocate the heap lazily
		LocalViewHeap = new ThreadLocalOnlineHeap(GetParentDevice(),GetGPUMask(), this);
		if (LocalViewHeap)
		{
			wconstraint(NumLocalViewDescriptors);
			LocalViewHeap->Init(NumLocalViewDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
		else
		{
			wconstraint(false);
			return false;
		}
	}

	LocalViewHeap->NotifyCurrentCommandList(CommandListHandle);
	CurrentViewHeap = LocalViewHeap;
	const bool bDescriptorHeapsChanged = SetDescriptorHeaps();

	wconstraint(IsHeapSet(LocalViewHeap->GetHeap()));
	return bDescriptorHeapsChanged;
}

bool DescriptorCache::SwitchToContextLocalSamplerHeap()
{
	bool bDescriptorHeapsChanged = false;
	if (UsingGlobalSamplerHeap())
	{
		bUsingGlobalSamplerHeap = false;
		CurrentSamplerHeap = &LocalSamplerHeap;
		bDescriptorHeapsChanged = SetDescriptorHeaps();
	}

	wconstraint(IsHeapSet(LocalSamplerHeap.GetHeap()));
	return bDescriptorHeapsChanged;
}

bool DescriptorCache::SwitchToGlobalSamplerHeap()
{
	bool bDescriptorHeapsChanged = false;
	if (!UsingGlobalSamplerHeap())
	{
		bUsingGlobalSamplerHeap = true;
		CurrentSamplerHeap = &GetParentDevice()->GetGlobalSamplerHeap();
		bDescriptorHeapsChanged = SetDescriptorHeaps();
	}

	// Sometimes this is called when there is no underlying command list.
	// This is OK, as the desriptor heaps will be set when a command list is opened.
	wconstraint((CmdContext->CommandListHandle == nullptr) || IsHeapSet(GetParentDevice()->GetGlobalSamplerHeap().GetHeap()));
	return bDescriptorHeapsChanged;
}

void ThreadLocalOnlineHeap::Init(uint32 NumDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE Type)
{
	Desc = {};
	Desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	Desc.Type = Type;
	Desc.NumDescriptors = NumDescriptors;
	Desc.NodeMask = GPUIndex;

	//LLM_SCOPE(ELLMTag::DescriptorCache);
	CheckHResult(GetParentDevice()->GetDevice()->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(&Heap.ReleaseAndGetRef())));
	D3D::Debug(Heap, Desc.Type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ? "Thread Local - Online View Heap" : "Thread Local - Online Sampler Heap");

	Entry.Heap = Heap;

	CPUBase = Heap->GetCPUDescriptorHandleForHeapStart();
	GPUBase = Heap->GetGPUDescriptorHandleForHeapStart();
	DescriptorSize = GetParentDevice()->GetDevice()->GetDescriptorHandleIncrementSize(Type);

	if (Desc.Type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
	{
	}
	else
	{
	}
}

bool ThreadLocalOnlineHeap::RollOver()
{
	// Enqueue the current entry
	WAssert(CurrentCommandList != nullptr, "Would have set up a sync point with a null commandlist.");
	Entry.SyncPoint = CurrentCommandList;
	ReclaimPool.push(Entry);

	if (!ReclaimPool.empty() && !!(Entry = ReclaimPool.front()).SyncPoint && Entry.SyncPoint.IsComplete())
	{
		ReclaimPool.pop();

		Heap = Entry.Heap;
	}
	else
	{
		WF_Trace(platform::Descriptions::Informative, "OnlineHeap RollOver Detected. Increase the heap size to prevent creation of additional heaps");

		//LLM_SCOPE(ELLMTag::DescriptorCache);

		CheckHResult(GetParentDevice()->GetDevice()->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(&Heap.ReleaseAndGetRef())));
		D3D::Debug(Heap, Desc.Type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ? "Thread Local - Online View Heap" : "Thread Local - Online Sampler Heap");

		if (Desc.Type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
		{
		}
		else
		{
		}

		Entry.Heap = Heap;
	}

	NextSlotIndex = 0;
	FirstUsedSlot = 0;

	// Notify other layers of heap change
	CPUBase = Heap->GetCPUDescriptorHandleForHeapStart();
	GPUBase = Heap->GetGPUDescriptorHandleForHeapStart();
	return Parent->HeapRolledOver(Desc.Type);
}

void ThreadLocalOnlineHeap::NotifyCurrentCommandList(CommandListHandle& CommandListHandle)
{
	if (CurrentCommandList != nullptr && NextSlotIndex > 0)
	{
		// Track the previous command list
		SyncPointEntry SyncPoint;
		SyncPoint.SyncPoint = CommandListHandle;
		SyncPoint.LastSlotInUse = NextSlotIndex - 1;
		SyncPoints.push(SyncPoint);

		Entry.SyncPoint = CommandListHandle;

		// Free up slots for finished command lists
		while (!SyncPoints.empty() && !!(SyncPoint = SyncPoints.front()).SyncPoint && SyncPoint.SyncPoint.IsComplete())
		{
			SyncPoints.pop();
			FirstUsedSlot = SyncPoint.LastSlotInUse + 1;
		}
	}

	// Update the current command list
	CurrentCommandList = CommandListHandle;
}



OnlineHeap::OnlineHeap(NodeDevice* Device, GPUMaskType Node, bool CanLoopAround, DescriptorCache* _Parent)
	:DeviceChild(Device)
	,SingleNodeGPUObject(Node)
	, Parent(_Parent)
	, CurrentCommandList()
	, DescriptorSize(0)
	, NextSlotIndex(0)
	, FirstUsedSlot(0)
	, Desc({})
	, bCanLoopAround(CanLoopAround)
{
}



bool OnlineHeap::CanReserveSlots(uint32 NumSlots)
{
	const uint32 HeapSize = GetTotalSize();

	// Sanity checks
	if (0 == NumSlots)
	{
		return true;
	}
	if (NumSlots > HeapSize)
	{
#if !defined(_HAS_EXCEPTIONS) || _HAS_EXCEPTIONS == 1
		throw E_OUTOFMEMORY;
#else
#endif
	}
	uint32 FirstRequestedSlot = NextSlotIndex;
	uint32 SlotAfterReservation = NextSlotIndex + NumSlots;

	// TEMP: Disable wrap around by not allowing it to reserve slots if the heap is full.
	if (SlotAfterReservation > HeapSize)
	{
		return false;
	}

	return true;
}

uint32 OnlineHeap::ReserveSlots(uint32 NumSlotsRequested)
{
	const uint32 HeapSize = GetTotalSize();

	// Sanity checks
	if (NumSlotsRequested > HeapSize)
	{
#if !defined(_HAS_EXCEPTIONS) || _HAS_EXCEPTIONS == 1
		throw E_OUTOFMEMORY;
#else
		return HeapExhaustedValue;
#endif
	}

	// CanReserveSlots should have been called first
	wconstraint(CanReserveSlots(NumSlotsRequested));

	// Decide which slots will be reserved and what needs to be cleaned up
	uint32 FirstRequestedSlot = NextSlotIndex;
	uint32 SlotAfterReservation = NextSlotIndex + NumSlotsRequested;

	// Loop around if the end of the heap has been reached
	if (bCanLoopAround && SlotAfterReservation > HeapSize)
	{
		FirstRequestedSlot = 0;
		SlotAfterReservation = NumSlotsRequested;

		FirstUsedSlot = SlotAfterReservation;

		Parent->HeapLoopedAround(Desc.Type);
	}

	// Note where to start looking next time
	NextSlotIndex = SlotAfterReservation;

	if (Desc.Type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
	{
	}
	else
	{
	}

	return FirstRequestedSlot;
}




void OnlineHeap::SetNextSlot(uint32 NextSlot)
{
	// For samplers, ReserveSlots will be called with a conservative estimate
	// This is used to correct for the actual number of heap slots used
	wconstraint(NextSlot <= NextSlotIndex);

	wconstraint(Desc.Type != D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	NextSlotIndex = NextSlot;
}


void OnlineHeap::NotifyCurrentCommandList(CommandListHandle& CommandListHandle)
{
	//Specialization should be called
	wconstraint(false);
}

void GlobalOnlineHeap::Init(uint32 TotalSize, D3D12_DESCRIPTOR_HEAP_TYPE Type)
{
	D3D12_DESCRIPTOR_HEAP_FLAGS HeapFlags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	Desc = {};
	Desc.Flags = HeapFlags;
	Desc.Type = Type;
	Desc.NumDescriptors = TotalSize;
	Desc.NodeMask = GPUIndex;

	//LLM_SCOPE(ELLMTag::DescriptorCache);

	CheckHResult(GetParentDevice()->GetDevice()->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(&Heap.ReleaseAndGetRef())));
	D3D::Debug(Heap, Desc.Type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ? "Device Global - Online View Heap" : "Device Global - Online Sampler Heap");

	CPUBase = Heap->GetCPUDescriptorHandleForHeapStart();
	GPUBase = Heap->GetGPUDescriptorHandleForHeapStart();
	DescriptorSize = GetParentDevice()->GetDevice()->GetDescriptorHandleIncrementSize(Desc.Type);

	if (Desc.Type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
	{
		// Reserve the whole heap for sub allocation
		ReserveSlots(TotalSize);
	}
	else
	{
	}
}

bool GlobalOnlineHeap::RollOver()
{
	wconstraint(false);
	WF_Trace(platform::Descriptions::Alert, "Global Descriptor heaps can't roll over!");
	return false;
}

void SubAllocatedOnlineHeap::Init(SubAllocationDesc _Desc)
{
	SubDesc = _Desc;

	const uint32 Blocks = SubDesc.Size / DESCRIPTOR_HEAP_BLOCK_SIZE;
	wconstraint(Blocks > 0);
	wconstraint(SubDesc.Size >= DESCRIPTOR_HEAP_BLOCK_SIZE);

	uint32 BaseSlot = SubDesc.BaseSlot;
	for (uint32 i = 0; i < Blocks; i++)
	{
		DescriptorBlockPool.push(OnlineHeapBlock(BaseSlot, DESCRIPTOR_HEAP_BLOCK_SIZE));
		wconstraint(BaseSlot + DESCRIPTOR_HEAP_BLOCK_SIZE <= SubDesc.ParentHeap->GetTotalSize());
		BaseSlot += DESCRIPTOR_HEAP_BLOCK_SIZE;
	}

	Heap = SubDesc.ParentHeap->GetHeap();

	DescriptorSize = SubDesc.ParentHeap->GetDescriptorSize();
	Desc = SubDesc.ParentHeap->GetDesc();

	CurrentSubAllocation = DescriptorBlockPool.front();
	DescriptorBlockPool.pop();

	CPUBase = SubDesc.ParentHeap->GetCPUSlotHandle(CurrentSubAllocation.BaseSlot);
	GPUBase = SubDesc.ParentHeap->GetGPUSlotHandle(CurrentSubAllocation.BaseSlot);
}

bool SubAllocatedOnlineHeap::RollOver()
{
	// Enqueue the current entry
	CurrentSubAllocation.SyncPoint = CurrentCommandList;
	CurrentSubAllocation.bFresh = false;
	DescriptorBlockPool.push(CurrentSubAllocation);

	if (!DescriptorBlockPool.empty() && (CurrentSubAllocation = DescriptorBlockPool.front(),
		(CurrentSubAllocation.bFresh || CurrentSubAllocation.SyncPoint.IsComplete())))
	{
		DescriptorBlockPool.pop();
	}
	else
	{
		// Notify parent that we have run out of sub allocations
		// This should *never* happen but we will handle it and revert to local heaps to be safe
		WF_Trace(platform::Descriptions::Informative,"Descriptor cache ran out of sub allocated descriptor blocks! Moving to Context local View heap strategy");
		return Parent->SwitchToContextLocalViewHeap(CurrentCommandList);
	}

	NextSlotIndex = 0;
	FirstUsedSlot = 0;

	// Notify other layers of heap change
	CPUBase = SubDesc.ParentHeap->GetCPUSlotHandle(CurrentSubAllocation.BaseSlot);
	GPUBase = SubDesc.ParentHeap->GetGPUSlotHandle(CurrentSubAllocation.BaseSlot);
	return false;	// Sub-allocated descriptor heaps don't change, so no need to set descriptor heaps.
}

void SubAllocatedOnlineHeap::NotifyCurrentCommandList(CommandListHandle& CommandListHandle)
{
	// Update the current command list
	CurrentCommandList = CommandListHandle;
}

uint32  platform_ex::Windows::D3D12::GetTypeHash(const SamplerArrayDesc& Key)
{
	return static_cast<uint32>(CityHash64((char*)Key.SamplerID,static_cast<white::uint32>(Key.Count * sizeof(Key.SamplerID[0]))));
}