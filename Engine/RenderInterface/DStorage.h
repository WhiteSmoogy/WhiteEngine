#pragma once

#define DIRECT_STORAGE_SUPPORT 1

#include <filesystem>
#include <WBase/wmemory.hpp>

namespace fs = std::filesystem;

namespace platform::Windows::D3D12
{
	struct DStorageFile
	{
		virtual ~DStorageFile();

		uint64_t FileSize;
	};

	class DirectStorage
	{
	public:
		virtual std::shared_ptr<DStorageFile> OpenFile(const fs::path& path) = 0;
	};
}