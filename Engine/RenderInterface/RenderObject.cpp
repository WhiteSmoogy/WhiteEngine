#include "RenderObject.h"
#include <WBase/UnboundedQueue.h>

using namespace platform::Render;

static white::USPSCQueue<RObject*,false> PendingDeletes;
white::uint32 RObject::CurrentFrame = 0;
std::vector<RObject::RObjectToDelete> RObject::DeferredDeletionQueue;

void RObject::FlushPendingDeletes()
{
	auto Delete = [](std::vector<RObject*>& ToDelete)
	{
		for (auto& Object : ToDelete)
		{
			delete Object;
		}
	};

	while (!PendingDeletes.empty())
	{
		DeferredDeletionQueue.emplace_back(CurrentFrame);

		while (!PendingDeletes.empty())
		{
			DeferredDeletionQueue.back().Objects.push_back(PendingDeletes.dequeue());
		}
	}

	const auto NumFramesToExpire = 3;

	if (!DeferredDeletionQueue.empty())
	{
		auto DeletedBatchCount = 0;
		while (DeletedBatchCount < DeferredDeletionQueue.size())
		{
			auto& ObjectBatch = DeferredDeletionQueue[DeletedBatchCount];

			if (((ObjectBatch.FrameDeleted + NumFramesToExpire) < CurrentFrame))
			{
				Delete(ObjectBatch.Objects);
				++DeletedBatchCount;
			}
			else
			{
				break;
			}
		}

		DeferredDeletionQueue.erase(DeferredDeletionQueue.begin(), DeferredDeletionQueue.begin() + DeletedBatchCount);

		++CurrentFrame;
	}

}

platform::Render::RObject::~RObject()
{
}

void platform::Render::RObject::EnqueueDelete(RObject* value)
{
	PendingDeletes.enqueue(value);
}

