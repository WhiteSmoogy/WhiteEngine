#include "GraphicsBuffer.hpp"
#include "Context.h"
#include "Convert.h"
#include "View.h"
#include "NodeDevice.h"
#include "../ICommandList.h"
#include "System/SystemEnvironment.h"

namespace platform_ex::Windows::D3D12 {
	GraphicsBuffer::GraphicsBuffer(NodeDevice* Parent, Buffer::Usage usage,
		uint32_t access_hint, uint32_t size_in_byte,DXGI_FORMAT InFormat)
		:platform::Render::GraphicsBuffer(usage, access_hint, size_in_byte)
		, DeviceChild(Parent)
		, BaseShaderResource(Parent)
		, format(InFormat)
	{
	}

	GraphicsBuffer::~GraphicsBuffer() {
	}

	void GraphicsBuffer::CopyToBuffer(platform::Render::GraphicsBuffer& rhs_)
	{
		auto& rhs = static_cast<GraphicsBuffer&>(rhs_);

		D3D12_RESOURCE_BARRIER src_barrier_before, dst_barrier_before;
		src_barrier_before.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		src_barrier_before.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		D3D12_HEAP_TYPE src_heap_type;

		if (access == EAccessHint::EA_CPURead) {
			src_barrier_before.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
			src_heap_type = D3D12_HEAP_TYPE_READBACK;
		}
		else if ((rhs.GetAccess() & EAccessHint::EA_CPURead) || (rhs.GetAccess() & EAccessHint::EA_CPUWrite)) {
			src_barrier_before.Transition.StateBefore = D3D12_RESOURCE_STATE_GENERIC_READ;
			src_heap_type = D3D12_HEAP_TYPE_UPLOAD;
		}
		else {
			src_barrier_before.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
			src_heap_type = D3D12_HEAP_TYPE_DEFAULT;
		}
		src_barrier_before.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
		src_barrier_before.Transition.pResource = Location.GetResource()->Resource();
		src_barrier_before.Transition.Subresource = 0;

		dst_barrier_before.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		dst_barrier_before.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		D3D12_HEAP_TYPE dst_heap_type;
		if (rhs.GetAccess() == EAccessHint::EA_CPURead) {
			src_barrier_before.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
			src_heap_type = D3D12_HEAP_TYPE_READBACK;
		}
		else if ((rhs.GetAccess() & EAccessHint::EA_CPURead) || (rhs.GetAccess() & EAccessHint::EA_CPUWrite)) {
			dst_barrier_before.Transition.StateBefore = D3D12_RESOURCE_STATE_GENERIC_READ;
			dst_heap_type = D3D12_HEAP_TYPE_UPLOAD;
		}
		else {
			dst_barrier_before.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
			dst_heap_type = D3D12_HEAP_TYPE_DEFAULT;
		}
		dst_barrier_before.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
		dst_barrier_before.Transition.pResource = rhs.Location.GetResource()->Resource();
		dst_barrier_before.Transition.Subresource = 0;

		int n = 0;
		D3D12_RESOURCE_BARRIER barrier_before[2];
		if (src_heap_type != dst_heap_type) {
			if (D3D12_HEAP_TYPE_DEFAULT == src_heap_type)
				barrier_before[n++] = src_barrier_before;
			if (D3D12_HEAP_TYPE_DEFAULT == dst_heap_type)
				barrier_before[n++] = dst_barrier_before;
		}

		auto& cmd_list = Context::Instance().GetCommandList(Device::Command_Resource);
		if (n > 0)
			cmd_list->ResourceBarrier(n, barrier_before);

		cmd_list->CopyBufferRegion(rhs.Location.GetResource()->Resource(), 0, Location.GetResource()->Resource(), 0, size_in_byte);

		D3D12_RESOURCE_BARRIER barrier_after[2] = { barrier_before[0],barrier_before[1] };
		std::swap(barrier_after[0].Transition.StateBefore, barrier_after[0].Transition.StateAfter);
		std::swap(barrier_after[1].Transition.StateBefore, barrier_after[1].Transition.StateAfter);

		if (n > 0)
			cmd_list->ResourceBarrier(n, barrier_after);
	}

	std::pair<D3D12_RESOURCE_DESC, uint32> GetResourceDescAndAlignment(uint64 InSize, uint32 InStride, uint32 InAccess)
	{
		D3D12_RESOURCE_DESC ResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(InSize);

		if (InAccess & EA_GPUUnordered)
		{
			ResourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}

		if ((InAccess & EA_SRV) == 0)
		{
			ResourceDesc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
		}

		if (InAccess & EA_DrawIndirect)
		{
			ResourceDesc.Flags |= D3D12_RESOURCE_FLAG_NONE;
		}

		// Structured buffers, non-ByteAddress buffers, need to be aligned to their stride to ensure that they can be addressed correctly with element based offsets.
		uint32 Alignment = (InStride > 0) && (((InAccess & EA_GPUStructured) != 0) || ((InAccess & (EA_Raw | EA_DrawIndirect)) == 0)) ? InStride : 4;

		return { ResourceDesc,Alignment };
	}

	struct D3D12CommandInitializeBuffer final : platform::Render::CommandBase
	{
		GraphicsBuffer* Buffer;
		ResourceLocation SrcResourceLoc;
		uint32 Size;
		D3D12_RESOURCE_STATES DestinationState;

		D3D12CommandInitializeBuffer(GraphicsBuffer* InBuffer, ResourceLocation& InSrcResourceLoc, uint32 InSize, D3D12_RESOURCE_STATES InDestinationState)
			: Buffer(InBuffer)
			, SrcResourceLoc(InSrcResourceLoc.GetParentDevice())
			, Size(InSize)
			, DestinationState(InDestinationState)
		{
			ResourceLocation::TransferOwnership(SrcResourceLoc, InSrcResourceLoc);
		}

		void ExecuteAndDestruct(platform::Render::CommandListBase& CmdList, platform::Render::CommandListContext& Context)
		{
			ExecuteNoCmdList();
			this->~D3D12CommandInitializeBuffer();
		}

		void ExecuteNoCmdList()
		{
			auto CurrentBuffer = Buffer;

			ResourceHolder* Destination = CurrentBuffer->Location.GetResource();
			auto Device = CurrentBuffer->Location.GetParentDevice();

			auto& CommandContext = Device->GetDefaultCommandContext();
			auto& hCommandList = CommandContext.CommandListHandle;
			// Copy from the temporary upload heap to the default resource
			{
				++CommandContext.numInitialResourceCopies;

				//could have been suballocated from shared resource  - not very optimal and should be batched
				if (Destination->GetResourceState() != D3D12_RESOURCE_STATE_COPY_DEST)
				{
					hCommandList.AddTransitionBarrier(Destination, Destination->GetResourceState(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
				}

				hCommandList.FlushResourceBarriers();
				hCommandList->CopyBufferRegion(
					Destination->Resource(),
					CurrentBuffer->Location.GetOffsetFromBaseOfResource(),
					SrcResourceLoc.GetResource()->Resource(),
					SrcResourceLoc.GetOffsetFromBaseOfResource(), Size);

				// Update the resource state after the copy has been done (will take care of updating the residency as well)
				if (DestinationState != D3D12_RESOURCE_STATE_COPY_DEST)
				{
					hCommandList.AddTransitionBarrier(Destination, D3D12_RESOURCE_STATE_COPY_DEST, DestinationState, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
				}

				CommandContext.ConditionalFlushCommandList();
			}

			// Buffer is now written and ready, so unlock the block (locked after creation and can be defragmented if needed)
			CurrentBuffer->Location.UnlockPoolData();
		}
	};

	GraphicsBuffer* Device::CreateBuffer(platform::Render::CommandListImmediate* Cmdlist, Buffer::Usage usage, white::uint32 access, uint32 Size, uint32 Stride, DXGI_FORMAT format, ResourceCreateInfo& CreateInfo)
	{
		IResourceAllocator* Allocator = nullptr;

		auto [Desc, Alignment] = GetResourceDescAndAlignment(Size, Stride, access);
		Desc.Format = format;

		return CreateBuffer<ResourceStateMode::Default>(Cmdlist, Desc, usage, access, Alignment, Stride, Size, CreateInfo, Allocator);
	}
	
	const char* GetDebugBufferName(Buffer::Usage usage, white::uint32 access)
	{
		if (white::has_anyflags(access, EAccessHint::EA_GPUStructured))
			return "StructBuffer";
		return "Buffer";
	}

	GraphicsBuffer* Device::CreateBuffer(Buffer::Usage usage, white::uint32 access, uint32 Size, uint32 Stride, std::optional<void const*> init_data)
	{
		auto CreateInfo = FillResourceCreateInfo(init_data, GetDebugBufferName(usage,access));

		return CreateBuffer(&platform::Render::CommandListExecutor::GetImmediateCommandList(), usage, access, Size, Stride, DXGI_FORMAT_UNKNOWN, CreateInfo);
	}

	template<ResourceStateMode Mode>
	GraphicsBuffer* platform_ex::Windows::D3D12::Device::CreateBuffer(platform::Render::CommandListImmediate* Cmdlist, const D3D12_RESOURCE_DESC& Desc, 
		Buffer::Usage usage, white::uint32 access,
		uint32 Alignment, uint32 Stride, uint64 InSize, ResourceCreateInfo& CreateInfo,
		IResourceAllocator* Allocator)
	{
		wconstraint(InSize <= std::numeric_limits<uint32>::max());
		auto Size = static_cast<uint32>(InSize);

		auto device = GetNodeDevice(CreateInfo.GPUIndex);

		//WithoutNativeResource
		if (CreateInfo.WithoutNativeResource)
			return new GraphicsBuffer(device, usage, access, Size, Desc.Format);

		//CommonCreateBuffer
		const bool bIsDynamic = (usage & Buffer::Usage::Dynamic) ? true : false;

		//remove EA_CPUWrite/EA_CPURead?
		D3D12_HEAP_TYPE HeapType = bIsDynamic ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_DEFAULT;

		const D3D12_RESOURCE_STATES InitialState = BufferAllocator::GetDefaultInitialResourceState<ResourceStateMode::Default>(HeapType, access);

		GraphicsBuffer* BufferOut = nullptr;
		if (bIsDynamic)
		{
			BufferOut = new GraphicsBuffer(device, usage, access, Size,Desc.Format);
			BufferOut->Alignment = Alignment;
			BufferOut->Stride = Stride;

			AllocateBuffer<Mode>(GetNodeDevice(0), Desc, Size, usage, access, InitialState, CreateInfo, Alignment, BufferOut, BufferOut->Location, Allocator);
		}
		else
		{
			const D3D12_RESOURCE_STATES CreateState = CreateInfo.ResouceData != nullptr ? D3D12_RESOURCE_STATE_COPY_DEST : InitialState;

			BufferOut = new GraphicsBuffer(device, usage, access, Size, Desc.Format);
			BufferOut->Alignment = Alignment;
			BufferOut->Stride = Stride;

			AllocateBuffer<Mode>(GetNodeDevice(0), Desc, Size, usage, access, CreateState, CreateInfo, Alignment, BufferOut, BufferOut->Location, Allocator);

			if (CreateInfo.ResouceData == nullptr)
			{
				BufferOut->Location.UnlockPoolData();
			}
		}

		if (CreateInfo.ResouceData != nullptr)
		{
			if (!bIsDynamic && BufferOut->Location.IsValid())
			{
				const bool bOnAsyncThread = !Environment->Scheduler->is_render_schedule();

				ResourceLocation SrcResourceLoc(BufferOut->GetParentDevice());
				void* pData;
				if (bOnAsyncThread)
				{
					const uint32 GPUIdx = SrcResourceLoc.GetParentDevice()->GetGPUIndex();
					pData = GetUploadHeapAllocator(GPUIdx).AllocUploadResource(Size, 4u, SrcResourceLoc);
				}
				else
				{
					pData = SrcResourceLoc.GetParentDevice()->GetDefaultFastAllocator().Allocate(Size, 4UL, &SrcResourceLoc);
				}
				wconstraint(pData);
				std::memcpy(pData, CreateInfo.ResouceData, Size);

				if (bOnAsyncThread)
				{
					new (Cmdlist->AllocCommand<D3D12CommandInitializeBuffer>()) D3D12CommandInitializeBuffer(BufferOut, SrcResourceLoc, Size, InitialState);
				}
				else
				{
					D3D12CommandInitializeBuffer Command(BufferOut, SrcResourceLoc, Size, InitialState);
					Command.ExecuteNoCmdList();
				}
			}
		}

		return BufferOut;
	}

	template
	GraphicsBuffer* platform_ex::Windows::D3D12::Device::CreateBuffer<ResourceStateMode::Single>(platform::Render::CommandListImmediate* Cmdlist, const D3D12_RESOURCE_DESC& Desc,
			Buffer::Usage usage, white::uint32 access,
			uint32 Alignment, uint32 Stride, uint64 InSize, ResourceCreateInfo& CreateInfo,
			IResourceAllocator* Allocator);

	template<ResourceStateMode Mode>
	void Device::AllocateBuffer(NodeDevice* Device, const D3D12_RESOURCE_DESC& InDesc, 
		uint32 Size, uint32 InUsage, uint32 InAccess,
		D3D12_RESOURCE_STATES InCreateState,
		ResourceCreateInfo& CreateInfo,
		uint32 Alignment, GraphicsBuffer* Buffer,
		ResourceLocation& ResourceLocation, IResourceAllocator* ResourceAllocator)
	{
		const bool bIsDynamic = (InUsage & Buffer::Usage::Dynamic) ? true : false;

		if (bIsDynamic)
		{
			wconstraint(ResourceAllocator == nullptr);
			wconstraint(InCreateState == D3D12_RESOURCE_STATE_GENERIC_READ);
			void* pData = GetUploadHeapAllocator(Device->GetGPUIndex()).AllocUploadResource(Size, Alignment, ResourceLocation);
			wconstraint(ResourceLocation.GetSize() == Size);

			if (CreateInfo.ResouceData)
			{
				// Handle initial data
				std::memcpy(pData, CreateInfo.ResouceData, Size);
			}
		}
		else
		{
			if (ResourceAllocator)
			{
				throw white::unimplemented();
			}
			else
			{
				Device->GetDefaultBufferAllocator().AllocDefaultResource<Mode>(D3D12_HEAP_TYPE_DEFAULT, InDesc, InAccess, InCreateState, ResourceLocation, Alignment, CreateInfo.DebugName);
			}
		}
	}

	void GraphicsBuffer::UpdateSubresource(white::uint32 offset, white::uint32 size, void const* data)
	{
	}

	ID3D12Resource* GraphicsBuffer::UploadResource() const
	{
		return nullptr;
	}
	RenderTargetView* GraphicsBuffer::RetriveRenderTargetView(uint16 width, uint16 height, platform::Render::EFormat pf)
	{
		if (!rtv_maps)
			rtv_maps = std::make_unique<std::unordered_map<std::size_t, std::unique_ptr<RenderTargetView>>>();

		auto key = hash_combine_seq(0, width, height, pf);
		auto iter = rtv_maps->find(key);
		if (iter != rtv_maps->end())
			return iter->second.get();

		D3D12_RENDER_TARGET_VIEW_DESC desc;
		desc.Format = Convert(pf);
		desc.ViewDimension = D3D12_RTV_DIMENSION_BUFFER;
		desc.Buffer.FirstElement = 0;
		desc.Buffer.NumElements = std::min<UINT>(width * height, GetSize() / NumFormatBytes(pf));

		return rtv_maps->emplace(key, std::make_unique<RenderTargetView>(GetDefaultNodeDevice(), desc, this)).first->second.get();
	}
	ShaderResourceView* GraphicsBuffer::RetriveShaderResourceView()
	{
		uint32 StartOffsetBytes = 0;
		uint32 NumElements = -1;

		if (!srv)
		{
			wconstraint(white::has_anyflags(access, EAccessHint::EA_GPURead) || white::has_allflags(access, EAccessHint::EA_AccelerationStructure));
			
			auto CreateShaderResourceView = [&]<typename CommandType>()
			{
				srv = std::make_unique<ShaderResourceView>(GetParentDevice());

				auto& CmdList = platform::Render::CommandListExecutor::GetImmediateCommandList();

				if (!CmdList.IsExecuting() && white::has_anyflags(usage, Buffer::Dynamic))
				{
					CL_ALLOC_COMMAND(CmdList, CommandType) { this, srv.get(), StartOffsetBytes, NumElements };
				}
				else
				{
					CommandType Command{ this,srv.get(),StartOffsetBytes,NumElements };
					Command.ExecuteNoCmdList();
				}

				return srv.get();
			};

			//TODO:switch case
			if (white::has_allflags(access, EAccessHint::EA_AccelerationStructure))
			{
				D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};

				SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
				SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
				SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
				SRVDesc.RaytracingAccelerationStructure.Location = Location.GetGPUVirtualAddress() + StartOffsetBytes;

				srv = std::make_unique<ShaderResourceView>(this, SRVDesc, 4);
				return srv.get();
			}
			if (!white::has_anyflags(access, EAccessHint::EA_GPUStructured))
			{
				struct D3D12InitializeBufferSRVCommand final : platform::Render::CommandBase
				{
					GraphicsBuffer* Buffer;
					ShaderResourceView* SRV;
					uint32 StartOffsetBytes;
					uint32 NumElements;

					D3D12InitializeBufferSRVCommand(GraphicsBuffer* InBuffer, ShaderResourceView* InSrv, uint32 InStartOffsetBytes, uint32 InNumElements)
						:Buffer(InBuffer), SRV(InSrv), StartOffsetBytes(InStartOffsetBytes), NumElements(InNumElements)
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
						const auto format = Buffer->format;
						const uint32 BufferSize = Buffer->size_in_byte;

						uint32 CreationStride = Stride;

						if (CreationStride == 0)
						{
							if (white::has_anyflags(access, EAccessHint::EA_Raw))
								CreationStride = 4;
							else
							{
								CreationStride = NumFormatBytes(Convert(format));
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
							SRVDesc.Format = format;
						}

						SRVDesc.Buffer.FirstElement = (BufferOffset) / CreationStride;
						SRVDesc.Buffer.NumElements = NumBytes / CreationStride;

						SRV->Initialize(SRVDesc, Buffer, CreationStride);
					}
				};

				return CreateShaderResourceView.operator()<D3D12InitializeBufferSRVCommand>();
			}
			else {

				struct D3D12InitializeStructuredBufferSRVCommand final : platform::Render::CommandBase
				{
					GraphicsBuffer* StructuredBuffer;
					ShaderResourceView* SRV;
					uint32 StartOffsetBytes;
					uint32 NumElements;

					D3D12InitializeStructuredBufferSRVCommand(GraphicsBuffer* InBuffer, ShaderResourceView* InSrv, uint32 InStartOffsetBytes, uint32 InNumElements)
						:StructuredBuffer(InBuffer), SRV(InSrv), StartOffsetBytes(InStartOffsetBytes), NumElements(InNumElements)
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

						uint32 Stride = StructuredBuffer->Stride;
						uint32 MaxElements =static_cast<uint32>(Location.GetSize() / Stride);
						StartOffsetBytes = std::min<uint32>(StartOffsetBytes, static_cast<uint32>(Location.GetSize()));
						uint32 StartElement = StartOffsetBytes / Stride;

						if (bByteAccessBuffer)
						{
							SRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
							SRVDesc.Format = DXGI_FORMAT_R32_TYPELESS;
							Stride = 4;
						}
						else
						{
							SRVDesc.Buffer.StructureByteStride = Stride;
							SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
						}

						SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
						SRVDesc.Buffer.NumElements = std::min<uint32>(MaxElements - StartElement, NumElements);
						SRVDesc.Buffer.FirstElement = Offset / Stride + StartElement;

						SRV->Initialize(SRVDesc, StructuredBuffer, Stride);
					}
				};

				return CreateShaderResourceView.operator()<D3D12InitializeStructuredBufferSRVCommand>();
			}
		}
		return srv.get();
	}
	UnorderedAccessView* GraphicsBuffer::RetriveUnorderedAccessView()
	{
		return uav.get();
	}

	struct D3D12CommandRenameUploadBuffer final : platform::Render::CommandBase
	{
		GraphicsBuffer* Buffer;
		ResourceLocation NewLocation;

		D3D12CommandRenameUploadBuffer(GraphicsBuffer* InBuffer, NodeDevice* ParentDevice)
			: Buffer(InBuffer)
			, NewLocation(ParentDevice)
		{
		}

		void ExecuteAndDestruct(platform::Render::CommandListBase& CmdList, platform::Render::CommandListContext& Context)
		{
			ExecuteNoCmdList();
			this->~D3D12CommandRenameUploadBuffer();
		}

		void ExecuteNoCmdList()
		{
			ResourceLocation::TransferOwnership(Buffer->Location, NewLocation);
			//recreate srv
		}
	};


	void* GraphicsBuffer::Map(platform::Render::CommandListImmediate& CmdList,platform::Render::Buffer::Access ba)
	{
		uint32 BufferSize = size_in_byte;

		uint32 Offset = 0;
		uint32 Size = size_in_byte;

		wconstraint(Size <= BufferSize);

		auto Adapter = GetParentDevice()->GetParentAdapter();

		const bool bIsDynamic = (usage & Buffer::Dynamic) ? true : false;

		void* Data = nullptr;

		if (bIsDynamic)
		{
			wconstraint(ba == Buffer::Write_Only || ba == Buffer::Write_No_Overwrite);

			if (MapData.bHasNeverBeenMappeded)
			{
				Data = Location.GetMappedBaseAddress();
			}
			else
			{
				if (!CmdList.IsExecuting() && ba == Buffer::Write_Only)
				{
					auto Command = CL_ALLOC_COMMAND(CmdList, D3D12CommandRenameUploadBuffer)(this, GetParentDevice());
					Data = Adapter->GetUploadHeapAllocator(GetParentDevice()->GetGPUIndex()).AllocUploadResource(BufferSize,Alignment, Command->NewLocation);
				}
				else
				{
					D3D12CommandRenameUploadBuffer Command(this, GetParentDevice());
					Data = Adapter->GetUploadHeapAllocator(GetParentDevice()->GetGPUIndex()).AllocUploadResource(BufferSize, Alignment, Command.NewLocation);

					Command.ExecuteNoCmdList();
				}
			}
		}
		else
		{
			throw white::unimplemented();
		}

		MapData.bHasNeverBeenMappeded = false;

		return Data;
	}
	void GraphicsBuffer::Unmap(platform::Render::CommandListImmediate& CmdList)
	{
		const bool bIsDynamic = (usage & Buffer::Dynamic) ? true : false;

		if (bIsDynamic)
		{
			// If the Buffer is dynamic, its upload heap memory can always stay mapped. Don't do anything.
		}
		else
		{
			throw white::unimplemented();
		}
	}

}
