#include "DirectStorage.h"
#include "Fence.h"
#include "Asset/DStorageAsset.h"
#include "Texture.h"
#include "View.h"

using namespace platform_ex::Windows::D3D12;

DEFINE_GUID(IID_IDStorageFactory,	0x6924ea0c, 0xc3cd, 0x4826, 0xb1, 0x0a, 0xf6, 0x4f, 0x4e, 0xd9, 0x27, 0xc1);
DEFINE_GUID(IID_IDStorageQueue1,		0xdd2f482c, 0x5eff, 0x41e8, 0x9c, 0x9e, 0xd2, 0x37, 0x4b, 0x27, 0x81, 0x28);
DEFINE_GUID(IID_IDStorageFile,		0x5de95e7b, 0x955a, 0x4868, 0xa7, 0x3c, 0x24, 0x3b, 0x29, 0xf4, 0xb8, 0xda);

DirectStorage::DirectStorage(D3D12Adapter* InParent)
	:adapter(InParent)
{
	DSTORAGE_CONFIGURATION1 config;
	CheckHResult(DStorageSetConfiguration1(&config));

	CheckHResult(DStorageGetFactory(COMPtr_RefParam(factory, IID_IDStorageFactory)));

#ifndef  NDEBUG
	factory->SetDebugFlags(DSTORAGE_DEBUG_BREAK_ON_ERROR | DSTORAGE_DEBUG_SHOW_ERRORS);
#endif // ! NDEBUG
	factory->SetStagingBufferSize(platform_ex::DSFileFormat::kDefaultStagingBufferSize);

	RequestNextStatusIndex();
}

uint32 DirectStorage::RequestNextStatusIndex()
{
	if (status_count >= kStatusCount)
	{
		CheckHResult(factory->CreateStatusArray(
			kStatusCount,
			nullptr,
			IID_PPV_ARGS(status_array.ReleaseAndGetAddress())));
		status_count = 0;
	}

	return status_count++;
}

void DirectStorage::CreateUploadQueue(ID3D12Device* device)
{
	{
		DSTORAGE_QUEUE_DESC queueDesc{};
		queueDesc.Capacity = DSTORAGE_MAX_QUEUE_CAPACITY;
		queueDesc.Priority = DSTORAGE_PRIORITY_NORMAL;
		queueDesc.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
		queueDesc.Device = nullptr;

		CheckHResult(factory->CreateQueue(&queueDesc, COMPtr_RefParam(memory_upload_queue, IID_IDStorageQueue1)));
	}

	{
		DSTORAGE_QUEUE_DESC queueDesc{};
		queueDesc.Capacity = DSTORAGE_MAX_QUEUE_CAPACITY;
		queueDesc.Priority = DSTORAGE_PRIORITY_NORMAL;
		queueDesc.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
		queueDesc.Device = device;

		CheckHResult(factory->CreateQueue(&queueDesc, COMPtr_RefParam(gpu_upload_queue, IID_IDStorageQueue1)));
	}
}

DirectStorage::DStorageSyncPoint::DStorageSyncPoint(platform_ex::COMPtr<IDStorageStatusArray> statusArray, uint32 index)
	:status_array(statusArray),
	status_index(index),
	complete_event(CreateEventW(nullptr, TRUE, FALSE, nullptr)),
	continue_handle(nullptr),
	wait(nullptr)
{
}

DirectStorage::DStorageSyncPoint::~DStorageSyncPoint()
{
	if (wait)
	{
		WAssert(continue_handle == nullptr, "unexcepted syncpoint destroy");

		WaitForThreadpoolWaitCallbacks(wait, TRUE);
		CloseThreadpoolWait(wait);
		wait = nullptr;
	}
}

bool DirectStorage::DStorageSyncPoint::IsReady() const
{
	return status_array->IsComplete(status_index);
}

void DirectStorage::DStorageSyncPoint::Wait()
{
	WaitForSingleObject(complete_event, INFINITE);
}

void DirectStorage::DStorageSyncPoint::AwaitSuspend(std::coroutine_handle<> handle)
{
	continue_handle = handle;

	wait = CreateThreadpoolWait([](TP_CALLBACK_INSTANCE*, void* context, TP_WAIT*, TP_WAIT_RESULT)
		{
			auto target = reinterpret_cast<DStorageSyncPoint*>(context);
			auto self_count = target->shared_from_this();
			target->continue_handle.resume();
			target->continue_handle = nullptr;
		}, this, nullptr);
	ResetEvent(complete_event);
	::SetThreadpoolWait(wait, complete_event, nullptr);
}

std::shared_ptr<platform_ex::DStorageSyncPoint> DirectStorage::SubmitUpload(platform_ex::DStorageQueueType type)
{
	auto status_index = RequestNextStatusIndex();
	auto syncpoint = std::make_shared<DStorageSyncPoint>(status_array, status_index);

	auto upload_queue = type == platform_ex::DStorageQueueType::Gpu ? gpu_upload_queue : memory_upload_queue;

	upload_queue->EnqueueSetEvent(syncpoint->complete_event);
	upload_queue->EnqueueStatus(syncpoint->status_array.Get(), syncpoint->status_index);
	upload_queue->Submit();
	file_references.clear();
	
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

	memory_upload_queue->EnqueueRequest(&request);
}

void DirectStorage::EnqueueRequest(const DStorageFile2GpuRequest& req)
{
	file_references.emplace_back(req.File.Source);

	DSTORAGE_REQUEST request = {};
	request.Options.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
	request.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_MEMORY;
	request.Options.CompressionFormat = static_cast<DSTORAGE_COMPRESSION_FORMAT>(req.Compression);
	request.Source.File.Source = std::static_pointer_cast<const DStorageFile>(req.File.Source)->Get();
	request.Source.File.Offset = req.File.Offset;
	request.Source.File.Size = req.File.Size;
	request.UncompressedSize = req.UncompressedSize;

	std::visit([&](auto&& Destination)
		{
			using T = std::decay_t<decltype(Destination)>;
			if constexpr (std::is_same_v<T, DStorageFile2GpuRequest::TextureRegion>)
			{
				auto Resouce = dynamic_cast_texture(Destination.Target);
				request.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_TEXTURE_REGION;
				request.Destination.Texture.Resource = Resouce->D3DResource();
				request.Destination.Texture.SubresourceIndex = Destination.SubresourceIndex;

				D3D12_BOX region{};
				region.right = Destination.Region.Right;
				region.bottom = Destination.Region.Bottom;
				region.back = Destination.Region.Back;
				request.Destination.Texture.Region = region;
			}
			else if constexpr (std::is_same_v<T, DStorageFile2GpuRequest::MultipleSubresources>)
			{
				auto Resouce = dynamic_cast_texture(Destination.Texture);
				request.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_MULTIPLE_SUBRESOURCES;
				request.Destination.MultipleSubresources.Resource = Resouce->D3DResource();
				request.Destination.MultipleSubresources.FirstSubresource = Destination.FirstSubresource;
			}
		}, req.Destination);

	gpu_upload_queue->EnqueueRequest(&request);
}