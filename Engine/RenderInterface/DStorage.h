#pragma once

#define DIRECT_STORAGE_SUPPORT 1

#include <WBase/wmemory.hpp>
#include <WBase/winttype.hpp>
#include "SyncPoint.h"

#include <filesystem>


namespace fs = std::filesystem;

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

	template<typename Source,typename Target>
	struct DStorageRequest;

	template<>
	struct DStorageRequest< DStorageFile, byte>
	{
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

	using DStorageFile2MemoryRequest = DStorageRequest< DStorageFile, byte>;

	class DirectStorage
	{
	public:
		virtual std::shared_ptr<DStorageFile> OpenFile(const fs::path& path) = 0;

		virtual void EnqueueRequest(const DStorageFile2MemoryRequest& request) = 0;

		virtual std::shared_ptr<platform::Render::SyncPoint> SubmitUpload() = 0;
	};
}