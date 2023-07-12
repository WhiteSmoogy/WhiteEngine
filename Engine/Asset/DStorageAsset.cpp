#include "DStorageAsset.h"
#include "RenderInterface/IContext.h"
#include "RenderInterface/ITexture.hpp"
#include <vector>

using platform::Render::DStorage;
using platform_ex::DStorageQueueType;
using platform_ex::DStorageFile2MemoryRequest;
using platform_ex::DStorageFile2GpuRequest;
using platform_ex::DStorageCompressionFormat;
using platform_ex::path;
using namespace platform_ex::DSFileFormat;

template<typename T>
class MemoryRegion
{
	std::unique_ptr<white::byte[]> buffer;
public:
	MemoryRegion() = default;

	MemoryRegion(std::unique_ptr<white::byte[]> buffer)
		: buffer(std::move(buffer))
	{
	}

	white::byte* data()
	{
		return buffer.get();
	}

	T* operator->()
	{
		return reinterpret_cast<T*>(buffer.get());
	}

	T const* operator->() const
	{
		return reinterpret_cast<T const*>(buffer.get());
	}
};

struct DStorageAssetFile
{
	DStorageAssetFile(path const& assetpath)
		:storage_api(platform::Render::Context::Instance().GetDevice().GetDStorage()),
		file(storage_api.OpenFile(assetpath))
	{
	}

	template<typename T>
	void EnqueueRead(uint64_t offset, T* dest)
	{
		DStorageFile2MemoryRequest r{};

		r.File.Source = file;
		r.File.Offset = offset;
		r.File.Size = static_cast<uint32>(sizeof(T));

		r.Compression = DStorageCompressionFormat::None;
		r.Memory.Buffer = reinterpret_cast<white::byte*>(dest);
		r.Memory.Size = r.File.Size;

		storage_api.EnqueueRequest(r);
	}

	template<typename T>
	MemoryRegion<T> EnqueueRead(const Region<T>& region)
	{
		MemoryRegion<T> memory{ std::make_unique<white::byte[]>(region.UncompressedSize) };

		DStorageFile2MemoryRequest r{};

		r.File.Source = file;
		r.File.Offset = region.Data.Offset;
		r.File.Size = region.CompressedSize;

		r.Compression = region.Compression;
		r.Memory.Buffer = memory.data();
		r.Memory.Size = region.UncompressedSize;

		storage_api.EnqueueRequest(r);
		
		return memory;
	}

	white::coroutine::Task<void> LoadAsync()
	{
		Header header;
		EnqueueRead(0, &header);

		co_await storage_api.SubmitUpload(DStorageQueueType::Memory);

		auto metadata = EnqueueRead(header.CpuMedadata);

		co_await storage_api.SubmitUpload(DStorageQueueType::Memory);

		std::vector<std::shared_ptr<platform::Render::Texture>> textures;

		co_await Environment->Scheduler->schedule_render();

		for (uint32 i = 0; i < metadata->TexturesCount; ++i)
		{
			//create texture
		}
	}

private:
	DStorage& storage_api;
	std::shared_ptr<platform_ex::DStorageFile> file;
};

white::coroutine::Task<void> platform_ex::AsyncLoadDStorageAsset(path const& assetpath)
{
	DStorageAssetFile asset(assetpath);

	co_await asset.LoadAsync();
}