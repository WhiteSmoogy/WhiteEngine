#include "View.h"
#include "GraphicsBuffer.hpp"
#include "Adapter.h"
#include "Convert.h"
#include "NodeDevice.h"

using namespace platform_ex::Windows::D3D12;
using platform::Render::shared_raw_robject;

D3DView::D3DView(NodeDevice* InDevice, DescriptorHeapType InHeapType)
	:DeviceChild(InDevice),
	OfflineHandle(InDevice->GetOfflineDescriptorManager(InHeapType).AllocateHeapSlot())
	, HeapType(InHeapType)
{
}

D3DView::~D3DView()
{
	GetParentDevice()->GetOfflineDescriptorManager(HeapType).FreeHeapSlot(OfflineHandle);
}

void D3DView::CreateView(BaseShaderResource* InResource, NullDescPtr NullDescriptor)
{
	ShaderResource = InResource;
	Resource = InResource->GetResource();
	Location = &InResource->Location;

	ViewSubset.Layout = Resource->GetDesc();
	UpdateDescriptor();
}

void D3DView::CreateView(ResourceLocation* InResource, NullDescPtr NullDescriptor)
{
	ShaderResource = nullptr;
	Resource = InResource->GetResource();
	Location = InResource;

	ViewSubset.Layout = Resource->GetDesc();
	UpdateDescriptor();
}

RenderTargetView::RenderTargetView(NodeDevice* InDevice)
	:TView(InDevice, DescriptorHeapType::RenderTarget)
{
}

void RenderTargetView::UpdateDescriptor()
{
	GetParentDevice()->GetDevice()->CreateRenderTargetView(
		Resource->Resource(),
		&Desc,
		OfflineHandle
	);

	OfflineHandle.IncrementVersion();
}

DepthStencilView::DepthStencilView(NodeDevice* InDevice)
	:TView(InDevice, DescriptorHeapType::DepthStencil)
{
}

void DepthStencilView::UpdateDescriptor()
{
	GetParentDevice()->GetDevice()->CreateDepthStencilView(
		Resource->Resource(),
		&Desc,
		OfflineHandle
	);

	OfflineHandle.IncrementVersion();
}

ShaderResourceView::ShaderResourceView(NodeDevice* InDevice)
	:TView(InDevice, DescriptorHeapType::Standard)
{
}



template<typename TResource>
void ShaderResourceView::CreateView(TResource InResource, D3D12_SHADER_RESOURCE_VIEW_DESC const& InD3DViewDesc, EFlags InFlags)
{
	OffsetInBytes = 0;
	StrideInBytes = 0;
	Flags = InFlags;

	//
	// Buffer / acceleration structure views can apply an offset in bytes from the start of the logical resource.
	//
	// Reconstruct this value and store it for later. We'll need it if the view is renamed, to determine where
	// the view should exist within the bounds of the new resource location.
	//
	if (InD3DViewDesc.ViewDimension == D3D12_SRV_DIMENSION_BUFFER)
	{
		StrideInBytes = InD3DViewDesc.Format == DXGI_FORMAT_UNKNOWN
			? InD3DViewDesc.Buffer.StructureByteStride
			: GetFormatSizeInBytes(InD3DViewDesc.Format);

		wassume(StrideInBytes > 0);

		OffsetInBytes = (InD3DViewDesc.Buffer.FirstElement * StrideInBytes) - InResource->GetOffsetFromBaseOfResource();
		wassume((OffsetInBytes % StrideInBytes) == 0);
	}
	else if (InD3DViewDesc.ViewDimension == D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE)
	{
		OffsetInBytes = InD3DViewDesc.RaytracingAccelerationStructure.Location - InResource->GetGPUVirtualAddress();
		StrideInBytes = 1;
	}

	TView::CreateView(InD3DViewDesc, InResource);
}

template<>
void ShaderResourceView::CreateView<BaseShaderResource*>(BaseShaderResource* InResource, D3D12_SHADER_RESOURCE_VIEW_DESC const& InD3DViewDesc, EFlags InFlags);
template<>
void ShaderResourceView::CreateView<ResourceLocation*>(ResourceLocation* InResource, D3D12_SHADER_RESOURCE_VIEW_DESC const& InD3DViewDesc, EFlags InFlags);

void ShaderResourceView::UpdateMinLODClamp(float MinLODClamp)
{
	switch (Desc.ViewDimension)
	{
	default: throw white::unsupported(); return; // not supported
	case D3D12_SRV_DIMENSION_TEXTURE2D: Desc.Texture2D.ResourceMinLODClamp = MinLODClamp; break;
	case D3D12_SRV_DIMENSION_TEXTURE2DARRAY: Desc.Texture2DArray.ResourceMinLODClamp = MinLODClamp; break;
	case D3D12_SRV_DIMENSION_TEXTURE3D: Desc.Texture3D.ResourceMinLODClamp = MinLODClamp; break;
	case D3D12_SRV_DIMENSION_TEXTURECUBE: Desc.TextureCube.ResourceMinLODClamp = MinLODClamp; break;
	case D3D12_SRV_DIMENSION_TEXTURECUBEARRAY: Desc.TextureCubeArray.ResourceMinLODClamp = MinLODClamp; break;
	}

	UpdateDescriptor();
}

void ShaderResourceView::UpdateDescriptor()
{
	// NOTE (from D3D Debug runtime): pResource must be NULL for acceleration structures, since the resource location comes from a GPUVA in pDesc.
	ID3D12Resource* TargetResource = Desc.ViewDimension != D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE
		? GetResource()->Resource()
		: nullptr;

	GetParentDevice()->GetDevice()->CreateShaderResourceView(
		TargetResource,
		&Desc,
		OfflineHandle
	);

	OfflineHandle.IncrementVersion();
}

UnorderedAccessView::UnorderedAccessView(NodeDevice* InDevice)
	:TView(InDevice, DescriptorHeapType::Standard)
{
}

template<typename TResource>
void UnorderedAccessView::CreateView(TResource InResource, D3D12_UNORDERED_ACCESS_VIEW_DESC const& InD3DViewDesc, EFlags InFlags)
{
	OffsetInBytes = 0;
	StrideInBytes = 0;

	//
	// Buffer views can apply an offset in bytes from the start of the logical resource.
	//
	// Reconstruct this value and store it for later. We'll need it if the view is renamed, 
	// to determine where the view should exist within the bounds of the new resource location.
	//
	if (InD3DViewDesc.ViewDimension == D3D12_UAV_DIMENSION_BUFFER)
	{
		StrideInBytes = InD3DViewDesc.Format == DXGI_FORMAT_UNKNOWN
			? InD3DViewDesc.Buffer.StructureByteStride
			: GetFormatSizeInBytes(InD3DViewDesc.Format);

		wassume(StrideInBytes > 0);

		OffsetInBytes = (InD3DViewDesc.Buffer.FirstElement * StrideInBytes) - InResource->GetOffsetFromBaseOfResource();
		wassume((OffsetInBytes % StrideInBytes) == 0);
	}

	//
	// UAVs optionally support a hidden counter. D3D12 requires the user to allocate this as a buffer resource.
	//
	if (white::has_anyflags(InFlags, EFlags::NeedsCounter))
	{
		auto Device = GetParentDevice();
		const auto Node = Device->GetGPUMask();

		Device->GetParentAdapter()->CreateBuffer(
			CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT, Node.GetNative(), Node.GetNative())
			, Node
			, D3D12_RESOURCE_STATE_UNORDERED_ACCESS
			//, ResourceStateMode::Multi
			, D3D12_RESOURCE_STATE_UNORDERED_ACCESS
			, 4
			, CounterResource.ReleaseAndGetAddress()
			, "Counter"
			, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
		);
	}

	TView::CreateView(InD3DViewDesc, InResource);
}

void UnorderedAccessView::UpdateDescriptor()
{
	ID3D12Resource* Resource = this->Resource->Resource();

	ID3D12Resource* Counter = CounterResource
		? CounterResource->Resource()
		: nullptr;

	GetParentDevice()->GetDevice()->CreateUnorderedAccessView(
		Resource,
		Counter,
		&Desc,
		OfflineHandle
	);

	OfflineHandle.IncrementVersion();
}

SRVRIRef Device::CreateShaderResourceView(const platform::Render::GraphicsBuffer* InBuffer, EFormat format)
{
	auto Resource = static_cast<GraphicsBuffer*>(const_cast<platform::Render::GraphicsBuffer*>(InBuffer));
	auto access = InBuffer->GetAccess();
	auto usage = InBuffer->GetUsage();

	uint32 StartOffsetBytes = 0;
	uint32 NumElements = -1;

	wconstraint(white::has_anyflags(access, EAccessHint::EA_GPURead) || white::has_allflags(access, EAccessHint::EA_AccelerationStructure));

	auto CreateShaderResourceView = [&]<typename CommandType>()
	{
		auto srv = new ShaderResourceView(Resource->GetParentDevice());

		auto& CmdList = platform::Render::CommandListExecutor::GetImmediateCommandList();

		if (!CmdList.IsExecuting() && white::has_anyflags(usage, Buffer::Dynamic))
		{
			CL_ALLOC_COMMAND(CmdList, CommandType) { Resource, srv, StartOffsetBytes, NumElements, format };
		}
		else
		{
			CommandType Command{ Resource,srv,StartOffsetBytes,NumElements, format };
			Command.ExecuteNoCmdList();
		}

		return shared_raw_robject(srv);
	};

	//TODO:switch case
	if (white::has_allflags(access, EAccessHint::EA_AccelerationStructure))
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};

		SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.RaytracingAccelerationStructure.Location = Resource->Location.GetGPUVirtualAddress() + StartOffsetBytes;

		auto srv = new ShaderResourceView(GetNodeDevice(0));
		srv->CreateView(Resource, SRVDesc, ShaderResourceView::EFlags::None);
		return shared_raw_robject(srv);
	}
	if (!white::has_anyflags(access, EAccessHint::EA_GPUStructured))
	{
		struct D3D12InitializeBufferSRVCommand final : platform::Render::CommandBase
		{
			GraphicsBuffer* Buffer;
			ShaderResourceView* SRV;
			uint32 StartOffsetBytes;
			uint32 NumElements;
			DXGI_FORMAT Format;

			D3D12InitializeBufferSRVCommand(GraphicsBuffer* InBuffer, ShaderResourceView* InSrv, uint32 InStartOffsetBytes, uint32 InNumElements, EFormat InFormat)
				:Buffer(InBuffer), SRV(InSrv), StartOffsetBytes(InStartOffsetBytes), NumElements(InNumElements), Format(Buffer->GetFormat())
			{}

			void ExecuteAndDestruct(platform::Render::CommandListBase& CmdList, platform::Render::CommandListContext& Context)
			{
				ExecuteNoCmdList();
				this->~D3D12InitializeBufferSRVCommand();
			}

			void ExecuteNoCmdList()
			{
				auto& Location = Buffer->Location;
				const auto Stride = Buffer->Stride;
				const auto access = Buffer->GetAccess();
				const uint32 BufferSize = Buffer->size_in_byte;

				uint32 CreationStride = Stride;

				if (CreationStride == 0)
				{
					if (white::has_anyflags(access, EAccessHint::EA_Raw))
						CreationStride = 4;
					else
					{
						CreationStride = NumFormatBytes(Convert(Format));
					}
				}

				const auto BufferOffset = Location.GetOffsetFromBaseOfResource();
				const uint64 NumRequestedBytes = NumElements * CreationStride;

				const uint32 OffsetBytes = std::min(StartOffsetBytes, BufferSize);
				const uint32 NumBytes = static_cast<uint32>(std::min<uint64>(NumRequestedBytes, BufferSize - OffsetBytes));

				D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc{};

				SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
				SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;

				if (white::has_anyflags(access, EAccessHint::EA_Raw))
				{
					SRVDesc.Format = DXGI_FORMAT_R32_TYPELESS;
					SRVDesc.Buffer.Flags |= D3D12_BUFFER_SRV_FLAG_RAW;
				}
				else
				{
					SRVDesc.Format = Format;
				}

				SRVDesc.Buffer.FirstElement = (BufferOffset) / CreationStride;
				SRVDesc.Buffer.NumElements = NumBytes / CreationStride;

				SRV->CreateView(Buffer, SRVDesc, ShaderResourceView::EFlags::None);
			}
		};

		return CreateShaderResourceView.operator() < D3D12InitializeBufferSRVCommand > ();
	}
	else {

		struct D3D12InitializeStructuredBufferSRVCommand final : platform::Render::CommandBase
		{
			GraphicsBuffer* StructuredBuffer;
			ShaderResourceView* SRV;
			uint32 StartOffsetBytes;
			uint32 NumElements;
			EFormat Format;

			D3D12InitializeStructuredBufferSRVCommand(GraphicsBuffer* InBuffer, ShaderResourceView* InSrv, uint32 InStartOffsetBytes, uint32 InNumElements, EFormat InFormat)
				:StructuredBuffer(InBuffer), SRV(InSrv), StartOffsetBytes(InStartOffsetBytes), NumElements(InNumElements), Format(InFormat)
			{}

			void ExecuteAndDestruct(platform::Render::CommandListBase& CmdList, platform::Render::CommandListContext& Context)
			{
				ExecuteNoCmdList();
				this->~D3D12InitializeStructuredBufferSRVCommand();
			}

			void ExecuteNoCmdList()
			{
				auto& Location = StructuredBuffer->Location;

				const uint64 Offset = Location.GetOffsetFromBaseOfResource();
				const D3D12_RESOURCE_DESC& BufferDesc = Location.GetResource()->GetDesc();

				const bool bByteAccessBuffer = white::has_anyflags(StructuredBuffer->access, EAccessHint::EA_Raw);
				// Create a Shader Resource View
				D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
				SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

				uint32 EffectiveStride = StructuredBuffer->Stride;
				SRVDesc.Format = DXGI_FORMAT_UNKNOWN;

				if (bByteAccessBuffer)
				{
					SRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
					EffectiveStride = 4;
					SRVDesc.Format = DXGI_FORMAT_R32_TYPELESS;
				}
				else
				{
					SRVDesc.Buffer.StructureByteStride = EffectiveStride;
				}

				uint32 MaxElements = static_cast<uint32>(Location.GetSize() / EffectiveStride);
				StartOffsetBytes = std::min<uint32>(StartOffsetBytes, static_cast<uint32>(Location.GetSize()));
				uint32 StartElement = StartOffsetBytes / EffectiveStride;


				SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
				SRVDesc.Buffer.NumElements = std::min<uint32>(MaxElements - StartElement, NumElements);
				SRVDesc.Buffer.FirstElement = Offset / EffectiveStride + StartElement;

				SRV->CreateView(StructuredBuffer, SRVDesc, ShaderResourceView::EFlags::None);
			}
		};

		return CreateShaderResourceView.operator() < D3D12InitializeStructuredBufferSRVCommand > ();
	}
}

UAVRIRef Device::CreateUnorderedAccessView(const platform::Render::GraphicsBuffer* InBuffer, EFormat format)
{
	auto Buffer = static_cast<GraphicsBuffer*>(const_cast<platform::Render::GraphicsBuffer*>(InBuffer));
	auto& Location = Buffer->Location;

	D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc{};
	UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;

	uint32 EffectiveStride = Buffer->Stride;

	const bool bByteAccessBuffer = white::has_anyflags(Buffer->GetAccess(), EAccessHint::EA_Raw);

	if (bByteAccessBuffer)
	{
		UAVDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		UAVDesc.Buffer.Flags |= D3D12_BUFFER_UAV_FLAG_RAW;
		EffectiveStride = 4;
	}
	else if (white::has_anyflags(Buffer->GetAccess(), EAccessHint::EA_GPUStructured))
	{
		UAVDesc.Format = DXGI_FORMAT_UNKNOWN;
		EffectiveStride = Buffer->Stride;
	}
	else
	{
		UAVDesc.Format = FindUnorderedAccessDXGIFormat(Convert(format));
		EffectiveStride = NumFormatBytes(format);
	}

	UAVDesc.Buffer.FirstElement = Location.GetOffsetFromBaseOfResource() / EffectiveStride;
	UAVDesc.Buffer.NumElements = Location.GetSize() / EffectiveStride;
	UAVDesc.Buffer.StructureByteStride = !bByteAccessBuffer ? EffectiveStride : 0;

	auto uav = new UnorderedAccessView(Buffer->GetParentDevice());

	uav->CreateView(Buffer, UAVDesc, UnorderedAccessView::EFlags::None);

	return shared_raw_robject(uav);
}
