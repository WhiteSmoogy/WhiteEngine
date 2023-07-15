#include "DirectStorage.h"
#include "Fence.h"
#include "Asset/DStorageAsset.h"
#include "Texture.h"
#include "View.h"
#include <zlib.h>
#include <spdlog/spdlog.h>

#include <WinPixEventRuntime/pix3.h>

using namespace platform_ex::Windows::D3D12;

DEFINE_GUID(IID_IDStorageFactory, 0x6924ea0c, 0xc3cd, 0x4826, 0xb1, 0x0a, 0xf6, 0x4f, 0x4e, 0xd9, 0x27, 0xc1);
DEFINE_GUID(IID_IDStorageQueue1, 0xdd2f482c, 0x5eff, 0x41e8, 0x9c, 0x9e, 0xd2, 0x37, 0x4b, 0x27, 0x81, 0x28);
DEFINE_GUID(IID_IDStorageFile, 0x5de95e7b, 0x955a, 0x4868, 0xa7, 0x3c, 0x24, 0x3b, 0x29, 0xf4, 0xb8, 0xda);


DirectStorage::DirectStorage(D3D12Adapter* InParent)
	:adapter(InParent)
{
	DSTORAGE_CONFIGURATION1 config;
	CheckHResult(DStorageSetConfiguration1(&config));

	CheckHResult(DStorageGetFactory(COMPtr_RefParam(factory, IID_IDStorageFactory)));

#ifndef NDEBUG
	factory->SetDebugFlags(DSTORAGE_DEBUG_BREAK_ON_ERROR | DSTORAGE_DEBUG_SHOW_ERRORS);
#endif // ! NDEBUG
	factory->SetStagingBufferSize(platform_ex::DSFileFormat::kDefaultStagingBufferSize);

	RequestNextStatusIndex();

	decompression_queue = factory.As<IDStorageCustomDecompressionQueue1>();
	decompression_queue_event = decompression_queue->GetEvent();

	decompression_wait = CreateThreadpoolWait(OnCustomDecompressionRequestsAvailable, this, nullptr);
	SetThreadpoolWait(decompression_wait, decompression_queue_event, nullptr);

	decompression_work = CreateThreadpoolWork(DecompressionWork, this, nullptr);
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
		queueDesc.Name = "memory_upload_queue";

		CheckHResult(factory->CreateQueue(&queueDesc, COMPtr_RefParam(memory_upload_queue, IID_IDStorageQueue1)));

		auto error_event = memory_upload_queue->GetErrorEvent();
		auto error_wait = CreateThreadpoolWait(OnQueueError, memory_upload_queue.Get(), nullptr);
		SetThreadpoolWait(error_wait, error_event, nullptr);
	}

	{
		DSTORAGE_QUEUE_DESC queueDesc{};
		queueDesc.Capacity = DSTORAGE_MAX_QUEUE_CAPACITY;
		queueDesc.Priority = DSTORAGE_PRIORITY_NORMAL;
		queueDesc.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
		queueDesc.Device = device;
		queueDesc.Name = "gpu_upload_queue";

		CheckHResult(factory->CreateQueue(&queueDesc, COMPtr_RefParam(gpu_upload_queue, IID_IDStorageQueue1)));
		auto error_event = gpu_upload_queue->GetErrorEvent();
		auto error_wait = CreateThreadpoolWait(OnQueueError, gpu_upload_queue.Get(), nullptr);
		SetThreadpoolWait(error_wait, error_event, nullptr);
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

constexpr DSTORAGE_COMPRESSION_FORMAT CUSTOM_COMPRESSION_FORMAT_ZLIB = static_cast<DSTORAGE_COMPRESSION_FORMAT>(platform_ex::DStorageCompressionFormat::Zlib);

void CALLBACK DirectStorage::DecompressionWork(TP_CALLBACK_INSTANCE*, void* pv, TP_WORK*)
{
	PIXScopedEvent(0, "OnDecompress");

	auto context = reinterpret_cast<DirectStorage*>(pv);

	DSTORAGE_CUSTOM_DECOMPRESSION_REQUEST request;
	{
		std::unique_lock lock(context->requests_mutex);

		// DecompressionWork is submitted once per request added to the deque
		wassume(!context->decompression_requests.empty());

		request = context->decompression_requests.front();
		context->decompression_requests.pop_front();
	}

	WAssert(request.CompressionFormat == CUSTOM_COMPRESSION_FORMAT_ZLIB, "only expect ZLib requests");

	// The destination maybe an upload heap (write-combined memory)
	// ZLib decompression tends to read from the destination buffer, which is extremely slow from write-combined memory.
	std::unique_ptr<uint8_t[]> scratchBuffer;
	void* decompressionDestination;

	if (request.Flags & DSTORAGE_CUSTOM_DECOMPRESSION_FLAG_DEST_IN_UPLOAD_HEAP)
	{
		scratchBuffer.reset(new uint8_t[request.DstSize]);
		decompressionDestination = scratchBuffer.get();
	}
	else
	{
		decompressionDestination = request.DstBuffer;
	}

	// Perform the actual decompression
	uLong decompressedSize = static_cast<uLong>(request.DstSize);
	int uncompressResult = uncompress(
		reinterpret_cast<Bytef*>(decompressionDestination),
		&decompressedSize,
		static_cast<Bytef const*>(request.SrcBuffer),
		static_cast<uLong>(request.SrcSize));

	if (uncompressResult == Z_OK && decompressionDestination != request.DstBuffer)
	{
		memcpy(request.DstBuffer, decompressionDestination, request.DstSize);
	}

	// Tell DirectStorage that this request has been completed.
	DSTORAGE_CUSTOM_DECOMPRESSION_RESULT result{};
	result.Id = request.Id;
	result.Result = uncompressResult == Z_OK ? S_OK : E_FAIL;

	context->decompression_queue->SetRequestResults(1, &result);
}

void CALLBACK DirectStorage::OnCustomDecompressionRequestsAvailable(TP_CALLBACK_INSTANCE*, void* pv, TP_WAIT* wait, TP_WAIT_RESULT)
{
	PIXScopedEvent(0, "OnCustomDecompressionRequestsReady");

	auto context = reinterpret_cast<DirectStorage*>(pv);

	// Loop through all requests pending requests until no more remain.
	while (true)
	{
		DSTORAGE_CUSTOM_DECOMPRESSION_REQUEST requests[64];
		uint32_t numRequests = 0;

		DSTORAGE_GET_REQUEST_FLAGS flags = DSTORAGE_GET_REQUEST_FLAG_SELECT_CUSTOM;

		// Pull off a batch of requests to process in this loop.
		CheckHResult(context->decompression_queue->GetRequests1(flags, _countof(requests), requests, &numRequests));

		if (numRequests == 0)
			break;

		std::unique_lock lock(context->requests_mutex);

		for (uint32_t i = 0; i < numRequests; ++i)
		{
			context->decompression_requests.push_back(requests[i]);
		}

		lock.unlock();

		// Submit one piece of work for each request.
		for (uint32_t i = 0; i < numRequests; ++i)
		{
			SubmitThreadpoolWork(context->decompression_work);
		}
	}

	// Re-register the custom decompression queue event with this callback to be
	// called when the next decompression requests become available for
	// processing.
	SetThreadpoolWait(wait, context->decompression_queue_event, nullptr);
}

void CALLBACK DirectStorage::OnQueueError(TP_CALLBACK_INSTANCE*, void* pv, TP_WAIT* wait, TP_WAIT_RESULT)
{
	auto queue = reinterpret_cast<IDStorageQueue1*>(pv);

	DSTORAGE_ERROR_RECORD record;
	queue->RetrieveErrorRecord(&record);

	DSTORAGE_QUEUE_INFO info;
	queue->Query(&info);

	spdlog::error("DirectStorage Queue:{} FailureCount:{} FirstFailure:{:x}", info.Desc.Name, record.FailureCount, record.FirstFailure.HResult);

	SetThreadpoolWait(wait, queue->GetErrorEvent(), nullptr);
}
