#pragma once

#define DIRECT_STORAGE_SUPPORT 1

#include <WBase/wmemory.hpp>
#include <filesystem>

namespace fs = std::filesystem;

namespace platform_ex
{
	struct DStorageFile
	{
		virtual ~DStorageFile();

		DStorageFile(uint64_t filesize)
			:file_size(filesize)
		{}

		uint64_t file_size;
	};

	class DirectStorage
	{
	public:
		virtual std::shared_ptr<DStorageFile> OpenFile(const fs::path& path) = 0;
	};
}