#include "DStorageAsset.h"
#include "RenderInterface/IContext.h"
#include "RenderInterface/ITexture.hpp"
#include "Asset/DDSX.h"
#include <vector>

using platform::Render::DStorage;
using platform_ex::DStorageQueueType;
using platform_ex::DStorageFile2MemoryRequest;
using platform_ex::DStorageFile2GpuRequest;
using platform_ex::DStorageCompressionFormat;
using platform_ex::path;
using namespace platform_ex::DSFileFormat;

namespace platform
{
	void EmplaceResource(const std::string& name, Render::TexturePtr texture);
}

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

		std::vector<platform::Render::TexturePtr> textures;

		co_await Environment->Scheduler->schedule_render();

		auto& device = platform::Render::Context::Instance().GetDevice();
		for (uint32 i = 0; i < metadata->TexturesCount; ++i)
		{
			const auto& desc = metadata->TexuresDesc[i];

			textures.emplace_back(platform::Render::shared_raw_robject(
				CreateTexture(device, desc)));

			const char* Name = metadata->Textures[i].Name;

			platform::EmplaceResource(Name, textures.back());
		}

		co_await Environment->Scheduler->schedule();

		for (uint32 i = 0; i < metadata->TexturesCount; ++i)
		{
			const auto& desc = metadata->TexuresDesc[i];
			const auto& textureMetadata = metadata->Textures[i];

			EnqueuReadTexture(textures[i], desc, textureMetadata);
		}

		co_await storage_api.SubmitUpload(DStorageQueueType::Gpu);
	}

	DStorageFile2GpuRequest BuildRequestForRegion(const GpuRegion& region)
	{
		DStorageFile2GpuRequest r;
		r.Compression = region.Compression;
		r.UncompressedSize = region.UncompressedSize;
		r.File.Offset = region.Data.Offset;
		r.File.Source = file;
		r.File.Size = region.CompressedSize;

		return r;
	}

	void EnqueuReadTexture(platform::Render::TexturePtr tex, const D3DResourceDesc& desc, TexturMetadata const& textureMetadata)
	{
		for (uint32_t i = 0; i < textureMetadata.SingleMipsCount; ++i)
		{
			auto const& region = textureMetadata.SingleMips[i];

			DStorageFile2GpuRequest r = BuildRequestForRegion(region);

			DStorageFile2GpuRequest::TextureRegion destRegion;
			destRegion.Region.Right = static_cast<uint32_t>(desc.Width >> i);
			destRegion.Region.Bottom = static_cast<uint32_t>(desc.Height >> i);
			destRegion.Region.Back = 1;
			destRegion.Target = tex.get();
			destRegion.SubresourceIndex = i;

			r.Destination = destRegion;

			storage_api.EnqueueRequest(r);
		}

		//has remaing mips?
		if (textureMetadata.RemainingMips.UncompressedSize > 0)
		{
			DStorageFile2GpuRequest r = BuildRequestForRegion(textureMetadata.RemainingMips);

			DStorageFile2GpuRequest::MultipleSubresources multiRes;

			multiRes.Texture = tex.get();
			multiRes.FirstSubresource = textureMetadata.SingleMipsCount;
			r.Destination = multiRes;

			storage_api.EnqueueRequest(r);
		}
	}

	platform::Render::Texture* CreateTexture(platform::Render::Device& device, const platform_ex::DSFileFormat::D3DResourceDesc& desc)
	{
		auto format = dds::FromDXGIFormat(desc.Format);
		auto access = platform::Render::EA_GPURead | platform::Render::EA_Immutable;
		platform::Render::SampleDesc sample_info{ desc.SampleDesc.Count,desc.SampleDesc.Quality };

		switch ((platform::Render::TextureType)desc.Dimension)
		{
		case platform::Render::TextureType::T_1D:
			return device.CreateTexture(
				static_cast<uint16>(desc.Width),
				static_cast<uint8>(desc.MipLevels),
				static_cast<uint8>(desc.DepthOrArraySize),
				format, access, sample_info, nullptr
			);
		case platform::Render::TextureType::T_2D:
			return device.CreateTexture(
				static_cast<uint16>(desc.Width),
				static_cast<uint16>(desc.Height),
				static_cast<uint8>(desc.MipLevels),
				static_cast<uint8>(desc.DepthOrArraySize),
				format,access,sample_info,nullptr
			);
		case platform::Render::TextureType::T_3D:
		{
			platform::Render::Texture3DInitializer Initializer
			{
				.Width = static_cast<uint16>(desc.Width),
				.Height = static_cast<uint16>(desc.Height),
				.Depth = static_cast<uint16>(desc.DepthOrArraySize),
				.NumMipmaps = static_cast<uint8>(desc.MipLevels),
				.ArraySize = 1,
				.Format = format,
				.Access = access,
				.NumSamples = 1
			};
			return device.CreateTexture(Initializer, nullptr);
		}
		default:
			WAssert(false, "unexcepted texture Dimension");
			break;
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