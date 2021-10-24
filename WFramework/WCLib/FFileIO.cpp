#undef __STRICT_ANSI__
#include "FFileIO.h"
#include "Debug.h"
#include "NativeAPI.h"
#include "FReference.h"
#include "FileSystem.h"
#include <WBase/functional.hpp>
#include <WBase/streambuf.hpp>

#if WFL_Win32
#	if defined(__MINGW32__) && !defined(__MINGW64_VERSION_MAJOR)
// At least one headers of <stdlib.h>, <stdio.h>, <Windows.h>, <Windef.h>
//	(and probably more) should have been included to make the MinGW-W64 macro
//	available if it is really being used.
#		undef _fileno
#	endif
#include "../Win32/WCLib/Mingw32.h"
#	include  "../Win32/WCLib/NLS.h"
//	platform_ex::GetErrnoFromWin32, platform_ex::QueryFileLinks,
//	platform_ex::QueryFileNodeID, platform_ex::QueryFileTime,
//	platform_ex::WCSToUTF8, platform_ex::UTF8ToWCS, platform_ex::ConvertTime,
//	platform_ex::Windows::SetFileTime;

using platform_ex::Windows::FileAttributes;
using platform_ex::Windows::GetErrnoFromWin32;
using platform_ex::Windows::QueryFileLinks;
//@{
using platform_ex::Windows::QueryFileNodeID;
using platform_ex::Windows::QueryFileTime;
using platform_ex::Windows::UTF8ToWCS;
using platform_ex::Windows::WCSToUTF8;
using platform_ex::Windows::ConvertTime;
//@}
using platform_ex::MakePathStringW;
#elif WFL_API_POSIXFileSystem
#	include <WBase/CHRLib/CharacterProcessing.h>
#	include <dirent.h>

using namespace CHRLib;
#else
#	error "Unsupported platform found."
#endif

namespace platform
{
	static_assert(std::is_same<mode_t, white::underlying_type_t<Mode>>::value,
		"Mismatched mode types found.");

	namespace
	{
		using platform_ex::estat;
		using platform_ex::cstat;
#if WFL_Win32
		using platform_ex::ToHandle;

		void
			QueryFileTime(const char* path, ::FILETIME* p_ctime, ::FILETIME* p_atime,
				::FILETIME* p_mtime, bool follow_reparse_point)
		{
			QueryFileTime(MakePathStringW(path).c_str(), p_ctime, p_atime, p_mtime,
				follow_reparse_point);
		}
		//@{
		void
			QueryFileTime(int fd, ::FILETIME* p_ctime, ::FILETIME* p_atime,
				::FILETIME* p_mtime)
		{
			QueryFileTime(ToHandle(fd), p_ctime, p_atime, p_mtime);
	}

		// TODO: Blocked. Use C++14 generic lambda expressions.
		wconstexpr const struct
		{
			template<typename _tParam, typename... _tParams>
			FileTime
				operator()(_tParam arg, _tParams... args) const
			{
				::FILETIME atime{};

				QueryFileTime(arg, {}, &atime, {}, args...);
				return ConvertTime(atime);
			}
		} get_st_atime{};
		wconstexpr const struct
		{
			template<typename _tParam, typename... _tParams>
			FileTime
				operator()(_tParam arg, _tParams... args) const
			{
				::FILETIME mtime{};

				QueryFileTime(arg, {}, {}, &mtime, args...);
				return ConvertTime(mtime);
			}
		} get_st_mtime{};
		wconstexpr const struct
		{
			template<typename _tParam, typename... _tParams>
			array<FileTime, 2>
				operator()(_tParam arg, _tParams... args) const
			{
				::FILETIME mtime{}, atime{};

				QueryFileTime(arg, {}, &atime, &mtime, args...);
				return array<FileTime, 2>{ConvertTime(mtime), ConvertTime(atime)};
			}
		} get_st_matime{};
		//@}

		//! \since build 639
		//@{
		WB_NONNULL(1) bool
			UnlinkWithAttr(const wchar_t* path, FileAttributes attr) wnothrow
		{
			return !(attr & FileAttributes::ReadOnly) || ::SetFileAttributesW(path,
				attr & ~FileAttributes::ReadOnly) ? ::_wunlink(path) == 0
				: (errno = GetErrnoFromWin32(), false);
		}

		template<typename _func>
		WB_NONNULL(2) bool
			CallFuncWithAttr(_func f, const char* path)
		{
			const auto& wpath(MakePathStringW(path));
			const auto& wstr(wpath.c_str());
			const auto attr(FileAttributes(::GetFileAttributesW(wstr)));

			return attr != platform_ex::Windows::Invalid ? f(wstr, attr)
				: (errno = GetErrnoFromWin32(), false);
		}
		//@}
#else
		inline PDefH(::timespec, ToTimeSpec, FileTime ft) wnothrow
			ImplRet({ std::time_t(ft.count() / 1000000000LL),
				long(ft.count() % 1000000000LL) })

			//! \since build 719
			void
			SetFileTime(int fd, const ::timespec(&times)[2])
		{
#if YCL_DS
			// XXX: Hack.
#ifndef UTIME_OMIT
#	define UTIME_OMIT (-1L)
#endif
			wunused(fd), wunused(times);
			white::throw_error(std::errc::function_not_supported, wfsig);
#else
			WCL_CallF_CAPI(, ::futimens, fd, times);
#endif
		}

		//! \since build 631
		//@{
		const auto get_st_atime([](struct ::stat& st) {
			return FileTime(st.st_atime);
		});
		const auto get_st_mtime([](struct ::stat& st) {
			return FileTime(st.st_mtime);
		});
		const auto get_st_matime([](struct ::stat& st) {
			return array<FileTime, 2>{FileTime(st.st_mtime), FileTime(st.st_atime)};
		});
		//@}

		//! \since build 638
		//@{
		static_assert(std::is_integral<::dev_t>(),
			"Nonconforming '::dev_t' type found.");
		static_assert(std::is_unsigned<::ino_t>(),
			"Nonconforming '::ino_t' type found.");

		inline PDefH(FileNodeID, get_file_node_id, struct ::stat& st) wnothrow
			ImplRet({ std::uint64_t(st.st_dev), std::uint64_t(st.st_ino) })
			//@}
			//! \since build 719
			inline WB_NONNULL(2, 4) PDefH(void, cstat, struct ::stat& st,
				const char16_t* path, bool follow_link, const char* sig)
			ImplRet(cstat(st, MakePathString(path).c_str(), follow_link, sig))
			//! \since build 632
			inline WB_NONNULL(2) PDefH(int, estat, struct ::stat& st, const char16_t* path,
				bool follow_link)
			ImplRet(estat(st, MakePathString(path).c_str(), follow_link))
#endif

			//! \since build 632
			template<typename _func, typename... _tParams>
		auto
			FetchFileTime(_func f, _tParams... args)
#if WFL_Win32
			-> white::invoke_result_t<_func,_tParams&...>
#else
			-> white::invoke_result_t<_func,struct ::stat&>
#endif
		{
#if WFL_Win32
			// NOTE: The %::FILETIME has resolution of 100 nanoseconds.
			// XXX: Error handling for indirect calls.
			return f(args...);
#else
			// TODO: Get more precise time count.
			struct ::stat st;

			cstat(st, args..., wfsig);
			return f(st);
#endif
		}

		//! \since build 765
		template<int _vErr, typename _fCallable, class _tObj, typename _tByteBuf,
			typename... _tParams>
			_tByteBuf
			FullReadWrite(_fCallable f, _tObj&& obj, _tByteBuf ptr, size_t nbyte,
				_tParams&&... args)
		{
			while (nbyte > 0)
			{
				const auto n(white::invoke(f, obj, ptr, nbyte, yforward(args)...));

				if (n == size_t(-1))
					break;
				if (n == 0)
				{
					errno = _vErr;
					break;
				}
				wunseq(ptr += n, nbyte -= n);
			}
			return ptr;
		}

		//@{
#if WFL_Win64
		using rwsize_t = unsigned;
#else
		using rwsize_t = size_t;
#endif

		template<typename _func, typename _tBuf>
		size_t
			SafeReadWrite(_func f, int fd, _tBuf buf, rwsize_t nbyte) wnothrowv
		{
			WAssert(fd != -1, "Invalid file descriptor found.");
			return size_t(RetryOnInterrupted([&] {
				return f(fd, Nonnull(buf), nbyte);
			}));
		}
		//@}

#if !YCL_DS
		WB_NONNULL(2) void
			UnlockFileDescriptor(int fd, const char* sig) wnothrowv
		{
#	if WFL_Win32
			const auto res(platform_ex::Windows::UnlockFile(ToHandle(fd)));

			if WB_UNLIKELY(!res)
				WCL_Trace_Win32E(Descriptions::Warning, UnlockFileEx, sig);
			WAssert(res, "Narrow contract violated.");
#	else
			const auto res(YCL_TraceCall_CAPI(::flock, sig, fd, LOCK_UN));

			WAssert(res == 0, "Narrow contract violated.");
			wunused(res);
#	endif
		}
#endif

		bool
			IsNodeShared_Impl(const char* a, const char* b, bool follow_link) wnothrow
		{
#if WFL_Win32
			return CallNothrow({}, [=] {
				return QueryFileNodeID(MakePathStringW(a).c_str(), follow_link)
					== QueryFileNodeID(MakePathStringW(b).c_str(), follow_link);
			});
#else
			struct ::stat st;

			if (estat(st, a, follow_link) != 0)
				return {};

			const auto id(get_file_node_id(st));

			if (estat(st, b, follow_link) != 0)
				return {};

			return IsNodeShared(id, get_file_node_id(st));
#endif
		}
		bool
			IsNodeShared_Impl(const char16_t* a, const char16_t* b, bool follow_link)
			wnothrow
		{
#if WFL_Win32
			return CallNothrow({}, [=] {
				return QueryFileNodeID(wcast(a), follow_link)
					== QueryFileNodeID(wcast(b), follow_link);
			});
#else
			return IsNodeShared_Impl(MakePathString(a).c_str(),
				MakePathString(b).c_str(), follow_link);
#endif
		}

		//! \since build 701
		template<typename _tChar>
		WB_NONNULL(1, 2) pair<UniqueFile, UniqueFile>
			HaveSameContents_Impl(const _tChar* path_a, const _tChar* path_b, mode_t mode)
		{
			if (UniqueFile p_a{ uopen(path_a, int(OpenMode::ReadRaw), mode) })
				if (UniqueFile p_b{ uopen(path_b, int(OpenMode::ReadRaw), mode) })
					return { std::move(p_a), std::move(p_b) };
			return {};
		}

} // unnamed namespace;


	string
		MakePathString(const char16_t* s)
	{
#if WFL_Win32
		return platform_ex::Windows::WCSToUTF8(wcast(s));
#else
		return CHRLib::MakeMBCS(s);
#endif
	}


	NodeCategory
		CategorizeNode(mode_t st_mode) wnothrow
	{
		auto res(NodeCategory::Empty);
		const auto m(Mode(st_mode) & Mode::FileType);

		if ((m & Mode::Directory) == Mode::Directory)
			res |= NodeCategory::Directory;
#if !WFL_Win32
		if ((m & Mode::Link) == Mode::Link)
			res |= NodeCategory::Link;
#endif
		if ((m & Mode::Regular) == Mode::Regular)
			res |= NodeCategory::Regular;
		if WB_UNLIKELY((m & Mode::Character) == Mode::Character)
			res |= NodeCategory::Character;
#if !WFL_Win32
		else if WB_UNLIKELY((m & Mode::Block) == Mode::Block)
			res |= NodeCategory::Block;
#endif
		if WB_UNLIKELY((m & Mode::FIFO) == Mode::FIFO)
			res |= NodeCategory::FIFO;
#if !WFL_Win32
		if ((m & Mode::Socket) == Mode::Socket)
			res |= NodeCategory::Socket;
#endif
		return res;
	}


	void
		FileDescriptor::Deleter::operator()(pointer p) const wnothrow
	{
		if (p && WCL_CallGlobal(close, *p) < 0)
			// NOTE: This is not a necessarily error. See $2016-04
			//	@ %Documentation::Workflow::Annual2016.
			TraceDe(Descriptions::Warning,
				"Falied closing file descriptor, errno = %d.", errno);
	}


	FileDescriptor::FileDescriptor(std::FILE* fp) wnothrow
		: desc(fp ? WCL_CallGlobal(fileno, fp) : -1)
	{}

	FileTime
		FileDescriptor::GetAccessTime() const
	{
		return FetchFileTime(get_st_atime, desc);
	}
	NodeCategory
		FileDescriptor::GetCategory() const wnothrow
	{
#if WFL_Win32
		return platform_ex::Windows::CategorizeNode(ToHandle(desc));
#else
		struct ::stat st;

		return estat(st, desc) == 0 ? CategorizeNode(st.st_mode)
			: NodeCategory::Invalid;
#endif
	}
	int
		FileDescriptor::GetFlags() const
	{
#if WCL_API_POSIXFileSystem
		return WCL_CallF_CAPI(, ::fcntl, desc, F_GETFL);
#else
		white::throw_error(std::errc::function_not_supported, wfsig);
#endif
	}
	mode_t
		FileDescriptor::GetMode() const
	{
		struct ::stat st;

		cstat(st, desc, wfsig);
		return st.st_mode;
	}
	FileTime
		FileDescriptor::GetModificationTime() const
	{
		return FetchFileTime(get_st_mtime, desc);
	}
	array<FileTime, 2>
		FileDescriptor::GetModificationAndAccessTime() const
	{
		return FetchFileTime(get_st_matime, desc);
	}
	FileNodeID
		FileDescriptor::GetNodeID() const wnothrow
	{
#if WFL_Win32
		return CallNothrow({}, [=] {
			return QueryFileNodeID(ToHandle(desc));
		});
#else
		struct ::stat st;

		return estat(st, desc) == 0 ? get_file_node_id(st) : FileNodeID();
#endif
	}
	size_t
		FileDescriptor::GetNumberOfLinks() const wnothrow
	{
#if WFL_Win32
		return CallNothrow({}, [=] {
			return QueryFileLinks(ToHandle(desc));
		});
#else
		struct ::stat st;
		static_assert(std::is_unsigned<decltype(st.st_nlink)>(),
			"Unsupported 'st_nlink' type found.");

		return estat(st, desc) == 0 ? size_t(st.st_nlink) : size_t();
#endif
	}
	std::uint64_t
		FileDescriptor::GetSize() const
	{
#if WFL_Win32
		return platform_ex::Windows::QueryFileSize(ToHandle(desc));
#else
		struct ::stat st;

		WCL_CallF_CAPI(, ::fstat, desc, &st);

		// XXX: No negative file size should be found. See also:
		//	http://stackoverflow.com/questions/12275831/why-is-the-st-size-field-in-struct-stat-signed.
		if (st.st_size >= 0)
			return std::uint64_t(st.st_size);
		throw std::invalid_argument("Negative file size found.");
#endif
	}

	void
		FileDescriptor::SetAccessTime(FileTime ft) const
	{
#if WFL_Win32
		auto atime(ConvertTime(ft));

		platform_ex::Windows::SetFileTime(ToHandle(desc), {}, &atime, {});
#else
		const ::timespec times[]{ ToTimeSpec(ft),{ wimpl(0), UTIME_OMIT } };

		SetFileTime(desc, times);
#endif
	}
	bool
		FileDescriptor::SetBlocking() const
	{
#if WCL_API_POSIXFileSystem
		// NOTE: Read-modify-write operation is needed for compatibility.
		//	See http://pubs.opengroup.org/onlinepubs/9699919799/functions/fcntl.html.
		const auto oflags((OpenMode(GetFlags())));

		return bool(oflags & OpenMode::Nonblocking)
			? (SetFlags(int(oflags & ~OpenMode::Nonblocking)), true) : false;
#else
		white::throw_error(std::errc::function_not_supported, wfsig);
#endif
	}
	void
		FileDescriptor::SetFlags(int flags) const
	{
#if WCL_API_POSIXFileSystem
		WCL_CallF_CAPI(, ::fcntl, desc, F_SETFL, flags);
#else
		// TODO: Try using NT6 %::SetFileInformationByHandle for Win32.
		wunused(flags);
		white::throw_error(std::errc::function_not_supported, wfsig);
#endif
	}
	void
		FileDescriptor::SetMode(mode_t mode) const
	{
#if WCL_API_POSIXFileSystem
		WCL_CallF_CAPI(, ::fchmod, desc, mode);
#else
		wunused(mode);
		white::throw_error(std::errc::function_not_supported, wfsig);
#endif
	}
	void
		FileDescriptor::SetModificationTime(FileTime ft) const
	{
#if WFL_Win32
		auto mtime(ConvertTime(ft));

		platform_ex::Windows::SetFileTime(ToHandle(desc), {}, {}, &mtime);
#else
		const ::timespec times[]{ { wimpl(0), UTIME_OMIT }, ToTimeSpec(ft) };

		SetFileTime(desc, times);
#endif
	}
	void
		FileDescriptor::SetModificationAndAccessTime(array<FileTime, 2> fts) const
	{
#if WFL_Win32
		auto atime(ConvertTime(fts[0])), mtime(ConvertTime(fts[1]));

		platform_ex::Windows::SetFileTime(ToHandle(desc), {}, &atime, &mtime);
#else
		const ::timespec times[]{ ToTimeSpec(fts[0]), ToTimeSpec(fts[1]) };

		SetFileTime(desc, times);
#endif
	}
	bool
		FileDescriptor::SetNonblocking() const
	{
#if WCL_API_POSIXFileSystem
		// NOTE: Read-modify-write operation is need for compatibility.
		//	See http://pubs.opengroup.org/onlinepubs/9699919799/functions/fcntl.html.
		const auto oflags((OpenMode(GetFlags())));

		return !bool(oflags & OpenMode::Nonblocking)
			? (SetFlags(int(oflags | OpenMode::Nonblocking)), true) : false;
#else
		white::throw_error(std::errc::function_not_supported, wfsig);
#endif
	}
	bool
		FileDescriptor::SetSize(size_t size) wnothrow
	{
#if WFL_Win32
		return ::_chsize(desc, long(size)) == 0;
#else
		// FIXME: It seems that on Android 32-bit size is always used even if
		//	'_FILE_OFFSET_BITS == 1'. Thus it is not safe and would better to be
		//	ignored slightly.
		return ::ftruncate(desc, ::off_t(size)) == 0;
#endif
	}
	int
		FileDescriptor::SetTranslationMode(int mode) const wnothrow
	{
		// TODO: Other platforms.
#if WFL_Win32 || (defined(_NEWLIB_VERSION) && defined(__SCLE))
		return WCL_CallGlobal(setmode, desc, mode);
#else
		// NOTE: No effect.
		wunused(mode);
		return 0;
#endif
	}

	void
		FileDescriptor::Flush()
	{
#if WFL_Win32
		WCL_CallF_CAPI(, ::_commit, desc);
#else
		// XXX: http://austingroupbugs.net/view.php?id=672.
		WCL_CallF_CAPI(, ::fsync, desc);
#endif
	}

	size_t
		FileDescriptor::FullRead(void* buf, size_t nbyte) wnothrowv
	{
		using namespace std::placeholders;
		const auto p_buf(static_cast<char*>(buf));

		return size_t(FullReadWrite<0>(&FileDescriptor::Read, this, p_buf, nbyte)
			- p_buf);
	}

	size_t
		FileDescriptor::FullWrite(const void* buf, size_t nbyte) wnothrowv
	{
		using namespace std::placeholders;
		const auto p_buf(static_cast<const char*>(buf));

		return size_t(FullReadWrite<ENOSPC>(&FileDescriptor::Write, this, p_buf,
			nbyte) - p_buf);
	}

	size_t
		FileDescriptor::Read(void* buf, size_t nbyte) wnothrowv
	{
		return SafeReadWrite(WCL_ReservedGlobal(read), desc, buf, rwsize_t(nbyte));
	}

	size_t
		FileDescriptor::Write(const void* buf, size_t nbyte) wnothrowv
	{
		return SafeReadWrite(WCL_ReservedGlobal(write), desc, buf, rwsize_t(nbyte));
	}

	void
		FileDescriptor::WriteContent(FileDescriptor ofd, FileDescriptor ifd,
			byte* buf, size_t size)
	{
		WAssertNonnull(ifd),
			WAssertNonnull(ofd),
			WAssertNonnull(buf),
			WAssert(size != 0, "Invalid size found.");

		white::retry_on_cond([&](size_t len) {
			if (len == size_t(-1))
				WCL_Raise_SysE(, "Failed reading source file '" + to_string(*ifd),
					wfsig);
			if (ofd.FullWrite(buf, len) == size_t(-1))
				WCL_Raise_SysE(,
					"Failed writing destination file '" + to_string(*ofd), wfsig);
			return len != 0;
		}, &FileDescriptor::Read, &ifd, buf, size);
	}
	void
		FileDescriptor::WriteContent(FileDescriptor ofd, FileDescriptor ifd,
			size_t size)
	{
		white::temporary_buffer<byte> buf(size);

		WriteContent(ofd, ifd, buf.get().get(), buf.size());
	}

	void
		FileDescriptor::lock()
	{
#if YCL_DS
#elif WFL_Win32
		platform_ex::Windows::LockFile(ToHandle(desc));
#else
		WCL_CallF_CAPI(, ::flock, desc, LOCK_EX);
#endif
	}

	void
		FileDescriptor::lock_shared()
	{
#if YCL_DS
#elif WFL_Win32
		platform_ex::Windows::LockFile(ToHandle(desc), 0, std::uint64_t(-1), {});
#else
		WCL_CallF_CAPI(, ::flock, desc, LOCK_SH);
#endif
	}

	bool
		FileDescriptor::try_lock() wnothrowv
	{
#if YCL_DS
		return true;
#elif WFL_Win32
		return platform_ex::Windows::TryLockFile(ToHandle(desc), 0, std::uint64_t(-1));
#else
		return YCL_TraceCallF_CAPI(::flock, desc, LOCK_EX | LOCK_NB) != -1;
#endif
	}

	bool
		FileDescriptor::try_lock_shared() wnothrowv
	{
#if YCL_DS
		return true;
#elif WFL_Win32
		return platform_ex::Windows::TryLockFile(ToHandle(desc), 0, std::uint64_t(-1), {});
#else
		return YCL_TraceCallF_CAPI(::flock, desc, LOCK_SH | LOCK_NB) != -1;
#endif
	}

	void
		FileDescriptor::unlock() wnothrowv
	{
#if YCL_DS
#else
		UnlockFileDescriptor(desc, wfsig);
#endif
	}

	void
		FileDescriptor::unlock_shared() wnothrowv
	{
#if YCL_DS
#else
		UnlockFileDescriptor(desc, wfsig);
#endif
	}


	mode_t
		DefaultPMode() wnothrow
	{
#if WFL_Win32
		// XXX: For compatibility with newer version of MSVCRT, no %_umask call
		//	would be considered. See https://msdn.microsoft.com/en-us/library/z0kc8e3z.aspx.
		return mode_t(Mode::UserReadWrite);
#else
		return mode_t(Mode::UserReadWrite | Mode::GroupReadWrite | Mode::OtherRead);
#endif
	}

	void
		SetBinaryIO(std::FILE* stream) wnothrowv
	{
#if WFL_Win32
		FileDescriptor(Nonnull(stream)).SetTranslationMode(int(OpenMode::Binary));
#else
		// NOTE: No effect.
		Nonnull(stream);
#endif
	}


	int
		omode_conv(std::ios_base::openmode mode) wnothrow
	{
		using namespace std;
		auto res(OpenMode::None);

		switch (unsigned((mode &= ~ios_base::ate)
			& ~(ios_base::binary | ios_nocreate | ios_noreplace)))
		{
		case ios_base::out:
			res = OpenMode::Create | OpenMode::Write;
			break;
		case ios_base::out | ios_base::trunc:
			res = OpenMode::Create | OpenMode::WriteTruncate;
			break;
		case ios_base::out | ios_base::app:
		case ios_base::app:
			res = OpenMode::Create | OpenMode::WriteAppend;
			break;
		case ios_base::in:
			res = OpenMode::Read;
			break;
		case ios_base::in | ios_base::out:
			res = OpenMode::ReadWrite;
			break;
		case ios_base::in | ios_base::out | ios_base::trunc:
			res = OpenMode::Create | OpenMode::ReadWriteTruncate;
			break;
		case ios_base::in | ios_base::out | ios_base::app:
		case ios_base::in | ios_base::app:
			res = OpenMode::Create | OpenMode::ReadWriteAppend;
			break;
		default:
			return int(res);
		}
		// XXX: Order is significant.
		if (mode & ios_noreplace)
			res |= OpenMode::Create | OpenMode::Exclusive;
		// NOTE: %O_EXCL without %O_CREAT leads to undefined behavior in POSIX.
		if (mode & ios_nocreate)
			res &= ~OpenMode::Create | OpenMode::Exclusive;
		return int(res);
	}

	int
		omode_convb(std::ios_base::openmode mode) wnothrow
	{
		return omode_conv(mode)
			| int(mode & std::ios_base::binary ? OpenMode::Binary : OpenMode::Text);
	}


	int
		uaccess(const char* path, int amode) wnothrowv
	{
		WAssertNonnull(path);
#if WFL_Win32
		return CallNothrow(-1, [=] {
			return ::_waccess(MakePathStringW(path).c_str(), amode);
		});
#else
		return ::access(path, amode);
#endif
	}
	int
		uaccess(const char16_t* path, int amode) wnothrowv
	{
		WAssertNonnull(path);
#if WFL_Win32
		return ::_waccess(wcast(path), amode);
#else
		return CallNothrow(-1, [=] {
			return ::access(MakePathString(path).c_str(), amode);
		});
#endif
	}

	int
		uopen(const char* filename, int oflag, mode_t pmode) wnothrowv
	{
		WAssertNonnull(filename);
#if WFL_Win32
		return CallNothrow(-1, [=] {
			return ::_wopen(MakePathStringW(filename).c_str(), oflag, int(pmode));
		});
#else
		return ::open(filename, oflag, pmode);
#endif
	}
	int
		uopen(const char16_t* filename, int oflag, mode_t pmode) wnothrowv
	{
		WAssertNonnull(filename);
#if WFL_Win32
		return ::_wopen(wcast(filename), oflag, int(pmode));
#else
		return CallNothrow(-1, [=] {
			return ::open(MakePathString(filename).c_str(), oflag, pmode);
		});
#endif
	}

	std::FILE*
		ufopen(const char* filename, const char* mode) wnothrowv
	{
		WAssertNonnull(filename);
		WAssert(Deref(mode) != char(), "Invalid argument found.");
#if WFL_Win32
		return CallNothrow({}, [=] {
			return ::_wfopen(MakePathStringW(filename).c_str(),
				MakePathStringW(mode).c_str());
		});
#else
		return std::fopen(filename, mode);
#endif
	}
	std::FILE*
		ufopen(const char16_t* filename, const char16_t* mode) wnothrowv
	{
		WAssertNonnull(filename);
		WAssert(Deref(mode) != char(), "Invalid argument found.");
#if WFL_Win32
		return ::_wfopen(wcast(filename), wcast(mode));
#else
		return CallNothrow({}, [=] {
			return std::fopen(MakePathString(filename).c_str(),
				MakePathString(mode).c_str());
		});
#endif
	}
	std::FILE*
		ufopen(const char* filename, std::ios_base::openmode mode) wnothrowv
	{
		if (const auto c_mode = white::openmode_conv(mode))
			return ufopen(filename, c_mode);
		return {};
	}
	std::FILE*
		ufopen(const char16_t* filename, std::ios_base::openmode mode) wnothrowv
	{
		WAssertNonnull(filename);
		if (const auto c_mode = white::openmode_conv(mode))
			return CallNothrow({}, [=] {
#if WFL_Win32
			return ::_wfopen(wcast(filename), MakePathStringW(c_mode).c_str());
#else
			return std::fopen(MakePathString(filename).c_str(), c_mode);
#endif
		});
		return {};
	}

	bool
		ufexists(const char* filename) wnothrowv
	{
#if WFL_Win32
		return white::call_value_or(white::compose(std::fclose,
			white::addrof<>()), ufopen(filename, "rb"), wimpl(1)) == 0;
#else
		return white::fexists(filename);
#endif
	}
	bool
		ufexists(const char16_t* filename) wnothrowv
	{
		return white::call_value_or(white::compose(std::fclose,
			white::addrof<>()), ufopen(filename, u"rb"), wimpl(1)) == 0;
	}

	char*
		ugetcwd(char* buf, size_t size) wnothrowv
	{
		WAssertNonnull(buf);
		if (size != 0)
		{
			using namespace std;

#if WFL_Win32
			try
			{
				const auto p(make_unique<wchar_t[]>(size));

				if (const auto cwd = ::_wgetcwd(p.get(), int(size)))
				{
					const auto res(MakePathString(ucast(cwd)));
					const auto len(res.length());

					if (size < len + 1)
						errno = ERANGE;
					else
						return white::ntctscpy(buf, res.data(), len);
				}
			}
			CatchExpr(std::bad_alloc&, errno = ENOMEM);
#else
			// NOTE: POSIX.1 2004 has no guarantee about slashes. POSIX.1 2013
			//	mandates there are no redundant slashes. See http://pubs.opengroup.org/onlinepubs/009695399/functions/getcwd.html.
			return ::getcwd(buf, size);
#endif
		}
		else
			errno = EINVAL;
		return {};
	}
	char16_t*
		ugetcwd(char16_t* buf, size_t len) wnothrowv
	{
		WAssertNonnull(buf);
		if (len != 0)
		{
			using namespace std;

#if WFL_Win32
			// NOTE: Win32 guarantees there will be a separator if and only if when
			//	the result is root directory for ::_wgetcwd, and actually it is
			//	the same in ::%GetCurrentDirectoryW.
			const auto n(::GetCurrentDirectoryW(static_cast<DWORD>(len), wcast(buf)));

			if (n != 0)
				return buf;
			errno = GetErrnoFromWin32();
			return {};
#else
			// XXX: Alias by %char array is safe. 
			if (const auto cwd
				= ::getcwd(white::aligned_store_cast<char*>(buf), len))
				try
			{
				const auto res(platform_ex::MakePathStringU(cwd));
				const auto rlen(res.length());

				if (len < rlen + 1)
					errno = ERANGE;
				else
					return white::ntctscpy(buf, res.data(), rlen);
			}
			CatchExpr(std::bad_alloc&, errno = ENOMEM)
#endif
		}
		else
			errno = EINVAL;
		return {};
	}

#define YCL_Impl_FileSystem_ufunc_1(_n) \
bool \
_n(const char* path) wnothrowv \
{ \
	WAssertNonnull(path); \

#if WFL_Win32
#	define YCL_Impl_FileSystem_ufunc_2(_fn, _wfn) \
	return CallNothrow({}, [&]{ \
		return _wfn(MakePathStringW(path).c_str()) == 0; \
	}); \
}
#else
#	define YCL_Impl_FileSystem_ufunc_2(_fn, _wfn) \
	return _fn(path) == 0; \
}
#endif

#define YCL_Impl_FileSystem_ufunc(_n, _fn, _wfn) \
	YCL_Impl_FileSystem_ufunc_1(_n) \
	YCL_Impl_FileSystem_ufunc_2(_fn, _wfn)

	YCL_Impl_FileSystem_ufunc(uchdir, ::chdir, ::_wchdir)

		YCL_Impl_FileSystem_ufunc_1(umkdir)
#if WFL_Win32
		YCL_Impl_FileSystem_ufunc_2(_unused_, ::_wmkdir)
#else
		return ::mkdir(path, mode_t(Mode::Access)) == 0;
}
#endif

YCL_Impl_FileSystem_ufunc(urmdir, ::rmdir, ::_wrmdir)

YCL_Impl_FileSystem_ufunc_1(uunlink)
#if WFL_Win32
return CallNothrow({}, [=] {
	return CallFuncWithAttr(UnlinkWithAttr, path);
});
}
#else
YCL_Impl_FileSystem_ufunc_2(::unlink, )
#endif


YCL_Impl_FileSystem_ufunc_1(uremove)
#if WFL_Win32
// NOTE: %::_wremove is same to %::_wunlink on Win32 which cannot delete
//	empty directories.
return CallNothrow({}, [=] {
	return CallFuncWithAttr([](const wchar_t* wstr, FileAttributes attr)
		WB_NONNULL(1) wnothrow {
		return attr & FileAttributes::Directory ? ::_wrmdir(wstr) == 0
			: UnlinkWithAttr(wstr, attr);
	}, path);
});
}
#else
YCL_Impl_FileSystem_ufunc_2(std::remove, )
#endif

#undef YCL_Impl_FileSystem_ufunc_1
#undef YCL_Impl_FileSystem_ufunc_2
#undef YCL_Impl_FileSystem_ufunc


#if WFL_Win32
template<>
WF_API string
FetchCurrentWorkingDirectory(size_t init)
{
	return MakePathString(FetchCurrentWorkingDirectory<char16_t>(init));
}
template<>
WF_API u16string
FetchCurrentWorkingDirectory(size_t)
{
	u16string res;
	unsigned long len, rlen(0);

	// NOTE: Retry is necessary to prevent failure due to modification of
	//	current directory from other threads.
	white::retry_on_cond([&]() -> bool {
		if (rlen < len)
		{
			res.pop_back();
			return {};
		}
		if (rlen != 0)
			return true;
		WCL_Raise_Win32E("GetCurrentDirectoryW", wfsig);
	}, [&] {
		if ((len = ::GetCurrentDirectoryW(0, {})) != 0)
		{
			res.resize(size_t(len + 1));
			rlen = ::GetCurrentDirectoryW(len, wcast(&res[0]));
		}
	});
	res.resize(rlen);
	return res;
}
#endif


FileTime
GetFileAccessTimeOf(const char* filename, bool follow_link)
{
	return FetchFileTime(get_st_atime, filename, follow_link);
}
FileTime
GetFileAccessTimeOf(const char16_t* filename, bool follow_link)
{
	return FetchFileTime(get_st_atime, wcast(filename), follow_link);
}

FileTime
GetFileModificationTimeOf(const char* filename, bool follow_link)
{
	return FetchFileTime(get_st_mtime, filename, follow_link);
}
FileTime
GetFileModificationTimeOf(const char16_t* filename, bool follow_link)
{
	return FetchFileTime(get_st_mtime, wcast(filename), follow_link);
}

array<FileTime, 2>
GetFileModificationAndAccessTimeOf(const char* filename, bool follow_link)
{
	return FetchFileTime(get_st_matime, filename, follow_link);
}
array<FileTime, 2>
GetFileModificationAndAccessTimeOf(const char16_t* filename, bool follow_link)
{
	return FetchFileTime(get_st_matime, wcast(filename), follow_link);
}

WB_NONNULL(1) size_t
FetchNumberOfLinks(const char* path, bool follow_link)
{
#if WFL_Win32
	return QueryFileLinks(MakePathStringW(path).c_str(), follow_link);
#else
	struct ::stat st;
	static_assert(std::is_unsigned<decltype(st.st_nlink)>(),
		"Unsupported 'st_nlink' type found.");

	cstat(st, path, follow_link, wfsig);
	return st.st_nlink;
#endif
}
WB_NONNULL(1) size_t
FetchNumberOfLinks(const char16_t* path, bool follow_link)
{
#if WFL_Win32
	return QueryFileLinks(wcast(path), follow_link);
#else
	return FetchNumberOfLinks(MakePathString(path).c_str(), follow_link);
#endif
}


UniqueFile
EnsureUniqueFile(const char* dst, mode_t mode, size_t allowed_links,
	bool share)
{
	static wconstexpr const auto oflag(OpenMode::Create
		| OpenMode::WriteTruncate | OpenMode::Binary);

	mode &= mode_t(Mode::User);
	if (UniqueFile
		p_file{ uopen(dst, int(oflag | OpenMode::Exclusive), mode) })
		return p_file;
	if (allowed_links != 0 && errno == EEXIST)
	{
		const auto n_links(FetchNumberOfLinks(dst, true));

		// FIXME: Blocked. TOCTTOU access.
		if (!(allowed_links < n_links)
			&& (n_links == 1 || share || uunlink(dst) || errno == ENOENT))
			if (UniqueFile p_file{ uopen(dst, int(oflag), mode) })
				return p_file;
	}
	WCL_Raise_SysE(, "::open", wfsig);
}

bool
HaveSameContents(const char* path_a, const char* path_b, mode_t mode)
{
	auto pr(HaveSameContents_Impl(path_a, path_b, mode));

	return pr.first ? HaveSameContents(std::move(pr.first),
		std::move(pr.second), path_a, path_b) : false;
}
bool
HaveSameContents(const char16_t* path_a, const char16_t* path_b, mode_t mode)
{
	auto pr(HaveSameContents_Impl(path_a, path_b, mode));

	// TODO: Support names in exception messages.
	return pr.first ? HaveSameContents(std::move(pr.first),
		std::move(pr.second), {}, {}) : false;
}
bool
HaveSameContents(UniqueFile p_a, UniqueFile p_b, const char* name_a,
	const char* name_b)
{
	if (!(bool(Nonnull(p_a)->GetCategory() & NodeCategory::Directory)
		|| bool(Nonnull(p_b)->GetCategory() & NodeCategory::Directory)))
	{
		if (IsNodeShared(p_a.get(), p_b.get()))
			return true;

		filebuf fb_a, fb_b;

		errno = 0;
		// FIXME: Implement for streams without open-by-raw-file extension.
		// TODO: Throw a nested exception with %errno if 'errno != 0'.
		if (!fb_a.open(std::move(p_a),
			std::ios_base::in | std::ios_base::binary))
			white::throw_error(std::errc::bad_file_descriptor,
				name_a ? ("Failed opening first file '" + string(name_a) + '\'')
				.c_str() : "Failed opening first file");
		// TODO: Throw a nested exception with %errno if 'errno != 0'.
		if (!fb_b.open(std::move(p_b),
			std::ios_base::in | std::ios_base::binary))
			white::throw_error(std::errc::bad_file_descriptor,
				name_b ? ("Failed opening second file '" + string(name_b)
					+ '\'').c_str() : "Failed opening second file");
		return white::streambuf_equal(fb_a, fb_b);
	}
	return {};
}

bool
IsNodeShared(const char* a, const char* b, bool follow_link) wnothrowv
{
	return a == b || white::ntctscmp(Nonnull(a), Nonnull(b)) == 0
		|| IsNodeShared_Impl(a, b, follow_link);
}
bool
IsNodeShared(const char16_t* a, const char16_t* b, bool follow_link) wnothrowv
{
	return a == b || white::ntctscmp(Nonnull(a), Nonnull(b)) == 0
		|| IsNodeShared_Impl(a, b, follow_link);
}
bool
IsNodeShared(FileDescriptor x, FileDescriptor y) wnothrow
{
	const auto id(x.GetNodeID());

	return id != FileNodeID() && id == y.GetNodeID();
}

} // namespace platform;

namespace platform_ex
{

#if WFL_Win32
	wstring
		MakePathStringW(const char* s)
	{
		return platform_ex::Windows::UTF8ToWCS(s);
	}
#else
	u16string
		MakePathStringU(const char* s)
	{
		return CHRLib::MakeUCS2LE(s);
	}
#endif

} // namespace platform_ex;
