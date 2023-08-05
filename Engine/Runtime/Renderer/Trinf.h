#pragma once

#include "CoreTypes.h"
#include "Asset/TrinfAsset.h"
#include "RenderInterface/ICommandList.h"
#include "RenderInterface/DStorage.h"

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
			Resident,
		};

		uint64 TrinfKey = -1;
		StreamingState State = StreamingState::None;

		std::shared_ptr<DStorageFile> File;
		std::shared_ptr<DStorageSyncPoint> IORequest;

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

	class StreamingScene
	{
	public:
		StreamingScene();

		void Init();

		void AddResource(std::shared_ptr<Resources> pResource);

		void BeginAsyncUpdate(platform::Render::CommandList& cmdList);

		void EndAsyncUpdate(platform::Render::CommandList& cmdList);
	private:
		void ProcessNewResources(platform::Render::CommandList& cmdList);
	public:
		template<typename T>
		struct TrinfBuffer
		{
			static constexpr uint32 kPageSize = 64 * 1024;

			GrowOnlySpanAllocator Allocator;
			std::shared_ptr<platform::Render::GraphicsBuffer> DataBuffer;

			int32 Allocate(DSFileFormat::GpuRegion region)
			{
				return Allocator.Allocate((region.UncompressedSize + kPageSize - 1) / kPageSize);
			}

			std::shared_ptr<platform::Render::GraphicsBuffer> ResizeByteAddressBufferIfNeeded(platform::Render::CommandList& cmdList);
		};

		TrinfBuffer<wm::float3> Position;
		TrinfBuffer<uint32> Tangent;
		TrinfBuffer<wm::float2> TexCoord;
		TrinfBuffer<uint32> Index;
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

		DirectStorage& storage_api;
	};

	extern std::unique_ptr<StreamingScene> Scene;
}
