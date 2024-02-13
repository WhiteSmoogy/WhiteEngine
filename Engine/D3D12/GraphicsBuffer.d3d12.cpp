#include "GraphicsBuffer.hpp"

#include "RenderInterface/ICommandList.h"
#include "System/SystemEnvironment.h"

#include "Context.h"
#include "Convert.h"
#include "View.h"
#include "NodeDevice.h"

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

	std::pair<D3D12_RESOURCE_DESC, uint32> GetResourceDescAndAlignment(BufferDesc BufferDesc)
	{
		D3D12_RESOURCE_DESC ResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(BufferDesc.Size);

		if (BufferDesc.Access & EA_GPUUnordered)
		{
			ResourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}

		if ((BufferDesc.Access & EA_SRV) == 0)
		{
			ResourceDesc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
		}

		if (BufferDesc.Access & EA_DrawIndirect)
		{
			ResourceDesc.Flags |= D3D12_RESOURCE_FLAG_NONE;
		}

		// Structured buffers, non-ByteAddress buffers, need to be aligned to their stride to ensure that they can be addressed correctly with element based offsets.
		uint32 Alignment = (BufferDesc.Stride > 0) && (((BufferDesc.Access & EA_GPUStructured) != 0) || ((BufferDesc.Access & (EA_Raw | EA_DrawIndirect)) == 0)) ? BufferDesc.Stride : 4;

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
			// Copy from the temporary upload heap to the default resource
			{
				//could have been suballocated from shared resource  - not very optimal and should be batched
				if (!Destination->RequiresResourceStateTracking())
				{
					CommandContext.AddTransitionBarrier(Destination, Destination->GetDefaultResourceState(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
				}

				CommandContext.FlushResourceBarriers();

				CommandContext.CopyCommandList()->CopyBufferRegion(
					Destination->Resource(),
					CurrentBuffer->Location.GetOffsetFromBaseOfResource(),
					SrcResourceLoc.GetResource()->Resource(),
					SrcResourceLoc.GetOffsetFromBaseOfResource(), Size);

				// Update the resource state after the copy has been done (will take care of updating the residency as well)
				if (DestinationState != D3D12_RESOURCE_STATE_COPY_DEST)
				{
					CommandContext.AddTransitionBarrier(Destination, D3D12_RESOURCE_STATE_COPY_DEST, DestinationState, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
				}

				CommandContext.ConditionalSplitCommandList();
			}

			// Buffer is now written and ready, so unlock the block (locked after creation and can be defragmented if needed)
			CurrentBuffer->Location.UnlockPoolData();
		}
	};

	static thread_local char DebugNames[256];

	const char* GetDebugBufferName(Buffer::Usage usage, white::uint32 access, uint32 size)
	{
		if (white::has_anyflags(access, EAccessHint::EA_GPUStructured))
			std::format_to(DebugNames, "StructBuffer({}x{})_{}", (uint32)usage, access, size);
		else
			std::format_to(DebugNames, "Buffer({}x{})_{}", (uint32)usage, access, size);

		return DebugNames;
	}

	GraphicsBuffer* Device::CreateBuffer(Buffer::Usage usage, white::uint32 access, uint32 Size, uint32 Stride, ResourceCreateInfo init_data)
	{
		 ReplaceUnknownName(init_data, GetDebugBufferName(usage, access,Size));

		BufferDesc BufferDesc = {
			.Size = Size,
			.Stride = Stride,
			.Usage = usage,
			.Access = access
		};

		return CreateBufferDesc(&CommandListExecutor::GetImmediateCommandList(), BufferDesc, DXGI_FORMAT_UNKNOWN, init_data);
	}

	GraphicsBuffer* Device::CreateBufferDesc(CommandListImmediate* Cmdlist, const BufferDesc& BufferDesc, DXGI_FORMAT format, ResourceCreateInfo& CreateInfo)
	{
		const bool bIsDynamic = (BufferDesc.Usage & Buffer::Usage::Dynamic) ? true : false;

		D3D12_HEAP_TYPE HeapType = bIsDynamic ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_DEFAULT;
		const D3D12_RESOURCE_STATES InitialState = BufferAllocator::GetDefaultInitialResourceState<ResourceStateMode::Default>(HeapType, BufferDesc.Access);

		return CreateBufferDispatch(Cmdlist, BufferDesc, InitialState, format, CreateInfo);
	}
	

	GraphicsBuffer* Device::CreateBufferDispatch(CommandListImmediate* Cmdlist, const BufferDesc& BufferDesc, D3D12_RESOURCE_STATES InitialState, DXGI_FORMAT format, ResourceCreateInfo& CreateInfo)
	{
		IResourceAllocator* Allocator = nullptr;

		auto [Desc, Alignment] = GetResourceDescAndAlignment(BufferDesc);
		Desc.Format = format;

		if (white::has_allflags(BufferDesc.Access,EA_AccelerationStructure))
			return CreateBuffer<ResourceStateMode::Single>(Cmdlist, Desc, InitialState, BufferDesc, Alignment, CreateInfo, Allocator);
		else if(white::has_allflags(BufferDesc.Access, EA_DStorage))
			return CreateBuffer<ResourceStateMode::Multi>(Cmdlist, Desc, InitialState, BufferDesc, Alignment, CreateInfo, Allocator);
		else
			return CreateBuffer<ResourceStateMode::Default>(Cmdlist, Desc, InitialState, BufferDesc, Alignment, CreateInfo, Allocator);
	}

	template<ResourceStateMode Mode>
	GraphicsBuffer* platform_ex::Windows::D3D12::Device::CreateBuffer(
		platform::Render::CommandListImmediate* Cmdlist, 
		const D3D12_RESOURCE_DESC& Desc, 
		D3D12_RESOURCE_STATES InitialState,
		const BufferDesc& BufferDesc,
		uint32 Alignment, ResourceCreateInfo& CreateInfo,
		IResourceAllocator* Allocator)
	{
		auto device = GetNodeDevice(CreateInfo.GPUIndex);

		GraphicsBuffer* BufferOut = new GraphicsBuffer(device, BufferDesc.Usage, BufferDesc.Access, BufferDesc.Size, Desc.Format);

		//WithoutNativeResource
		if (CreateInfo.WithoutNativeResource)
			return BufferOut;

		BufferOut->Alignment = Alignment;
		BufferOut->Stride = BufferDesc.Stride;

		bool bIsDynamic = BufferDesc.IsDynamic();
		if (bIsDynamic)
		{
			AllocateBuffer<Mode>(GetNodeDevice(0), Desc, BufferDesc, InitialState, CreateInfo, Alignment, BufferOut, BufferOut->Location, Allocator);
		}
		else
		{
			const D3D12_RESOURCE_STATES CreateState = CreateInfo.GetData() != nullptr ? D3D12_RESOURCE_STATE_COPY_DEST : InitialState;

			AllocateBuffer<Mode>(GetNodeDevice(0), Desc, BufferDesc, CreateState, CreateInfo, Alignment, BufferOut, BufferOut->Location, Allocator);

			if (CreateInfo.GetData() == nullptr)
			{
				BufferOut->Location.UnlockPoolData();
			}
		}

		if (CreateInfo.GetData() != nullptr)
		{
			if (!bIsDynamic && BufferOut->Location.IsValid())
			{
				const bool bOnAsyncThread = !Environment->Scheduler->is_render_schedule();

				ResourceLocation SrcResourceLoc(BufferOut->GetParentDevice());
				void* pData;
				if (bOnAsyncThread)
				{
					const uint32 GPUIdx = SrcResourceLoc.GetParentDevice()->GetGPUIndex();
					pData = GetUploadHeapAllocator(GPUIdx).AllocUploadResource(BufferDesc.Size, 4u, SrcResourceLoc);
				}
				else
				{
					pData = SrcResourceLoc.GetParentDevice()->GetDefaultFastAllocator().Allocate(BufferDesc.Size, 4UL, &SrcResourceLoc);
				}
				wconstraint(pData);
				std::memcpy(pData, CreateInfo.GetData(), BufferDesc.Size);

				if (bOnAsyncThread)
				{
					new (Cmdlist->AllocCommand<D3D12CommandInitializeBuffer>()) D3D12CommandInitializeBuffer(BufferOut, SrcResourceLoc, BufferDesc.Size, InitialState);
				}
				else
				{
					D3D12CommandInitializeBuffer Command(BufferOut, SrcResourceLoc, BufferDesc.Size, InitialState);
					Command.ExecuteNoCmdList();
				}
			}
		}

		return BufferOut;
	}

	template
	GraphicsBuffer* platform_ex::Windows::D3D12::Device::CreateBuffer<ResourceStateMode::Single>(
		CommandListImmediate* Cmdlist, 
		const D3D12_RESOURCE_DESC& InDesc,
		D3D12_RESOURCE_STATES InitialState,
		const BufferDesc& BufferDesc,
		uint32 Alignment,
		ResourceCreateInfo& CreateInfo,
		IResourceAllocator* ResourceAllocator);

	template
		GraphicsBuffer* platform_ex::Windows::D3D12::Device::CreateBuffer<ResourceStateMode::Multi>(
			CommandListImmediate* Cmdlist,
			const D3D12_RESOURCE_DESC& InDesc,
			D3D12_RESOURCE_STATES InitialState,
			const BufferDesc& BufferDesc,
			uint32 Alignment,
			ResourceCreateInfo& CreateInfo,
			IResourceAllocator* ResourceAllocator);

	template<ResourceStateMode Mode>
	void Device::AllocateBuffer(NodeDevice* Device, const D3D12_RESOURCE_DESC& InDesc, 
		const BufferDesc& BufferDesc,
		D3D12_RESOURCE_STATES InCreateState,
		ResourceCreateInfo& CreateInfo,
		uint32 Alignment, GraphicsBuffer* Buffer,
		ResourceLocation& ResourceLocation, IResourceAllocator* ResourceAllocator)
	{
		if (BufferDesc.IsDynamic())
		{
			wconstraint(ResourceAllocator == nullptr);
			wconstraint(InCreateState == D3D12_RESOURCE_STATE_GENERIC_READ);
			void* pData = GetUploadHeapAllocator(Device->GetGPUIndex()).AllocUploadResource(BufferDesc.Size, Alignment, ResourceLocation);
			wconstraint(ResourceLocation.GetSize() == BufferDesc.Size);

			if (CreateInfo.GetData())
			{
				// Handle initial data
				std::memcpy(pData, CreateInfo.GetData(), BufferDesc.Size);
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
				Device->GetDefaultBufferAllocator().AllocDefaultResource<Mode>(D3D12_HEAP_TYPE_DEFAULT, InDesc, BufferDesc.Access, InCreateState, ResourceLocation, Alignment, CreateInfo.Name);
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

		auto rtv = new RenderTargetView(GetDefaultNodeDevice());
		rtv->CreateView(desc, this);
		return rtv_maps->emplace(key, unique_raw(rtv)).first->second.get();
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
