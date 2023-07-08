#pragma once

#include <WBase/wdef.h>
#include <WFramework/Win32/WCLib/COM.h>
#include "RenderInterface/DStorage.h"
#include "Common.h"
#include "d3d12_dxgi.h"

namespace platform_ex::Windows::D3D12
{
	class DStorageFile : public platform_ex::DStorageFile
	{
	public:
		DStorageFile(platform_ex::COMPtr<IDStorageFile> infile, BY_HANDLE_FILE_INFORMATION ininfo)
			:platform_ex::DStorageFile(uint64(ininfo.nFileSizeHigh)<<32 | ininfo.nFileSizeLow),info(ininfo),file(infile)
		{}

		IDStorageFile* Get() const
		{
			return file.Get();
		}
	private:
		BY_HANDLE_FILE_INFORMATION info;
		platform_ex::COMPtr<IDStorageFile> file;
	};

	class DirectStorage :public platform_ex::DirectStorage
	{
	public:
		DirectStorage(D3D12Adapter* InParent);

		void CreateUploadQueue(ID3D12Device* device);

		std::shared_ptr<platform::Render::SyncPoint> SubmitUpload() override;

		std::shared_ptr<platform_ex::DStorageFile> OpenFile(const fs::path& path) override;

		void EnqueueRequest(const DStorageFile2MemoryRequest& request) override;
	private:
		D3D12Adapter* adapter;

		platform_ex::COMPtr<IDStorageFactory> factory;

		platform_ex::COMPtr<IDStorageQueue> file_upload_queue;

		std::vector<std::shared_ptr<const platform_ex::DStorageFile>> file_references;

		uint64 fence_commit_value;
	};
}