#include "AsyncArchive.h"
#include "Runtime/Core/Coroutine/AsyncStream.h"
#include "System/SystemEnvironment.h"
namespace WhiteEngine
{
	class FileReadArchive : public AsyncArchive
	{
	public:
		FileReadArchive(const std::filesystem::path& path)
			:stream(Environment->Scheduler->GetIOScheduler(),path,white::coroutine::file_share_mode::read)
			
		{
			ArIsLoading = true;
		}

		Task<AsyncArchive&> Serialize(void* v, white::uint64 length) override
		{
			co_await stream.Read(v, length);
			co_return *this;
		}
	private:
		white::coroutine::FileAsyncStream stream;
	};

	AsyncArchive* CreateFileReader(const std::filesystem::path& filename)
	{
		return new FileReadArchive(filename);
	}

	class FileWriteArchive : public AsyncArchive
	{
	public:
		FileWriteArchive(const std::filesystem::path& path)
			:stream(Environment->Scheduler->GetIOScheduler(), path, white::coroutine::file_share_mode::write)
		{
			ArIsSaving = true;
		}

		Task<AsyncArchive&> Serialize(void* v, white::uint64 length) override
		{
			co_await stream.Write(v, length);
			co_return *this;
		}
	private:
		white::coroutine::FileAsyncStream stream;
	};

	AsyncArchive* CreateFileWriter(const std::filesystem::path& filename)
	{
		return new FileWriteArchive(filename);
	}
}