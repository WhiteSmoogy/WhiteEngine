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
