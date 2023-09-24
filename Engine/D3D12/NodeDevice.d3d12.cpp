#include "NodeDevice.h"
#include "CommandContext.h"
#include "Adapter.h"

extern int GGlobalViewHeapSize;
using namespace platform_ex::Windows::D3D12;

NodeDevice::NodeDevice(GPUMaskType InGPUMask, D3D12Adapter* InAdapter)
	:SingleNodeGPUObject(InGPUMask), AdapterChild(InAdapter),
	OnlineDescriptorManager(this),
	GlobalSamplerHeap(this),
	NormalDescriptorHeapManager(this)
	//it's similar UploadHeapAllocator/FastConstantAllocator(64k pagesize)
	, DefaultFastAllocator(this,GPUMaskType::AllGPU(), D3D12_HEAP_TYPE_UPLOAD, 1024 * 1024 * 4)
	,DefaultBufferAllocator(this, GPUMaskType::AllGPU())
{
	for (uint32 HeapType = 0; HeapType < (uint32)DescriptorHeapType::Count; ++HeapType)
	{
		OfflineDescriptorManagers.emplace_back(this, (DescriptorHeapType)HeapType);
	}

	for (uint32 Type = 0; Type < (uint32)QueueType::Count; ++Type)
	{
		Queues.emplace_back(this, (QueueType)Type);
	}
}

void NodeDevice::Initialize()
{
	SetupAfterDeviceCreation();
}

ID3D12Device* NodeDevice::GetDevice()
{
	return GetParentAdapter()->GetDevice();
}

ID3D12Device5* NodeDevice::GetRayTracingDevice()
{
	return GetParentAdapter()->GetRayTracingDevice();
}

void platform_ex::Windows::D3D12::NodeDevice::CreateSamplerInternal(const D3D12_SAMPLER_DESC& Desc, D3D12_CPU_DESCRIPTOR_HANDLE Descriptor)
{
	GetDevice()->CreateSampler(&Desc, Descriptor);
}

int32 GGlobalResourceDescriptorHeapSize = 1000 * 1000;
int32 GGlobalSamplerDescriptorHeapSize = 2048;


int32 GOnlineDescriptorHeapSize = 500 * 1000;
int32 GOnlineDescriptorHeapBlockSize = 2000;

void NodeDevice::SetupAfterDeviceCreation()
{
	for (auto& Queue : Queues)
	{
		Queue.SetupAfterDeviceCreation();
	}

	ID3D12Device* Direct3DDevice = GetParentAdapter()->GetDevice();

	NormalDescriptorHeapManager.Init(GGlobalResourceDescriptorHeapSize, GGlobalSamplerDescriptorHeapSize);

	GlobalSamplerHeap.Init(NUM_SAMPLER_DESCRIPTORS);

	// This value can be tuned on a per app basis. I.e. most apps will never run into descriptor heap pressure so
	// can make this global heap smaller
	uint32 NumGlobalViewDesc = GGlobalViewHeapSize;

	const D3D12_RESOURCE_BINDING_TIER Tier = GetParentAdapter()->GetResourceBindingTier();
	uint32 MaximumSupportedHeapSize = NUM_VIEW_DESCRIPTORS_TIER_1;

	switch (Tier)
	{
	case D3D12_RESOURCE_BINDING_TIER_1:
		MaximumSupportedHeapSize = NUM_VIEW_DESCRIPTORS_TIER_1;
		break;
	case D3D12_RESOURCE_BINDING_TIER_2:
		MaximumSupportedHeapSize = NUM_VIEW_DESCRIPTORS_TIER_2;
		break;
	case D3D12_RESOURCE_BINDING_TIER_3:
	default:
		MaximumSupportedHeapSize = NUM_VIEW_DESCRIPTORS_TIER_3;
		break;
	}
	wconstraint(NumGlobalViewDesc <= MaximumSupportedHeapSize);

	OnlineDescriptorManager.Init(GOnlineDescriptorHeapSize, GOnlineDescriptorHeapBlockSize);

	CreateDefaultViews();

	ImmediateCommandContext = new CommandContext(this,QueueType::Direct, true);
}

void NodeDevice::CreateDefaultViews()
{
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc{};
		SRVDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.Texture2D.MipLevels = 1;
		SRVDesc.Texture2D.MostDetailedMip = 0;
		SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;

		DefaultViews.NullSRV = GetOfflineDescriptorManager(DescriptorHeapType::Standard).AllocateHeapSlot();
		GetDevice()->CreateShaderResourceView(nullptr, &SRVDesc, DefaultViews.NullSRV);
	}

	{
		D3D12_RENDER_TARGET_VIEW_DESC RTVDesc{};
		RTVDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		RTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		RTVDesc.Texture2D.MipSlice = 0;

		DefaultViews.NullRTV = GetOfflineDescriptorManager(DescriptorHeapType::RenderTarget).AllocateHeapSlot();
		GetDevice()->CreateRenderTargetView(nullptr, &RTVDesc, DefaultViews.NullRTV);
	}

	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc{};
		UAVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		UAVDesc.Texture2D.MipSlice = 0;

		DefaultViews.NullUAV = GetOfflineDescriptorManager(DescriptorHeapType::Standard).AllocateHeapSlot();
		GetDevice()->CreateUnorderedAccessView(nullptr, nullptr, &UAVDesc, DefaultViews.NullUAV);
	}

	{
		D3D12_DEPTH_STENCIL_VIEW_DESC DSVDesc{};
		DSVDesc.Format = DXGI_FORMAT_D32_FLOAT;
		DSVDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		DSVDesc.Texture2D.MipSlice = 0;

		DefaultViews.NullDSV = GetOfflineDescriptorManager(DescriptorHeapType::DepthStencil).AllocateHeapSlot();
		GetDevice()->CreateDepthStencilView(nullptr, &DSVDesc, DefaultViews.NullDSV);
	}

	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc{};

		DefaultViews.NullCBV = GetOfflineDescriptorManager(DescriptorHeapType::Standard).AllocateHeapSlot();
		GetDevice()->CreateConstantBufferView(&CBVDesc, DefaultViews.NullCBV);
	}

	{
		D3D12_SAMPLER_DESC desc;
		desc.AddressU = desc.AddressV = desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		desc.MipLODBias = 0;
		desc.MaxAnisotropy = 1;
		desc.MinLOD = 0;
		desc.MaxLOD = FLT_MAX;
		desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		desc.BorderColor[0] = 0;
		desc.BorderColor[1] = 0;
		desc.BorderColor[2] = 0;
		desc.BorderColor[3] = 0;
		desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;

		DefaultViews.DefaultSampler = CreateSampler(desc);

		// The default sampler must have ID=0
		// FD3D12DescriptorCache::SetSamplers relies on this
		wassume(DefaultViews.DefaultSampler->ID == 0);
	}
}

CommandAllocator* NodeDevice::ObtainCommandAllocator(QueueType Type)
{
	auto Allocator = Queues[(uint32)Type].ObjectPool.Allocators.Pop();
	if (!Allocator)
	{
		Allocator = new CommandAllocator(this, Type);
	}

	return Allocator;
}

void NodeDevice::ReleaseCommandAllocator(CommandAllocator* Allocator)
{
	Allocator->Reset();

	Queues[(uint32)Allocator->Type].ObjectPool.Allocators.Push(Allocator);
}

CommandList* platform_ex::Windows::D3D12::NodeDevice::ObtainCommandList(CommandAllocator* CommandAllocator)
{
	auto* List = Queues[(uint32)CommandAllocator->Type].ObjectPool.Lists.Pop();
	if (!List)
	{
		List = new CommandList(CommandAllocator);
	}
	else
	{
		List->Reset(CommandAllocator);
	}

	return List;
}

void platform_ex::Windows::D3D12::NodeDevice::ReleaseCommandList(CommandList* Cmd)
{
	Queues[(uint32)Cmd->Type].ObjectPool.Lists.Push(Cmd);
}

platform_ex::Windows::D3D12::NodeQueue::NodeQueue(NodeDevice* Device, QueueType QueueType)
	:Device(Device),Fence(Device->GetParentAdapter(),0,Device->GetGPUIndex()),Type(QueueType)
{
}

platform_ex::Windows::D3D12::NodeQueue::~NodeQueue()
{
}

void platform_ex::Windows::D3D12::NodeQueue::SetupAfterDeviceCreation()
{
}

