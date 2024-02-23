#include "Trinf.h"
#include "RenderInterface/InputLayout.hpp"
#include "RenderInterface/IContext.h"
#include "Runtime/RenderCore/UnifiedBuffer.h"
#include "Core/Container/vector.hpp"

using Trinf::StreamingScene;
using namespace platform::Render::Vertex;
namespace pr = platform::Render;
using namespace platform_ex;
using platform::Render::Context;

using RenderGraph::RGBufferDesc;

platform::Render::GlobalResource<StreamingScene> Trinf::Scene;

int32 Trinf::GrowOnlySpanAllocator::Allocate(int32 Num)
{
	const int32 FoundIndex = SearchFreeList(Num);

	// Use an existing free span if one is found
	if (FoundIndex != white::INDEX_NONE)
	{
		auto FreeSpan = FreeSpans[FoundIndex];

		if (FreeSpan.Num > Num)
		{
			// Update existing free span with remainder
			FreeSpans[FoundIndex] = LinearAlloc(FreeSpan.StartOffset + Num, FreeSpan.Num - Num);
		}
		else
		{
			// Fully consumed the free span
			white::remove_at_swap(FreeSpans, FoundIndex);
		}

		return FreeSpan.StartOffset;
	}

	// New allocation
	int32 StartOffset = MaxSize;
	MaxSize = MaxSize + Num;

	return StartOffset;
}

int32 Trinf::GrowOnlySpanAllocator::SearchFreeList(int32 Num)
{
	// Search free list for first matching
	for (int32 i = 0; i < FreeSpans.size(); i++)
	{
		auto CurrentSpan = FreeSpans[i];

		if (CurrentSpan.Num >= Num)
		{
			return i;
		}
	}

	return white::INDEX_NONE;
}

StreamingScene::StreamingScene()
{}

template<typename T>
RenderGraph::RGBufferRef StreamingScene::TrinfBuffer<T>::ResizeByteAddressBufferIfNeeded(RenderGraph::RGBuilder& Builder)
{
	auto target_size = Allocator.GetMaxSize() * kPageSize;
	auto InternalBufferOld = Builder.RegisterExternal(DataBuffer, RenderGraph::ERGBufferFlags::MultiFrame);

	if (DataBuffer->GetSize() < target_size)
	{
		auto NewDataBuffer = Builder.CreateBuffer(RGBufferDesc::CreateByteAddressDesc(target_size), __func__);

		platform::Render::MemcpyResourceParams params =
		{
			.Count = DataBuffer->GetSize(),
			.SrcOffset = 0,
			.DstOffset = 0,
		};

		platform::Render::MemcpyResource(Builder, NewDataBuffer, InternalBufferOld, params);
		DataBuffer = Builder.ToExternal(NewDataBuffer);

		return NewDataBuffer;
	}

	return InternalBufferOld;
}

void StreamingScene::InitRenderResource(platform::Render::CommandListBase&)
{
	Index.DataBuffer = RenderGraph::AllocatePooledBuffer(RGBufferDesc::CreateByteAddressDesc(4), "Trinf.Index");
	Position.DataBuffer = RenderGraph::AllocatePooledBuffer(RGBufferDesc::CreateByteAddressDesc(sizeof(wm::float3)), "Trinf.Position");
	Tangent.DataBuffer = RenderGraph::AllocatePooledBuffer(RGBufferDesc::CreateByteAddressDesc(sizeof(uint32)), "Trinf.Tangent");
	TexCoord.DataBuffer = RenderGraph::AllocatePooledBuffer(RGBufferDesc::CreateByteAddressDesc(sizeof(wm::float2)), "Trinf.TexCoord");

	storage_api = &Context::Instance().GetDevice().GetDStorage();
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

		Keys.resize(std::max(Keys.size(), pResource->TrinfKey + 1));
		Keys[pResource->TrinfKey] = key;

		PendingAdds.emplace_back(pResource);
	}
}

void StreamingScene::BeginAsyncUpdate(RenderGraph::RGBuilder& Builder)
{
	ProcessNewResources(Builder);

	for (auto itr = Streaming.begin(); itr != Streaming.end();)
	{
		if ((*itr)->IORequest->IsReady())
		{
			GpuStreaming.emplace_back(*itr);
			(*itr)->State = Resources::StreamingState::GPUStreaming;

			itr = Streaming.erase(itr);
		}
		else
			++itr;
	}
}

void StreamingScene::EndAsyncUpdate(RenderGraph::RGBuilder& Builder)
{
	auto index = Builder.RegisterExternal(Index.DataBuffer, RenderGraph::ERGBufferFlags::MultiFrame);
	auto position = Builder.RegisterExternal(Position.DataBuffer, RenderGraph::ERGBufferFlags::MultiFrame);
	auto tangent = Builder.RegisterExternal(Tangent.DataBuffer, RenderGraph::ERGBufferFlags::MultiFrame);
	auto texcoord = Builder.RegisterExternal(TexCoord.DataBuffer, RenderGraph::ERGBufferFlags::MultiFrame);

	for (auto itr = GpuStreaming.begin(); itr != GpuStreaming.end();)
	{
		auto resource = *itr;

		auto key = Keys[resource->TrinfKey];

		platform::Render::MemcpyResourceParams params =
		{
			.Count = resource->Metadata->Index.UncompressedSize / sizeof(float),
			.SrcOffset = 0,
			.DstOffset = key.Index * Index.kPageSize,
		};

		auto GpuStream = Builder.RegisterExternal(resource->GpuStream);

		platform::Render::MemcpyResource(Builder, index, GpuStream, params);

		params.SrcOffset += params.Count;
		params.Count = resource->Metadata->Position.UncompressedSize / sizeof(float);
		params.DstOffset = key.Position * Position.kPageSize;
		platform::Render::MemcpyResource(Builder, position, GpuStream, params);

		params.SrcOffset += params.Count;
		params.Count = resource->Metadata->Tangent.UncompressedSize / sizeof(float);
		params.DstOffset = key.Tangent * Tangent.kPageSize;
		platform::Render::MemcpyResource(Builder, tangent, GpuStream, params);

		params.SrcOffset += params.Count;
		params.Count = resource->Metadata->TexCoord.UncompressedSize / sizeof(float);
		params.DstOffset = key.TexCoord * TexCoord.kPageSize;
		platform::Render::MemcpyResource(Builder, texcoord, GpuStream, params);

		Resident.emplace_back(resource);
		resource->State = Resources::StreamingState::Resident;
		resource->GpuStream.reset();

		itr = GpuStreaming.erase(itr);
	}
}

void StreamingScene::ProcessNewResources(RenderGraph::RGBuilder& Builder)
{
	Position.ResizeByteAddressBufferIfNeeded(Builder);
	Tangent.ResizeByteAddressBufferIfNeeded(Builder);
	TexCoord.ResizeByteAddressBufferIfNeeded(Builder);
	Index.ResizeByteAddressBufferIfNeeded(Builder);

	for (auto resource : PendingAdds)
	{
		auto key = Keys[resource->TrinfKey];

		DStorageFile2GpuRequest::Buffer target{};

		auto GpuStreamingSize = 0;
		GpuStreamingSize += resource->Metadata->Index.UncompressedSize;
		GpuStreamingSize += resource->Metadata->Position.UncompressedSize;
		GpuStreamingSize += resource->Metadata->Tangent.UncompressedSize;
		GpuStreamingSize += resource->Metadata->TexCoord.UncompressedSize;

		resource->GpuStream = RenderGraph::AllocatePooledBuffer(RGBufferDesc::CreateByteAddressDesc(GpuStreamingSize) | pr::EAccessHint::DStorage, "Trinf.ResourceStream");

		{
			auto index_req = resource->BuildRequestForRegion(resource->Metadata->Index);
			target.Offset = 0;
			target.Size = index_req.UncompressedSize;
			target.Target = resource->GpuStream->GetRObject();
			index_req.Destination = target;
			storage_api->EnqueueRequest(index_req);
		}

		{
			auto pos_req = resource->BuildRequestForRegion(resource->Metadata->Position);
			target.Offset += target.Size;
			target.Size = pos_req.UncompressedSize;
			target.Target = resource->GpuStream->GetRObject();
			pos_req.Destination = target;
			storage_api->EnqueueRequest(pos_req);
		}

		{
			auto tan_req = resource->BuildRequestForRegion(resource->Metadata->Tangent);
			target.Offset += target.Size;
			target.Size = tan_req.UncompressedSize;
			target.Target = resource->GpuStream->GetRObject();
			tan_req.Destination = target;

			storage_api->EnqueueRequest(tan_req);
		}

		{
			auto uv_req = resource->BuildRequestForRegion(resource->Metadata->TexCoord);
			target.Offset += target.Size;
			target.Size = uv_req.UncompressedSize;
			target.Target = resource->GpuStream->GetRObject();
			uv_req.Destination = target;

			storage_api->EnqueueRequest(uv_req);
		}

		resource->IORequest = storage_api->SubmitUpload(DStorageQueueType::Gpu);
		resource->State = Resources::StreamingState::Streaming;

		Streaming.emplace_back(resource);
	}

	PendingAdds.clear();
}


