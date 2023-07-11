#pragma once

#define DIRECT_STORAGE_SUPPORT 1

#include <WBase/wmemory.hpp>
#include <WBase/winttype.hpp>
#include "SyncPoint.h"

#include <filesystem>
#include <variant>

namespace fs = std::filesystem;

namespace platform::Render
{
	class Textures;
}

namespace platform_ex
{
	using namespace white::inttype;
	using white::byte;

	struct DStorageFile
	{
		virtual ~DStorageFile();

		DStorageFile(uint64_t filesize)
			:file_size(filesize)
		{}

		uint64_t file_size;
	};

	enum class DStorageCompressionFormat
	{
		None = 0,
		GDeflate = 1,
		Zlib = 0x80,
	};

	template<typename Source,typename Target>
	struct DStorageRequest;

	template<>
	struct DStorageRequest< DStorageFile, byte>
	{
		DStorageCompressionFormat Compression = DStorageCompressionFormat::None;

		struct {
			std::shared_ptr<const DStorageFile> Source;
			uint64 Offset;
			uint32 Size;
		} File;

		struct {
			byte* Buffer;
			uint32 Size;
		} Memory;
	};

	template<>
	struct DStorageRequest< DStorageFile, platform::Render::RObject>
	{
		DStorageCompressionFormat Compression = DStorageCompressionFormat::None;

		struct {
			std::shared_ptr<const DStorageFile> Source;
			uint64 Offset;
			uint32 Size;
		} File;

		struct TextureRegion {
			platform::Render::Texture* Target;
			uint32 SubresourceIndex;

			struct {
				uint32 Right;
				uint32 Bottom;
				uint32 Back;
			} Region;
		};

		struct MultipleSubresources
		{
			union 
			{
				platform::Render::Texture* Texture;
			};
			uint32 FirstSubresource;
		};

		std::variant< TextureRegion, MultipleSubresources> Destination;
	};

	using DStorageFile2MemoryRequest = DStorageRequest< DStorageFile, byte>;
	using DStorageFile2GpuRequest = DStorageRequest< DStorageFile, platform::Render::RObject>;

	enum class DStorageQueueType
	{
		Memory = 0x1,
		Gpu = 0x2,
	};

	constexpr inline DStorageQueueType operator|(DStorageQueueType lhs, DStorageQueueType rhs)
	{
		return static_cast<DStorageQueueType>(static_cast<int32>(lhs) | static_cast<int32>(rhs));
	}

	class DStorageSyncPoint :platform::Render::SyncPoint
	{
	};

	class DirectStorage
	{
	public:
		virtual std::shared_ptr<DStorageFile> OpenFile(const fs::path& path) = 0;

		virtual void EnqueueRequest(const DStorageFile2MemoryRequest& request) = 0;
		virtual void EnqueueRequest(const DStorageFile2GpuRequest& request) = 0;

		virtual std::shared_ptr<DStorageSyncPoint> SubmitUpload(DStorageQueueType type) = 0;
	};
}