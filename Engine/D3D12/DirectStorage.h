#pragma once

#include <WBase/wdef.h>
#include <WFramework/Win32/WCLib/COM.h>
#include "RenderInterface/DStorage.h"
#include "Common.h"
#include "d3d12_dxgi.h"
#include <shared_mutex>

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

		std::shared_ptr<platform_ex::DStorageSyncPoint> SubmitUpload(platform_ex::DStorageQueueType type) override;

		std::shared_ptr<platform_ex::DStorageFile> OpenFile(const fs::path& path) override;

		void EnqueueRequest(const DStorageFile2MemoryRequest& request) override;
		void EnqueueRequest(const DStorageFile2GpuRequest& request) override;

		uint32 RequestNextStatusIndex();
	private:
		struct DStorageSyncPoint :platform_ex::DStorageSyncPoint
		{
			DStorageSyncPoint(platform_ex::COMPtr<IDStorageStatusArray> statusArray,uint32 index);

			~DStorageSyncPoint();

			bool IsReady() const override;

			void Wait() override;

			void AwaitSuspend(std::coroutine_handle<> handle) override;

			platform_ex::COMPtr<IDStorageStatusArray> status_array;
			uint32 status_index;
			HANDLE complete_event;
			std::coroutine_handle<> continue_handle;
			TP_WAIT* wait;
		};

		static void CALLBACK DecompressionWork(TP_CALLBACK_INSTANCE*, void*, TP_WORK*);
		static void CALLBACK OnCustomDecompressionRequestsAvailable(TP_CALLBACK_INSTANCE*, void*, TP_WAIT* wait, TP_WAIT_RESULT);
		static void CALLBACK OnQueueError(TP_CALLBACK_INSTANCE*, void*, TP_WAIT* wait, TP_WAIT_RESULT);
	private:
		D3D12Adapter* adapter;

		platform_ex::COMPtr<IDStorageFactory> factory;
		platform_ex::COMPtr<IDStorageCustomDecompressionQueue1> decompression_queue;
		HANDLE decompression_queue_event;
		TP_WAIT* decompression_wait;
		TP_WORK* decompression_work;

		std::deque<DSTORAGE_CUSTOM_DECOMPRESSION_REQUEST> decompression_requests;
		std::shared_mutex requests_mutex;

		platform_ex::COMPtr<IDStorageQueue1> memory_upload_queue;
		platform_ex::COMPtr<IDStorageQueue1> gpu_upload_queue;

		std::vector<std::shared_ptr<const platform_ex::DStorageFile>> file_references;

		uint64 fence_commit_value;

		platform_ex::COMPtr<IDStorageStatusArray> status_array;

		constexpr static uint32 kStatusCount = 16;
		uint32 status_count = kStatusCount;
	};
}