#include "DirectStorage.h"
#include "Fence.h"

using namespace platform_ex::Windows::D3D12;

DEFINE_GUID(IID_IDStorageFactory,	0x6924ea0c, 0xc3cd, 0x4826, 0xb1, 0x0a, 0xf6, 0x4f, 0x4e, 0xd9, 0x27, 0xc1);
DEFINE_GUID(IID_IDStorageQueue,		0xcfdbd83f, 0x9e06, 0x4fda, 0x8e, 0xa5, 0x69, 0x04, 0x21, 0x37, 0xf4, 0x9b);
DEFINE_GUID(IID_IDStorageFile,		0x5de95e7b, 0x955a, 0x4868, 0xa7, 0x3c, 0x24, 0x3b, 0x29, 0xf4, 0xb8, 0xda);

DirectStorage::DirectStorage(D3D12Adapter* InParent)
	:adapter(InParent)
{
	CheckHResult(DStorageGetFactory(COMPtr_RefParam(factory, IID_IDStorageFactory)));
}

void DirectStorage::CreateUploadQueue(ID3D12Device* device)
{
	DSTORAGE_QUEUE_DESC queueDesc{};
	queueDesc.Capacity = DSTORAGE_MAX_QUEUE_CAPACITY;
	queueDesc.Priority = DSTORAGE_PRIORITY_NORMAL;
	queueDesc.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
	queueDesc.Device = device;

	CheckHResult(factory->CreateQueue(&queueDesc, COMPtr_RefParam(file_upload_queue, IID_IDStorageQueue)));
}

std::shared_ptr<platform::Render::SyncPoint> DirectStorage::SubmitUpload()
{
	auto syncpoint = std::make_shared<AwaitableSyncPoint>(adapter, fence_commit_value++, 0);
	syncpoint->SetEventOnCompletion(fence_commit_value);

	auto& fence = syncpoint->GetFence();

	file_upload_queue->EnqueueSignal(fence.GetFence(), fence_commit_value);
	file_upload_queue->Submit();
	file_references.clear();

	syncpoint->SetOnWaited([queue = this->file_upload_queue]()
		{
			DSTORAGE_ERROR_RECORD errorRecord{};
			queue->RetrieveErrorRecord(&errorRecord);
			CheckHResult(errorRecord.FirstFailure.HResult);
		});

	return syncpoint;
}

std::shared_ptr<platform_ex::DStorageFile> DirectStorage::OpenFile(const fs::path& path)
{
	platform_ex::COMPtr<IDStorageFile> file;
	CheckHResult(factory->OpenFile(path.wstring().c_str(), COMPtr_RefParam(file, IID_IDStorageFile)));

	BY_HANDLE_FILE_INFORMATION info;
	file->GetFileInformation(&info);

	return std::make_shared<DStorageFile>(file, info);
}

void DirectStorage::EnqueueRequest(const DStorageFile2MemoryRequest& req)
{
	file_references.emplace_back(req.File.Source);

	DSTORAGE_REQUEST request = {};
	request.Options.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
	request.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_MEMORY;
	request.Options.CompressionFormat = static_cast<DSTORAGE_COMPRESSION_FORMAT>(req.Compression);
	request.Source.File.Source = std::static_pointer_cast<const DStorageFile>(req.File.Source)->Get();
	request.Source.File.Offset = req.File.Offset;
	request.Source.File.Size = req.File.Size;
	request.UncompressedSize = req.Memory.Size;
	request.Destination.Memory.Buffer = req.Memory.Buffer;
	request.Destination.Memory.Size = req.Memory.Size;

	file_upload_queue->EnqueueRequest(&request);
}