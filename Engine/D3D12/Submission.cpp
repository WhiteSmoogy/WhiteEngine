#include "Submission.h"
#include "NodeQueue.h"
#include "NodeDevice.h"
#include "Context.h"

namespace platform_ex::Windows::D3D12 {

	PayloadBase::PayloadBase(NodeDevice* Device, QueueType QueueType)
		:Queue(Device->GetQueue(QueueType))
	{
	}

	PayloadBase::~PayloadBase()
	{
		for (auto* Allocator : AllocatorsToRelease)
		{
			Queue.Device->ReleaseCommandAllocator(Allocator);
		}
	}

	void Context::SubmitPayloads(white::span<D3D12Payload*> payloads)
	{

	}

	uint64 NodeQueue::ExecutePayload()
	{
		return 0;
	}
	
}
