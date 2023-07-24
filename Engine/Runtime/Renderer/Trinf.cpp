#include "Trinf.h"
#include "RenderInterface/InputLayout.hpp"
#include "RenderInterface/IContext.h"

using Trinf::StreamingScene;
using namespace platform::Render::Vertex;
using namespace platform_ex;

std::unique_ptr<StreamingScene> Trinf::Scene;

StreamingScene::StreamingScene()
	:storage_api(platform::Render::Context::Instance().GetDevice().GetDStorage())
{}

void StreamingScene::Init()
{

}

void StreamingScene::AddResource(std::shared_ptr<Resources> pResource)
{
	if (pResource->TrinfKey == -1)
	{
		pResource->TrinfKey = KeyAllocator.Allocate(1);

		TrinfKey key{};
		key.Index = Index.Allocate(pResource->Metadata->Index);
		key.Position = Position.Allocate(pResource->Metadata->Position);
		key.Tangent = Tangent.Allocate(pResource->Metadata->Tangent);
		key.TexCoord = TexCoord.Allocate(pResource->Metadata->TexCoord);

		Keys.resize(std::max(Keys.size(),pResource->TrinfKey + 1));
		Keys[pResource->TrinfKey] = key;
	}
}

void StreamingScene::BeginAsyncUpdate(platform::Render::CommandList& cmdList)
{
	ProcessNewResources(cmdList);

	for (auto itr = Streaming.begin(); itr != Streaming.end();)
	{
		if ((*itr)->IORequest->IsReady())
		{
			itr = Streaming.erase(itr);
			Resident.emplace_back(itr);
			(*itr)->State = Resources::StreamingState::Resident;
		}
		else
			++itr;
	}
}

void StreamingScene::EndAsyncUpdate(platform::Render::CommandList& cmdList)
{
}



void StreamingScene::ProcessNewResources(platform::Render::CommandList& cmdList)
{
	Position.ResizeByteAddressBufferIfNeeded(cmdList);
	Tangent.ResizeByteAddressBufferIfNeeded(cmdList);
	TexCoord.ResizeByteAddressBufferIfNeeded(cmdList);
	Index.ResizeByteAddressBufferIfNeeded(cmdList);

	for (auto resource : PendingAdds)
	{
		auto index_req = resource->BuildRequestForRegion(resource->Metadata->Index);
		storage_api.EnqueueRequest(index_req);

		auto pos_req = resource->BuildRequestForRegion(resource->Metadata->Position);
		storage_api.EnqueueRequest(pos_req);

		auto tan_req = resource->BuildRequestForRegion(resource->Metadata->Tangent);
		storage_api.EnqueueRequest(tan_req);

		auto uv_req = resource->BuildRequestForRegion(resource->Metadata->TexCoord);
		storage_api.EnqueueRequest(uv_req);

		resource->IORequest = storage_api.SubmitUpload(DStorageQueueType::Gpu);
		resource->State = Resources::StreamingState::Streaming;

		Streaming.emplace_back(resource);
	}

	PendingAdds.clear();
}