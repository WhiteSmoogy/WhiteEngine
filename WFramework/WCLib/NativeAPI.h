/*!	\file NativeAPI.h
\ingroup Framework
\brief ͨ��ƽ̨Ӧ�ó���ӿ�������
*/

#ifndef FrameWork_NativeAPI_h
#define FrameWork_NativeAPI_h 1

#include <WFramework/WCLib/Platform.h>
#include <WBase/wmacro.h>

#ifndef  WF_Platform
#error "Unknown platform found."
#endif // ! WF_Platform

//@{
/*!
\def WCL_ReservedGlobal
\brief ��ʵ������ȫ�ֱ������ơ�
\see ISO C11 7.1.3 �� WG21 N4594 17.6.4.3 ��
\see https://msdn.microsoft.com/en-us/library/ttcz0bys.aspx ��
\see https://msdn.microsoft.com/en-us/library/ms235384(v=vs.100).aspx#Anchor_0 ��
*/
#if WFL_Win32
#	define WCL_ReservedGlobal(_n) _##_n
#else
#	define WCL_ReservedGlobal(_n) _n
#endif
//! \brief ���ð�ʵ�����εľ���ȫ�ֱ������Ƶĺ�����
#define WCL_CallGlobal(_n, ...) ::WCL_ReservedGlobal(_n)(__VA_ARGS__)
//@}

#include <stdio.h>
#include <fcntl.h>

namespace platform
{
#	if WFL_Win32
	using ssize_t = int;
#	else
	using ssize_t = white::make_signed_t<std::size_t>;
#	endif
} // namespace platform;

static_assert(std::is_signed<platform::ssize_t>(),
	"Invalid signed size type found.");


/*!
\ingroup name_collision_workarounds
\see http://pubs.opengroup.org/onlinepubs/9699919799/functions/V2_chap02.html#tag_15_01 ��
*/
//@{
//! \note ������ <stdio.h> ��
//@{
#undef ctermid
#undef dprintf
#undef fdopen
//! \note ��֪ DS ƽ̨�� newlib ����ʹ�úꡣ
#undef fileno
#undef flockfile
#undef fmemopen
#undef fseeko
#undef ftello
#undef ftrylockfile
#undef funlockfile
#undef getc_unlocked
#undef getchar_unlocked
#undef getdelim
#undef getline
#undef open_memstream
#undef pclose
#undef popen
#undef putc_unlocked
#undef putchar_unlocked
#undef renameat
#undef tempnam
#undef vdprintf
//@}
//! \note ������ <dirent.h> ��
//@{
#undef alphasort
#undef closedir
#undef dirfd
#undef fdopendir
#undef opendir
#undef readdir
#undef readdir_r
#undef rewinddir
#undef scandir
#undef seekdir
#undef telldir
//@}
//! \note ������ <fcntl.h> ��
//@{
#undef creat
#undef fcntl
#undef open
#undef openat
#undef posix_fadvise
#undef posix_fallocate
//@}
//! \note ������ <semaphore.h> ��
#undef sem_close
#undef sem_destroy
#undef sem_getvalue
#undef sem_init
#undef sem_open
#undef sem_post
#undef sem_timedwait
#undef sem_trywait
#undef sem_unlink
#undef sem_wait
//! \note ������ <unistd.h> ��
//@{
#undef _exit
#undef access
#undef alarm
#undef chdir
#undef chown
#undef close
#undef confstr
#undef crypt
#undef dup
#undef dup2
#undef encrypt
#undef execl
#undef execle
#undef execlp
#undef execv
#undef execve
#undef execvp
#undef faccessat
#undef fchdir
#undef fchown
#undef fchownat
#undef fdatasync
#undef fexecve
#undef fork
#undef fpathconf
#undef fsync
#undef ftruncate
#undef getcwd
#undef getegid
#undef geteuid
#undef getgid
#undef getgroups
#undef gethostid
#undef gethostname
#undef getlogin
#undef getlogin_r
#undef getopt
#undef getpgid
#undef getpgrp
#undef getpid
#undef getppid
#undef getsid
#undef getuid
#undef isatty
#undef lchown
#undef link
#undef linkat
#undef lockf
#undef lseek
#undef nice
#undef pathconf
#undef pause
#undef pipe
#undef pread
#undef pwrite
#undef read
#undef readlink
#undef readlinkat
#undef rmdir
#undef setegid
#undef seteuid
#undef setgid
#undef setpgid
#undef setpgrp
#undef setregid
#undef setreuid
#undef setsid
#undef setuid
#undef sleep
#undef swab
#undef symlink
#undef symlinkat
#undef sync
#undef sysconf
#undef tcgetpgrp
#undef tcsetpgrp
#undef truncate
#undef ttyname
#undef ttyname_r
#undef unlink
#undef unlinkat
#undef write
//@}
//! \note ������ <sys/mman.h> ��
//@{
#undef mlock
#undef mlockall
#undef mmap
#undef mprotect
#undef msync
#undef munlock
#undef munlockall
#undef munmap
#undef posix_madvise
#undef posix_mem_offset
#undef posix_typed_mem_get_info
#undef posix_typed_mem_open
#undef shm_open
#undef shm_unlink
//@}
//! \note ������ <sys/stat.h> ��
//@{
#undef chmod
#undef fchmod
#undef fchmodat
#undef fstat
#undef fstatat
#undef futimens
#undef lstat
#undef mkdir
#undef mkdirat
#undef mkfifo
#undef mkfifoat
#undef mknod
#undef mknodat
#undef stat
#undef umask
#undef utimensat
//@}
//@}


#if WFL_Win32

#	ifndef UNICODE
#		define UNICODE 1
#	endif

#	ifndef WINVER
#		define WINVER _WIN32_WINNT_WIN7
#	endif

#	ifndef WIN32_LEAN_AND_MEAN
#		define WIN32_LEAN_AND_MEAN 1
#	endif

#	ifndef NOMINMAX
#		define NOMINMAX 1
#	endif

#	include <Windows.h>
#	include <direct.h>
#   include <io.h>

//! \ingroup name_collision_workarounds
//@{
#	undef CopyFile
#	undef CreateHardLink
#	undef CreateSymbolicLink
#	undef DialogBox
#	undef DrawText
#	undef ExpandEnvironmentStrings
#	undef FindWindow
#	undef FormatMessage
#	undef PeekMessage
#	undef GetMessage
#	undef GetObject
#	undef PostMessage
#	undef SetEnvironmentVariable
//@}

extern "C"
{
#	if defined(__MINGW32__) && !defined(__MINGW64_VERSION_MAJOR)
	WB_API struct ::tm* __cdecl __MINGW_NOTHROW
		_gmtime32(const ::__time32_t*);
#	endif

} // extern "C";


namespace platform_ex
{

	/*!
	\brief ȡ�ļ���������Ӧ�ľ����
	*/
	inline PDefH(::HANDLE, ToHandle, int fd) wnothrow
		ImplRet(::HANDLE(::_get_osfhandle(fd)))

} // namespace platform_ex;
#endif

#if WFL_Win32 || WFL_API_POSIXFileSystem
#	include <sys/stat.h>

namespace platform
{

	enum class Mode
#	if WFL_Win32
		: unsigned short
#	else
		: ::mode_t
#	endif
	{
#	if WFL_Win32
		FileType = _S_IFMT,
		Directory = _S_IFDIR,
		Character = _S_IFCHR,
		FIFO = _S_IFIFO,
		Regular = _S_IFREG,
		UserRead = _S_IREAD,
		UserWrite = _S_IWRITE,
		UserExecute = _S_IEXEC,
		GroupRead = _S_IREAD >> 3,
		GroupWrite = _S_IWRITE >> 3,
		GroupExecute = _S_IEXEC >> 3,
		OtherRead = _S_IREAD >> 6,
		OtherWrite = _S_IWRITE >> 6,
		OtherExecute = _S_IEXEC >> 6,
#	else
		FileType = S_IFMT,
		Directory = S_IFDIR,
		Character = S_IFCHR,
		Block = S_IFBLK,
		Regular = S_IFREG,
		Link = S_IFLNK,
		Socket = S_IFSOCK,
		FIFO = S_IFIFO,
		UserRead = S_IRUSR,
		UserWrite = S_IWUSR,
		UserExecute = S_IXUSR,
		GroupRead = S_IRGRP,
		GroupWrite = S_IWGRP,
		GroupExecute = S_IXGRP,
		OtherRead = S_IROTH,
		OtherWrite = S_IWOTH,
		OtherExecute = S_IXOTH,
#	endif
		UserReadWrite = UserRead | UserWrite,
		User = UserReadWrite | UserExecute,
		GroupReadWrite = GroupRead | GroupWrite,
		Group = GroupReadWrite | GroupExecute,
		OtherReadWrite = OtherRead | OtherWrite,
		Other = OtherReadWrite | OtherExecute,
		Read = UserRead | GroupRead | OtherRead,
		Write = UserWrite | GroupWrite | OtherWrite,
		Execute = UserExecute | GroupExecute | OtherExecute,
		ReadWrite = Read | Write,
		Access = ReadWrite | Execute,
		//@{
#	if !WFL_Win32
		SetUserID = S_ISUID,
		SetGroupID = S_ISGID,
#	else
		SetUserID = 0,
		SetGroupID = 0,
#	endif
#	if WFL_Linux || _XOPEN_SOURCE
		VTX = S_ISVTX,
#	else
		VTX = 0,
#	endif
		PMode = SetUserID | SetGroupID | VTX | Access,
		All = PMode | FileType
		//@}
	};

	//! \relates Mode
	//@{
	DefBitmaskEnum(Mode)

#ifdef WB_IMPL_MSCPP
#pragma warning(push)
#pragma warning(disable:4800)
#endif
		inline PDefH(bool, HasExtraMode, Mode m)
		ImplRet(bool(m & ~(Mode::Access | Mode::FileType)))


		//@{
		//! \brief ��ģʽ��
		enum class OpenMode : int
	{
#define YCL_Impl_OMode(_n, _nm) _n = WCL_ReservedGlobal(O_##_nm)
#if WFL_Win32
#	define YCL_Impl_OMode_POSIX(_n, _nm) _n = 0
#	define YCL_Impl_OMode_Win32(_n, _nm) YCL_Impl_OMode(_n, _nm)
#else
#	define YCL_Impl_OMode_POSIX(_n, _nm) YCL_Impl_OMode(_n, _nm)
#	define YCL_Impl_OMode_Win32(_n, _nm) _n = 0
#endif
#if O_CLOEXEC
		YCL_Impl_OMode_POSIX(CloseOnExec, CLOEXEC),
#endif
		YCL_Impl_OMode(Create, CREAT),
#if O_DIRECTORY
		YCL_Impl_OMode_POSIX(Directory, DIRECTORY),
#else
		// FIXME: Platform %DS does not support this.
		Directory = 0,
#endif
		YCL_Impl_OMode(Exclusive, EXCL),
		CreateExclusive = Create | Exclusive,
		YCL_Impl_OMode_POSIX(NoControllingTerminal, NOCTTY),
#if O_NOFOLLOW
		YCL_Impl_OMode_POSIX(NoFollow, NOFOLLOW),
#else
		// NOTE: Platform %DS does not support links.
		// NOTE: Platform %Win32 does not support links for these APIs.
		NoFollow = 0,
#endif
		YCL_Impl_OMode(Truncate, TRUNC),
#if O_TTY_INIT
		YCL_Impl_OMode_POSIX(TerminalInitialize, TTY_INIT),
#endif
		YCL_Impl_OMode(Append, APPEND),
#if O_DSYNC
		YCL_Impl_OMode_POSIX(DataSynchronized, DSYNC),
#endif
		YCL_Impl_OMode_POSIX(Nonblocking, NONBLOCK),
#if O_RSYNC
		YCL_Impl_OMode_POSIX(ReadSynchronized, RSYNC),
#endif
		YCL_Impl_OMode_POSIX(Synchronized, SYNC),
#if O_EXEC
		YCL_Impl_OMode_POSIX(Execute, EXEC),
#endif
		YCL_Impl_OMode(Read, RDONLY),
		YCL_Impl_OMode(ReadWrite, RDWR),
#if O_SEARCH
		YCL_Impl_OMode_POSIX(Search, SEARCH),
#endif
		YCL_Impl_OMode(Write, WRONLY),
		ReadWriteAppend = ReadWrite | Append,
		ReadWriteTruncate = ReadWrite | Truncate,
		WriteAppend = Write | Append,
		WriteTruncate = Write | Truncate,
		YCL_Impl_OMode_Win32(Text, TEXT),
		YCL_Impl_OMode_Win32(Binary, BINARY),
		Raw = Binary,
		ReadRaw = Read | Raw,
		ReadWriteRaw = ReadWrite | Raw,
		// NOTE: On GNU/Hurd %O_ACCMODE can be zero.
#if O_ACCMODE
		YCL_Impl_OMode_POSIX(AccessMode, ACCMODE),
#else
		AccessMode = Read | Write | ReadWrite,
#endif
		YCL_Impl_OMode_Win32(WText, WTEXT),
		YCL_Impl_OMode_Win32(U16Text, U16TEXT),
		YCL_Impl_OMode_Win32(U8Text, U8TEXT),
		YCL_Impl_OMode_Win32(NoInherit, NOINHERIT),
		YCL_Impl_OMode_Win32(Temporary, TEMPORARY),
		YCL_Impl_OMode_Win32(ShortLived, SHORT_LIVED),
		CreateTemporary = Create | Temporary,
		CreateShortLived = Create | ShortLived,
		YCL_Impl_OMode_Win32(Sequential, SEQUENTIAL),
		YCL_Impl_OMode_Win32(Random, RANDOM),
#if O_NDELAY
		YCL_Impl_OMode(NoDelay, NDELAY),
#endif
		//! \warning ��ʵ���ڲ�ʹ�ã���Ҫ�ض��Ķ�����֧�֡�
		//@{
#if O_LARGEFILE
		//! \note ָ�� 64 λ�ļ���С��
		YCL_Impl_OMode(LargeFile, LARGEFILE),
#else
		LargeFile = 0,
#endif
#if _O_OBTAIN_DIR
		YCL_Impl_OMode(ObtainDirectory, OBTAIN_DIR),
#else
		//! \note ���� FILE_FLAG_BACKUP_SEMANTICS ��
		ObtainDirectory = Directory,
#endif
		//@}
		None = 0
#undef YCL_Impl_OMode_POSIX
#undef YCL_Impl_OMode_Win32
#undef YCL_Impl_OMode
	};

	//! \relates OpenMode
	DefBitmaskEnum(OpenMode)
		//@}
		//@}
#ifdef WB_IMPL_MSCPP
#pragma warning(pop)
#endif
} // namespace platform;

namespace platform_ex
{

	/*!
	\note ����������ʾ�Ƿ�������ӡ�
	\pre ��Ӷ��ԣ�ָ������ǿա�
	\note DS �� Win32 ƽ̨�����Ե���������ʼ�ղ��������ӡ�
	*/
	//@{
	/*!
	\brief �����Ŀɸ������ӵ� \c stat ���á�
	\throw std::system_error ���ʧ�ܡ�
	\note ���һ��������ʾ����ǩ����
	*/
	//@{
	WF_API WB_NONNULL(2, 4) void
		cstat(struct ::stat&, const char*, bool, const char*);
	WF_API WB_NONNULL(3) void
		cstat(struct ::stat&, int, const char*);
	//@}

	//! \brief �ɸ������ӵ� \c stat ���á�
	WF_API WB_NONNULL(2) int
		estat(struct ::stat&, const char*, bool) wnothrowv;
	inline PDefH(int, estat, struct ::stat& st, int fd) wnothrow
		ImplRet(::fstat(fd, &st))
		//@}

} // namespace platform_ex;
#endif


#endif
