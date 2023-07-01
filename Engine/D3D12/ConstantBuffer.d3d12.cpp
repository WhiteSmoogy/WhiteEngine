#include "ConstantBuffer.h"
#include "Adapter.h"
#include "NodeDevice.h"
using namespace  platform_ex::Windows::D3D12;

FastConstantBuffer::FastConstantBuffer(NodeDevice* InParent, FastConstantAllocator& InAllocator)
    :DeviceChild(InParent),CurrentUpdateSize(0),TotalUpdateSize(0),bIsDirty(false),Allocator(InAllocator)
{
}

bool FastConstantBuffer::Version(ResourceLocation& BufferOut, bool bDiscardSharedConstants)
{
	// If nothing has changed there is no need to alloc a new buffer.
	if (CurrentUpdateSize == 0)
	{
		return false;
	}

	if (bDiscardSharedConstants)
	{
		// If we're discarding shared constants, just use constants that have been updated since the last Commit.
		TotalUpdateSize = CurrentUpdateSize;
	}
	else
	{
		// If we're re-using shared constants, use all constants.
		TotalUpdateSize = std::max(CurrentUpdateSize, TotalUpdateSize);
	}

	void* Data = Allocator.Allocate(TotalUpdateSize, BufferOut);
	std::memcpy(Data, ShadowData, TotalUpdateSize);

	bIsDirty = false;

    return true;
}

void ConstantBuffer::Update(platform::Render::CommandList& cmdlist, white::uint32 size, void const* data)
{
	ResourceLocation UpdateLocation(GetParentDevice());

	void* MappedData = nullptr;
	if (Usage == platform::Render::Buffer::MultiFrame) {
		auto& Allocator = GetParentDevice()->GetParentAdapter()->GetUploadHeapAllocator(GetParentDevice()->GetGPUIndex());
		MappedData = Allocator.AllocUploadResource(ConstantBufferSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT, UpdateLocation);
	}
	else
	{
		auto& Allocator = GetParentDevice()->GetParentAdapter()->GetTransientConstantBufferAllocator();
		MappedData = Allocator.Allocate(ConstantBufferSize, UpdateLocation);
	}

	wconstraint(MappedData != nullptr);
	std::memcpy(MappedData, data, ConstantBufferSize);

	class D3D12UpdateConstantBuffer : platform::Render::CommandBase {
	public:
		ConstantBuffer* Buffer;
		ResourceLocation UpdatedLocation;

		D3D12UpdateConstantBuffer(ConstantBuffer* InBuffer, ResourceLocation& InUpdatedLocation)
			:Buffer(InBuffer), UpdatedLocation(InUpdatedLocation.GetParentDevice())
		{
			ResourceLocation::TransferOwnership(UpdatedLocation, InUpdatedLocation);
		}

		void ExecuteAndDestruct(platform::Render::CommandListBase& CmdList, platform::Render::CommandListContext& Context)
		{
			ResourceLocation::TransferOwnership(Buffer->Location, UpdatedLocation);

			this->~D3D12UpdateConstantBuffer();
		}
	};

	new (cmdlist.AllocCommand<D3D12UpdateConstantBuffer>()) D3D12UpdateConstantBuffer(this, UpdateLocation);
}

