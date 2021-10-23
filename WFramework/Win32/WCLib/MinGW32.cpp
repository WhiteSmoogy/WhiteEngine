#include "Mingw32.h"
#if WFL_Win32
#include "../../WCLib/FileSystem.h"
#include "Registry.h"
#include "../../Core/WCoreUtilities.h"
#include <WBase/container.hpp>
#include <WBase/scope_gurad.hpp>
#include "NLS.h"
using namespace white;
using platform::NodeCategory;
#endif
namespace platform_ex {
#if WFL_Win32

	namespace Windows {
		int
			ConvertToErrno(ErrorCode err) wnothrow
		{
			// NOTE: This mapping is from universal CRT in Windows SDK 10.0.10150.0,
			//	ucrt/misc/errno.cpp, except for fix of the bug error 124: it shall be
			//	%ERROR_INVALID_LEVEL but not %ERROR_INVALID_HANDLE. See https://connect.microsoft.com/VisualStudio/feedback/details/1641428.
			switch (err)
			{
			case ERROR_INVALID_FUNCTION:
				return EINVAL;
			case ERROR_FILE_NOT_FOUND:
			case ERROR_PATH_NOT_FOUND:
				return ENOENT;
			case ERROR_TOO_MANY_OPEN_FILES:
				return EMFILE;
			case ERROR_ACCESS_DENIED:
				return EACCES;
			case ERROR_INVALID_HANDLE:
				return EBADF;
			case ERROR_ARENA_TRASHED:
			case ERROR_NOT_ENOUGH_MEMORY:
			case ERROR_INVALID_BLOCK:
				return ENOMEM;
			case ERROR_BAD_ENVIRONMENT:
				return E2BIG;
			case ERROR_BAD_FORMAT:
				return ENOEXEC;
			case ERROR_INVALID_ACCESS:
			case ERROR_INVALID_DATA:
				return EINVAL;
			case ERROR_INVALID_DRIVE:
				return ENOENT;
			case ERROR_CURRENT_DIRECTORY:
				return EACCES;
			case ERROR_NOT_SAME_DEVICE:
				return EXDEV;
			case ERROR_NO_MORE_FILES:
				return ENOENT;
			case ERROR_LOCK_VIOLATION:
				return EACCES;
			case ERROR_BAD_NETPATH:
				return ENOENT;
			case ERROR_NETWORK_ACCESS_DENIED:
				return EACCES;
			case ERROR_BAD_NET_NAME:
				return ENOENT;
			case ERROR_FILE_EXISTS:
				return EEXIST;
			case ERROR_CANNOT_MAKE:
			case ERROR_FAIL_I24:
				return EACCES;
			case ERROR_INVALID_PARAMETER:
				return EINVAL;
			case ERROR_NO_PROC_SLOTS:
				return EAGAIN;
			case ERROR_DRIVE_LOCKED:
				return EACCES;
			case ERROR_BROKEN_PIPE:
				return EPIPE;
			case ERROR_DISK_FULL:
				return ENOSPC;
			case ERROR_INVALID_TARGET_HANDLE:
				return EBADF;
			case ERROR_INVALID_LEVEL:
				return EINVAL;
			case ERROR_WAIT_NO_CHILDREN:
			case ERROR_CHILD_NOT_COMPLETE:
				return ECHILD;
			case ERROR_DIRECT_ACCESS_HANDLE:
				return EBADF;
			case ERROR_NEGATIVE_SEEK:
				return EINVAL;
			case ERROR_SEEK_ON_DEVICE:
				return EACCES;
			case ERROR_DIR_NOT_EMPTY:
				return ENOTEMPTY;
			case ERROR_NOT_LOCKED:
				return EACCES;
			case ERROR_BAD_PATHNAME:
				return ENOENT;
			case ERROR_MAX_THRDS_REACHED:
				return EAGAIN;
			case ERROR_LOCK_FAILED:
				return EACCES;
			case ERROR_ALREADY_EXISTS:
				return EEXIST;
			case ERROR_FILENAME_EXCED_RANGE:
				return ENOENT;
			case ERROR_NESTING_NOT_ALLOWED:
				return EAGAIN;
			case ERROR_NOT_ENOUGH_QUOTA:
				return ENOMEM;
			default:
				break;
			}
			if (IsInClosedInterval<ErrorCode>(err, ERROR_WRITE_PROTECT,
				ERROR_SHARING_BUFFER_EXCEEDED))
				return EACCES;
			if (IsInClosedInterval<ErrorCode>(err, ERROR_INVALID_STARTING_CODESEG,
				ERROR_INFLOOP_IN_RELOC_CHAIN))
				return ENOEXEC;
			return EINVAL;
		}

		namespace
		{

			class Win32ErrorCategory : public std::error_category
			{
			public:
				PDefH(const char*, name, ) const wnothrow override
					ImplRet("Win32Error")
					PDefH(std::string, message, int ev) const override
					// NOTE: For Win32 a %::DWORD can be mapped one-to-one for 32-bit %int.
					ImplRet("Error " + std::to_string(ev) + ": "
						+ Win32Exception::FormatMessage(ErrorCode(ev)))
			};


			//@{
			template<typename _func>
			auto
				FetchFileInfo(_func f, UniqueHandle::pointer h)
				-> decltype(f(std::declval<::BY_HANDLE_FILE_INFORMATION&>()))
			{
				::BY_HANDLE_FILE_INFORMATION info;

				WCL_CallF_Win32(GetFileInformationByHandle, h, &info);
				return f(info);
			}

			template<typename _func, typename... _tParams>
			auto
				MakeFileToDo(_func f, _tParams&&... args)
				-> decltype(f(UniqueHandle::pointer()))
			{
				if (const auto h = MakeFile(wforward(args)...))
					return f(h.get());
				WCL_Raise_Win32E("CreateFileW", wfsig);
			}

			wconstfn
				PDefH(FileAttributesAndFlags, FollowToAttr, bool follow_reparse_point) wnothrow
				ImplRet(follow_reparse_point ? FileAttributesAndFlags::NormalWithDirectory
					: FileAttributesAndFlags::NormalAll)
				//@}


				wconstexpr const auto FSCTL_GET_REPARSE_POINT(0x000900A8UL);


			//@{
			// TODO: Extract to %YCLib.NativeAPI?
			wconstfn PDefH(unsigned long, High32, std::uint64_t val) wnothrow
				ImplRet(static_cast<unsigned long>(val >> 32UL))

				wconstfn PDefH(unsigned long, Low32, std::uint64_t val) wnothrow
				ImplRet(static_cast<unsigned long>(val))

				template<typename _func>
			auto
				DoWithDefaultOverlapped(_func f, std::uint64_t off)
				-> decltype(f(std::declval<::OVERLAPPED&>()))
			{
				::OVERLAPPED overlapped{ 0, 0,{ Low32(off), High32(off) },{} };

				return f(overlapped);
			}
			//@}


			//@{
			enum class SystemPaths
			{
				System,
				Windows
			};

			wstring
				FetchFixedSystemPath(SystemPaths e, size_t s)
			{
				// XXX: Depends right behavior on external API.
				const auto p_buf(make_unique_default_init<wchar_t[]>(s));
				const auto str(p_buf.get());

				switch (e)
				{
				case SystemPaths::System:
					WCL_CallF_Win32(GetSystemDirectoryW, str, unsigned(s));
					break;
				case SystemPaths::Windows:
					WCL_CallF_Win32(GetSystemWindowsDirectoryW, str, unsigned(s));
					break;
				}
				return white::rtrim(wstring(str), L'\\') + L'\\';
			}
			//@}

		} // unnamed namespace;


		Win32Exception::Win32Exception(ErrorCode ec, string_view msg, RecordLevel lv)
			: Exception(int(ec), GetErrorCategory(), msg, lv)
		{
			WAssert(ec != 0, "No error should be thrown.");
		}
		Win32Exception::Win32Exception(ErrorCode ec, string_view msg, const char* fn,
			RecordLevel lv)
			: Win32Exception(ec,string(msg) + " @ " + Nonnull(fn), lv)
		{}
		ImplDeDtor(Win32Exception)

			const std::error_category&
			Win32Exception::GetErrorCategory()
		{
			static const Win32ErrorCategory ecat{};

			return ecat;
		}

		std::string
			Win32Exception::FormatMessage(ErrorCode ec) wnothrow
		{
			return TryInvoke([=] {
				try
				{
					wchar_t* buf{};

					WCL_CallF_Win32(FormatMessageW, FORMAT_MESSAGE_ALLOCATE_BUFFER
						| FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
						{}, ec, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
						reinterpret_cast<wchar_t*>(&buf), 1, {});

					const auto p(unique_raw(buf, LocalDelete()));

					return WCSToMBCS(buf, unsigned(CP_UTF8));
				}
				CatchExpr(..., TraceDe(Warning, "FormatMessage failed."), throw)
			});
		}

		ModuleProc*
			LoadProc(::HMODULE h_module, const char* proc)
		{
			return reinterpret_cast<ModuleProc*>(WCL_CallF_Win32(GetProcAddress, h_module, proc));//cl bug
		}

		wstring
			FetchModuleFileName(::HMODULE h_module, RecordLevel lv)
		{
			// TODO: Avoid retry for NT 6 %::GetModuleFileNameW?
			return white::retry_for_vector<wstring>(MAX_PATH,
				[=](wstring& res, size_t s) -> bool {
				const auto r(size_t(::GetModuleFileNameW(h_module, &res[0],
					static_cast<unsigned long>(s))));
				const auto err(::GetLastError());

				if (err != ERROR_SUCCESS && err != ERROR_INSUFFICIENT_BUFFER)
					throw Win32Exception(err, "GetModuleFileNameW", lv);
				if (r < s)
				{
					res.resize(r);
					return {};
				}
				return true;
			});
		}


		void
			GlobalDelete::operator()(pointer h) const wnothrow
		{
			// NOTE: %::GlobalFree does not ignore null handle value.
			if (h && ::GlobalFree(h))
				WCL_Trace_Win32E(Warning, GlobalFree, wfsig);
		}


		GlobalLocked::GlobalLocked(::HGLOBAL h)
			: p_locked(WCL_CallF_Win32(GlobalLock, h))
		{}
		GlobalLocked::~GlobalLocked()
		{
			WFL_CallWin32F_Trace(GlobalUnlock, p_locked);
		}

		void
			LocalDelete::operator()(pointer h) const wnothrow
		{
			// FIXME: For some platforms, no %::LocalFree available. See https://msdn.microsoft.com/zh-cn/library/windows/desktop/ms679351(v=vs.85).aspx.
			// NOTE: %::LocalFree ignores null handle value.
			if WB_UNLIKELY(::LocalFree(h))
				TraceDe(Warning, "Error %lu: failed calling LocalFree @ %s.",
					::GetLastError(), wfsig);
		}



		wstring
			FetchWindowsPath(size_t s) {
			return FetchFixedSystemPath(SystemPaths::Windows, s);
		}


		NodeCategory
			TryCategorizeNodeAttributes(UniqueHandle::pointer h)
		{
			return FetchFileInfo([&](::BY_HANDLE_FILE_INFORMATION& info)
				-> NodeCategory {
				return CategorizeNode(FileAttributes(info.dwFileAttributes));
			}, h);
		}

		NodeCategory
			TryCategorizeNodeDevice(UniqueHandle::pointer h)
		{
			NodeCategory res;

			switch (::GetFileType(h))
			{
			case FILE_TYPE_CHAR:
				res = NodeCategory::Character;
				break;
			case FILE_TYPE_PIPE:
				res = NodeCategory::FIFO;
				break;
			case FILE_TYPE_UNKNOWN:
			{
				const auto err(::GetLastError());

				if (err != NO_ERROR)
					throw Win32Exception(err, "GetFileType", Err);
			}
			default:
				res = NodeCategory::Unknown;
			}
			return res;
		}

		NodeCategory
			CategorizeNode(FileAttributes attr, unsigned long reparse_tag) wnothrow
		{
			auto res(NodeCategory::Empty);

			if (IsDirectory(attr))
				res |= NodeCategory::Directory;
			if (attr & ReparsePoint)
			{
				switch (reparse_tag)
				{
				case IO_REPARSE_TAG_SYMLINK:
					res |= NodeCategory::SymbolicLink;
					break;
				case IO_REPARSE_TAG_MOUNT_POINT:
					res |= NodeCategory::MountPoint;
				default:
					;
				}
			}
			return res;
		}
		NodeCategory
			CategorizeNode(UniqueHandle::pointer h) wnothrow
		{
			TryRet(TryCategorizeNodeAttributes(h) | TryCategorizeNodeDevice(h))
				CatchIgnore(...)
				return NodeCategory::Invalid;
		}


		UniqueHandle
			MakeFile(const wchar_t* path, FileAccessRights desired_access,
				FileShareMode shared_mode, CreationDisposition creation_disposition,
				FileAttributesAndFlags attributes_and_flags) wnothrowv
		{
			using white::underlying;
			const auto h(::CreateFileW(Nonnull(path), underlying(
				desired_access), underlying(shared_mode), {}, underlying(
					creation_disposition), underlying(attributes_and_flags), {}));

			return UniqueHandle(h != INVALID_HANDLE_VALUE ? h
				: UniqueHandle::pointer());
		}

		bool
			CheckWine()
		{
			try
			{
				RegistryKey k1(HKEY_CURRENT_USER, L"Software\\Wine");
				RegistryKey k2(HKEY_LOCAL_MACHINE, L"Software\\Wine");

				wunused(k1),
					wunused(k2);
				return true;
			}
			CatchIgnore(Win32Exception&)
				return {};
		}


		void
			DirectoryFindData::Deleter::operator()(pointer p) const wnothrowv
		{
			FilterExceptions(std::bind(WCL_WrapCall_Win32(FindClose, p), wfsig),
				"directory find data deleter");
		}


		DirectoryFindData::DirectoryFindData(wstring name)
			: dir_name(std::move(name)), find_data()
		{
			if (white::rtrim(dir_name, L"/\\").empty())
				dir_name = L'.';
			WAssert(platform::EndsWithNonSeperator(dir_name),
				"Invalid argument found.");

			const auto attr(FileAttributes(::GetFileAttributesW(dir_name.c_str())));

			if (attr != Invalid)
			{
				if (attr & Directory)
					dir_name += L"\\*";
				else
					white::throw_error(ENOTDIR, wfsig);
			}
			else
				WCL_Raise_Win32E("GetFileAttributesW", wfsig);
		}

		NodeCategory
			DirectoryFindData::GetNodeCategory() const wnothrow
		{
			if (p_node && find_data.cFileName[0] != wchar_t())
			{
				auto res(CategorizeNode(find_data));
				wstring_view name(GetDirName());

				name.remove_suffix(1);
				TryInvoke([&] {
					auto gd(white::unique_guard([&]() wnothrow{
						res |= NodeCategory::Invalid;
					}));

					// NOTE: Only existed and accessible files are considered.
					// FIXME: Blocked. TOCTTOU access.
					if (const auto h = MakeFile((wstring(name) + GetEntryName()).c_str(),
						FileSpecificAccessRights::ReadAttributes,
						FileAttributesAndFlags::NormalWithDirectory))
						res |= TryCategorizeNodeAttributes(h.get())
						| TryCategorizeNodeDevice(h.get());
					white::dismiss(gd);
				});
				return res;
			}
			return NodeCategory::Empty;
		}

		bool
			DirectoryFindData::Read()
		{
			const auto chk_err([this](const char* fn, ErrorCode ec) WB_NONNULL(1) {
				const auto err(::GetLastError());

				if (err != ec)
					throw Win32Exception(err, fn, Err);
			});

			if (!p_node)
			{
				const auto h_node(::FindFirstFileW(GetDirName().c_str(), &find_data));

				if (h_node != INVALID_HANDLE_VALUE)
					p_node.reset(h_node);
				else
					chk_err("FindFirstFileW", ERROR_FILE_NOT_FOUND);
			}
			else if (!::FindNextFileW(p_node.get(), &find_data))
			{
				chk_err("FindNextFileW", ERROR_NO_MORE_FILES);
				p_node.reset();
			}
			if (p_node)
				return find_data.cFileName[0] != wchar_t();
			find_data.cFileName[0] = wchar_t();
			return {};
		}

		void
			DirectoryFindData::Rewind() wnothrow
		{
			p_node.reset();
		}


		struct ReparsePointData::Data final
		{
			struct tagSymbolicLinkReparseBuffer
			{
				unsigned short SubstituteNameOffset;
				unsigned short SubstituteNameLength;
				unsigned short PrintNameOffset;
				unsigned short PrintNameLength;
				unsigned long Flags;
				wchar_t PathBuffer[1];

				DefGetter(const wnothrow, wstring_view, PrintName,
				{ PathBuffer + size_t(PrintNameOffset) / sizeof(wchar_t),
					size_t(PrintNameLength / sizeof(wchar_t)) })
			};
			struct tagMountPointReparseBuffer
			{
				unsigned short SubstituteNameOffset;
				unsigned short SubstituteNameLength;
				unsigned short PrintNameOffset;
				unsigned short PrintNameLength;
				wchar_t PathBuffer[1];

				DefGetter(const wnothrow, wstring_view, PrintName,
				{ PathBuffer + size_t(PrintNameOffset) / sizeof(wchar_t),
					size_t(PrintNameLength / sizeof(wchar_t)) })
			};
			struct tagGenericReparseBuffer
			{
				unsigned char DataBuffer[1];
			};

			unsigned long ReparseTag;
			unsigned short ReparseDataLength;
			unsigned short Reserved;
			union
			{
				tagSymbolicLinkReparseBuffer SymbolicLinkReparseBuffer;
				tagMountPointReparseBuffer MountPointReparseBuffer;
				tagGenericReparseBuffer GenericReparseBuffer;
			};
		};


		ReparsePointData::ReparsePointData()
			: pun(white::default_init, &target_buffer)
		{
			static_assert(white::is_aligned_storable<decltype(target_buffer), Data>(),
				"Invalid buffer found.");
		}
		ImplDeDtor(ReparsePointData)


			wstring
			ResolveReparsePoint(const wchar_t* path)
		{
			return wstring(ResolveReparsePoint(path, ReparsePointData().Get()));
		}
		wstring_view
			ResolveReparsePoint(const wchar_t* path, ReparsePointData::Data& rdb)
		{
			return MakeFileToDo([=, &rdb](UniqueHandle::pointer h) {
				return FetchFileInfo([&](::BY_HANDLE_FILE_INFORMATION& info)
					-> wstring_view {
					if (info.dwFileAttributes & ReparsePoint)
					{
						WCL_CallF_Win32(DeviceIoControl, h, FSCTL_GET_REPARSE_POINT, {},
							0, &rdb, MAXIMUM_REPARSE_DATA_BUFFER_SIZE, {}, {});
						switch (rdb.ReparseTag)
						{
						case IO_REPARSE_TAG_SYMLINK:
							return rdb.SymbolicLinkReparseBuffer.GetPrintName();
						case IO_REPARSE_TAG_MOUNT_POINT:
							return rdb.MountPointReparseBuffer.GetPrintName();
						default:
							TraceDe(Warning, "Unsupported reparse tag '%lu' found",
								rdb.ReparseTag);
							white::throw_error(std::errc::not_supported, wfsig);
						}
					}
					throw std::invalid_argument(
						"Specified file is not a reparse point.");
				}, h);
			}, path, FileAttributesAndFlags::NormalAll);
		}


		wstring
			ExpandEnvironmentStrings(const wchar_t* p_src)
		{
			const auto w_len(WCL_CallF_Win32(ExpandEnvironmentStringsW, Nonnull(p_src),
			{}, 0));
			wstring wstr(w_len, wchar_t());

			WCL_CallF_Win32(ExpandEnvironmentStringsW, p_src, &wstr[0], w_len);
			return wstr;
		}


		size_t
			QueryFileLinks(UniqueHandle::pointer h)
		{
			return FetchFileInfo([](::BY_HANDLE_FILE_INFORMATION& info) {
				return size_t(info.nNumberOfLinks);
			}, h);
		}
		size_t
			QueryFileLinks(const wchar_t* path, bool follow_reparse_point)
		{
			return MakeFileToDo<size_t(UniqueHandle::pointer)>(QueryFileLinks, path,
				FollowToAttr(follow_reparse_point));
		}

		pair<VolumeID, FileID>
			QueryFileNodeID(UniqueHandle::pointer h)
		{
			return FetchFileInfo([](::BY_HANDLE_FILE_INFORMATION& info) wnothrow
				->pair<VolumeID, FileID>{
				return { VolumeID(info.dwVolumeSerialNumber),
				FileID(info.nFileIndexHigh) << 32 | info.nFileIndexLow };
			}, h);
		}
		pair<VolumeID, FileID>
			QueryFileNodeID(const wchar_t* path, bool follow_reparse_point)
		{
			return MakeFileToDo<pair<VolumeID, FileID>(UniqueHandle::pointer)>(
				QueryFileNodeID, path, FollowToAttr(follow_reparse_point));
		}

		std::uint64_t
			QueryFileSize(UniqueHandle::pointer h)
		{
			::LARGE_INTEGER sz;

			WCL_CallF_Win32(GetFileSizeEx, h, &sz);

			if (sz.QuadPart >= 0)
				return std::uint64_t(sz.QuadPart);
			throw std::invalid_argument("Negative file size found.");
		}
		std::uint64_t
			QueryFileSize(const wchar_t* path)
		{
			return MakeFileToDo<std::uint64_t(UniqueHandle::pointer)>(
				QueryFileSize, path, FileAttributesAndFlags::NormalWithDirectory);
		}

		void
			QueryFileTime(UniqueHandle::pointer h, ::FILETIME* p_ctime, ::FILETIME* p_atime,
				::FILETIME* p_mtime)
		{
			WCL_CallF_Win32(GetFileTime, h, p_ctime, p_atime, p_mtime);
		}
		void
			QueryFileTime(const wchar_t* path, ::FILETIME* p_ctime, ::FILETIME* p_atime,
				::FILETIME* p_mtime, bool follow_reparse_point)
		{
			MakeFileToDo(std::bind<void(UniqueHandle::pointer, ::FILETIME*, ::FILETIME*,
				::FILETIME*)>(QueryFileTime, std::placeholders::_1, p_ctime, p_atime,
					p_mtime), path, AccessRights::GenericRead,
				FollowToAttr(follow_reparse_point));
		}

		void
			SetFileTime(UniqueHandle::pointer h, ::FILETIME* p_ctime, ::FILETIME* p_atime,
				::FILETIME* p_mtime)
		{
			using ::SetFileTime;

			WCL_CallF_Win32(SetFileTime, h, p_ctime, p_atime, p_mtime);
		}
		void
			SetFileTime(const wchar_t* path, ::FILETIME* p_ctime, ::FILETIME* p_atime,
				::FILETIME* p_mtime, bool follow_reparse_point)
		{
			MakeFileToDo(std::bind<void(UniqueHandle::pointer, ::FILETIME*, ::FILETIME*,
				::FILETIME*)>(SetFileTime, std::placeholders::_1, p_ctime, p_atime,
					p_mtime), path, AccessRights::GenericWrite,
				FollowToAttr(follow_reparse_point));
		}

		std::chrono::nanoseconds
			ConvertTime(const ::FILETIME& file_time)
		{
			if (file_time.dwLowDateTime != 0 || file_time.dwHighDateTime != 0)
			{
				// FIXME: Local time conversion for FAT volumes.
				// NOTE: See $2014-10 @ %Documentation::Workflow::Annual2014.
				::LARGE_INTEGER date;

				// NOTE: The epoch is Jan. 1, 1601: 134774 days to Jan. 1, 1970, i.e.
				//	11644473600 seconds, or 116444736000000000 * 100 nanoseconds.
				// TODO: Strip away the magic number;
				wunseq(date.HighPart = long(file_time.dwHighDateTime),
					date.LowPart = file_time.dwLowDateTime);
				return std::chrono::nanoseconds((date.QuadPart - 116444736000000000LL)
					* 100U);
			}
			else
				white::throw_error(std::errc::not_supported, wfsig);
		}
		::FILETIME
			ConvertTime(std::chrono::nanoseconds file_time)
		{
			::FILETIME res;
			::LARGE_INTEGER date;

			date.QuadPart = file_time.count() / 100LL + 116444736000000000LL;
			wunseq(res.dwHighDateTime = static_cast<unsigned long>(date.HighPart),
				res.dwLowDateTime = date.LowPart);
			if (res.dwLowDateTime != 0 || res.dwHighDateTime != 0)
				return res;
			white::throw_error(std::errc::not_supported, wfsig);
		}


		void
			LockFile(UniqueHandle::pointer h, std::uint64_t off, std::uint64_t len,
				bool exclusive, bool immediate)
		{
			if WB_UNLIKELY(!TryLockFile(h, off, len, exclusive, immediate))
				WCL_Raise_Win32E("LockFileEx", wfsig);
		}

		bool
			TryLockFile(UniqueHandle::pointer h, std::uint64_t off, std::uint64_t len,
				bool exclusive, bool immediate) wnothrow
		{
			return DoWithDefaultOverlapped([=](::OVERLAPPED& overlapped) wnothrow{
				// NOTE: Since it can always be checked as result and this is called by
				//	%LockFile, no logging with %YCL_TraceCallF_Win32 is here.
				return ::LockFileEx(h, (exclusive ? LOCKFILE_EXCLUSIVE_LOCK : 0UL)
				| (immediate ? LOCKFILE_FAIL_IMMEDIATELY : 0UL), wimpl(0),
					Low32(len), High32(len), &overlapped);
			}, off);
		}

		bool
			UnlockFile(UniqueHandle::pointer h, std::uint64_t off, std::uint64_t len)
			wnothrow
		{
			return DoWithDefaultOverlapped([=](::OVERLAPPED& overlapped) wnothrow{
				return
				::UnlockFileEx(h, wimpl(0), Low32(len), High32(len), &overlapped);
			}, off);
		}
	}
#endif
}