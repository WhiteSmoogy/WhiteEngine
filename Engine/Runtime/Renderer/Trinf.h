#pragma once

#include "CoreTypes.h"
#include "Asset/TrinfAsset.h"
#include "RenderInterface/ICommandList.h"
#include "RenderInterface/DStorage.h"
#include "RenderInterface/RenderResource.h"

import RenderGraph;

namespace Trinf
{

	using namespace platform_ex;
	using namespace platform_ex::DSFileFormat::TrinfFormat;

	class Resources
	{
	public:
		enum class StreamingState
		{
			None,
			Streaming,
			GPUStreaming,
			Resident,
		};

		uint64 TrinfKey = -1;
		StreamingState State = StreamingState::None;

		std::shared_ptr<DStorageFile> File;
		std::shared_ptr<DStorageSyncPoint> IORequest;
		white::ref_ptr<RenderGraph::RGPooledBuffer> GpuStream;

		TrinfHeader Header;
		MemoryRegion<TrinfGridHeader> Metadata;

		DStorageFile2GpuRequest BuildRequestForRegion(const DSFileFormat::GpuRegion& region)
		{
			DStorageFile2GpuRequest r;
			r.Compression = region.Compression;
			r.UncompressedSize = region.UncompressedSize;
			r.File.Offset = region.Data.Offset;
			r.File.Size = region.CompressedSize;
			r.File.Source = File;

			return r;
		}
	};

	struct GrowOnlySpanAllocator
	{
		int32 Allocate(int32 Num);

		int32 GetMaxSize()
		{
			return MaxSize;
		}

		int32 SearchFreeList(int32 Num);

	private:
		struct LinearAlloc
		{
			LinearAlloc(int32 InStartOffset, int32 InNum) :
				StartOffset(InStartOffset),
				Num(InNum)
			{}

			int32 StartOffset;
			int32 Num;

			bool Contains(LinearAlloc Other)
			{
				return StartOffset <= Other.StartOffset && (StartOffset + Num) >= (Other.StartOffset + Other.Num);
			}

			bool operator<(const LinearAlloc& Other) const
			{
				return StartOffset < Other.StartOffset;
			}
		};

		int32 MaxSize;
		std::vector<LinearAlloc> FreeSpans;
	};

	class StreamingScene :public platform::Render::RenderResource
	{
	public:
		StreamingScene();

		void InitRenderResource(platform::Render::CommandListBase& CmdList) override;

		void AddResource(std::shared_ptr<Resources> pResource);

		void BeginAsyncUpdate(RenderGraph::RGBuilder& Builder);

		void EndAsyncUpdate(RenderGraph::RGBuilder& Builder);
	private:
		void ProcessNewResources(RenderGraph::RGBuilder& Builder);
	public:
		template<typename T>
		struct TrinfBuffer
		{
			const char* Name = "TrinfBuffer";

			static constexpr uint32 kPageSize = 64 * 1024;

			GrowOnlySpanAllocator Allocator;
			white::ref_ptr<RenderGraph::RGPooledBuffer> DataBuffer;

			int32 Allocate(DSFileFormat::GpuRegion region)
			{
				return Allocator.Allocate((region.UncompressedSize + kPageSize - 1) / kPageSize);
			}

			RenderGraph::RGBufferRef ResizeByteAddressBufferIfNeeded(RenderGraph::RGBuilder& Builder);
		};

		TrinfBuffer<wm::float3> Position {.Name = "Trinf.Position"};
		TrinfBuffer<uint32> Tangent{ .Name = "Trinf.Tangent" };
		TrinfBuffer<wm::float2> TexCoord{ .Name = "Trinf.TexCoord" };
		TrinfBuffer<uint32> Index{ .Name = "Trinf.Index" };
	private:
		struct TrinfKey
		{
			int32 Index;
			int32 Position;
			int32 Tangent;
			int32 TexCoord;
		};


		GrowOnlySpanAllocator KeyAllocator;
		std::vector<TrinfKey> Keys;

		std::vector<std::shared_ptr<Resources>> PendingAdds;

		std::vector<std::shared_ptr<Resources>> Streaming;
		std::vector<std::shared_ptr<Resources>> Resident;
		std::vector<std::shared_ptr<Resources>> GpuStreaming;

		DirectStorage* storage_api;
	};

	extern platform::Render::GlobalResource<StreamingScene> Scene;
}
