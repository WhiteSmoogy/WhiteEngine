#include "AsyncArchive.h"
#include "Runtime/Core/Coroutine/AsyncStream.h"
#include "Runtime/Core/LFile.h"
#include "System/SystemEnvironment.h"

namespace WhiteEngine
{
	class FileAsyncReadArchive : public AsyncArchive
	{
	public:
		FileAsyncReadArchive(const std::filesystem::path& path)
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

	AsyncArchive* CreateFileAsyncReader(const std::filesystem::path& filename)
	{
		return new FileAsyncReadArchive(filename);
	}

	class FileAsyncWriteArchive : public AsyncArchive
	{
	public:
		FileAsyncWriteArchive(const std::filesystem::path& path)
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

	AsyncArchive* CreateFileAsyncWriter(const std::filesystem::path& filename)
	{
		return new FileAsyncWriteArchive(filename);
	}

	class FileReadArchive :public Archive
	{
	public:
		FileReadArchive(const std::filesystem::path& path)
			:file(path.wstring(), platform::File::kToRead)
		{
			ArIsLoading = true;
		}

		int64 Tell() const override
		{
			return Offset;
		}

		void Seek(int64 offset) override
		{
			Offset = offset;
		}

		Archive& Serialize(void* v, white::uint64 length) override
		{
			auto io = file.Read(v, length,static_cast<uint64>(Offset));
			Offset += io;
			return *this;
		}
	private:
		platform::File file;
		int64 Offset = 0;
	};

	Archive* CreateFileReader(const std::filesystem::path& filename)
	{
		return new FileReadArchive(filename);
	}
}