#include "WriteOnlyFile.h"
#include <WFramework/WCLib/NativeAPI.h>

using namespace white::coroutine;

white::coroutine::WriteOnlyFile white::coroutine::WriteOnlyFile::open(
	white::coroutine::IOScheduler& ioService,
	const std::filesystem::path& path,
	file_open_mode openMode,
	file_share_mode shareMode,
	file_buffering_mode bufferingMode)
{
	return WriteOnlyFile(file::open(
		GENERIC_WRITE,
		ioService,
		path,
		openMode,
		shareMode,
		bufferingMode));
}

white::coroutine::WriteOnlyFile::WriteOnlyFile(
	win32::handle_t&& fileHandle) noexcept
	: file(std::move(fileHandle))
	, writable_file(win32::handle_t{})
{
}