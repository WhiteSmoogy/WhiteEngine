/*!	\file FFileIO.h
\ingroup Framework
\brief 容器、拟容器和适配器。
*/

#ifndef FrameWork_FFileIO_h
#define FrameWork_FFileIO_h 1

#include <WFramework/WCLib/Platform.h>
#include <WBase/wmacro.h>
#include <WBase/string.hpp>
#include <WFramework/WCLib/Debug.h>
#include <WFramework/WCLib/FReference.h>
#include <WFramework/WCLib/FContainer.h>
#include <WBase/container.hpp>

#include <chrono>
#include <ios>
#include <fstream>
#include <system_error>

#if __GLIBCXX__
#include <ext/stdio_filebuf.h>
#include <WBase/utility.hpp>
#endif

namespace platform
{
	/*!
	\ingroup diagnostic
	\brief 组合消息和函数签名字符串。
	\pre 间接断言：指针参数非空。
	\note 使用 ADL to_string 。
	*/
	//@{
	inline WB_NONNULL(2) PDefH(string, ComposeMessageWithSignature,
		const string& msg, const char* sig)
		ImplRet(msg + " @ " + Nonnull(sig))
		inline WB_NONNULL(1, 2) PDefH(string, ComposeMessageWithSignature,
			const char* msg, const char* sig)
		ImplRet(string(Nonnull(msg)) + " @ " + Nonnull(sig))
		template<class _type>
	inline WB_NONNULL(2) string
		ComposeMessageWithSignature(const _type& msg, const char* sig)
	{
		using white::to_string;

		return to_string(msg) + " @ " + Nonnull(sig);
	}
	//@}

	/*!
	\brief 构造适合表示路径的 \c char 字符串。
	\note 字符类型非 \c char 时转换。
	*/
	//@{
	//! \pre 间接断言：参数非空。
	inline WB_NONNULL(1) PDefH(string, MakePathString, const char* s)
		ImplRet(Nonnull(s))
		inline PDefH(const string&, MakePathString, const string& s)
		ImplRet(s)
		//! \pre Win32 平台：因实现不直接访问左值，字符的动态类型可为布局兼容的整数类型。
		//@{
		//! \pre 间接断言：参数非空。
		WF_API WB_NONNULL(1) string
		MakePathString(const char16_t*);
	inline PDefH(string, MakePathString, u16string_view sv)
		ImplRet(MakePathString(sv.data()))
		//@}
		//@}


		//@{
		//! \brief 文件节点标识类型。
		//@{
#if WFL_Win32
		using FileNodeID = pair<std::uint32_t, std::uint64_t>;
#else
		using FileNodeID = pair<std::uint64_t, std::uint64_t>;
#endif
	//@}

	/*!
	\bug 结构化类型污染。
	\relates FileNodeID
	*/
	//@{
	wconstfn PDefHOp(bool, == , const FileNodeID& x, const FileNodeID& y)
		ImplRet(x.first == y.first && x.second == y.second)
		wconstfn PDefHOp(bool, != , const FileNodeID& x, const FileNodeID& y)
		ImplRet(!(x == y))
		//@}
		//@}


		using FileTime = std::chrono::nanoseconds;


	/*!
	\brief 文件模式类型。
	*/
	//@{
#if WFL_Win32
	using mode_t = unsigned short;
#else
	using mode_t = ::mode_t;
#endif
	//@}


	/*!
	\brief 文件节点类别。
	*/
	enum class NodeCategory : std::uint_least32_t;

	/*
	\brief 取指定模式对应的文件节点类型。
	\relates NodeCategory
	\since build 658
	*/
	WF_API NodeCategory
		CategorizeNode(mode_t) wnothrow;


	/*!
	\brief 文件描述符包装类。
	\note 除非另行约定，具有无异常抛出保证的操作失败时可能设置 errno 。
	\note 不支持的平台操作失败设置 errno 为 ENOSYS 。
	\note 除非另行约定，无异常抛出的操作使用值初始化的返回类型表示失败结果。
	\note 以 \c int 为返回值的操作返回 \c -1 表示失败。
	\note 满足 NullablePointer 要求。
	\note 满足共享锁要求。
	\see WG21 N4606 17.6.3.3[nullablepointer.requirements] 。
	\see WG21 N4606 30.4.1.4[thread.sharedmutex.requirements] 。
	*/
	class WF_API FileDescriptor : private white::totally_ordered<FileDescriptor>
	{
	public:
		/*!
		\brief 删除器。
		*/
		struct WF_API Deleter
		{
			using pointer = FileDescriptor;

			void
				operator()(pointer) const wnothrow;
		};

	private:
		int desc = -1;

	public:
		wconstfn DefDeCtor(FileDescriptor)
			wconstfn
			FileDescriptor(int fd) wnothrow
			: desc(fd)
		{}
		wconstfn
			FileDescriptor(nullptr_t) wnothrow
		{}
		/*!
		\brief 构造：使用标准流。
		\note 对空参数不设置 errno 。

		当参数为空时得到表示无效文件的空描述符，否则调用 POSIX \c fileno 函数。
		*/
		FileDescriptor(std::FILE*) wnothrow;

		/*!
		\note 和 operator-> 不一致，不返回引用，以避免生存期问题。
		*/
		PDefHOp(int, *, ) const wnothrow
			ImplRet(desc)

			PDefHOp(FileDescriptor*, ->, ) wnothrow
			ImplRet(this)
			PDefHOp(const FileDescriptor*, ->, ) const wnothrow
			ImplRet(this)

			explicit DefCvt(const wnothrow, bool, desc != -1)

			friend wconstfn WB_PURE PDefHOp(bool,
				== , const FileDescriptor& x, const FileDescriptor& y) wnothrow
			ImplRet(x.desc == y.desc)

			//! \since build 639
			friend wconstfn WB_PURE PDefHOp(bool,
				<, const FileDescriptor& x, const FileDescriptor& y) wnothrow
			ImplRet(x.desc < y.desc)

			//! \exception std::system_error 参数无效或调用失败。
			//@{
			/*!
			\brief 取访问时间。
			\return 以 POSIX 时间相同历元的时间间隔。
			\note 当前 Windows 使用 \c ::GetFileTime 实现，其它只保证最高精确到秒。
			*/
			FileTime
			GetAccessTime() const;
		/*!
		\brief 取节点类别。
		\return 失败时为 NodeCategory::Invalid ，否则为对应类别。
		*/
		NodeCategory
			GetCategory() const wnothrow;
		//@{
		/*!
		\brief 取旗标。
		\note 非 POSIX 平台：不支持操作。
		*/
		int
			GetFlags() const;
		//! \brief 取模式。
		mode_t
			GetMode() const;
		//@}
		/*!
		\return 以 POSIX 时间相同历元的时间间隔。
		\note 当前 Windows 使用 \c ::GetFileTime 实现，其它只保证最高精确到秒。
		*/
		//@{
		//! \brief 取修改时间。
		FileTime
			GetModificationTime() const;
		//! \brief 取修改和访问时间。
		array<FileTime, 2>
			GetModificationAndAccessTime() const;
		//@}
		//@}
		/*!
		\note 若存储分配失败，设置 errno 为 \c ENOMEM 。
		*/
		//@{
		//! \brief 取文件系统节点标识。
		FileNodeID
			GetNodeID() const wnothrow;
		//! \brief 取链接数。
		size_t
			GetNumberOfLinks() const wnothrow;
		//@}
		/*!
		\brief 取大小。
		\return 以字节计算的文件大小。
		\throw std::system_error 描述符无效或文件大小查询失败。
		\throw std::invalid_argument 文件大小查询结果小于 0 。
		\note 非常规文件或文件系统实现可能出错。
		*/
		std::uint64_t
			GetSize() const;

		/*!
		\brief 设置访问时间。
		\throw std::system_error 设置失败。
		\note DS 平台：不支持操作。
		*/
		void
			SetAccessTime(FileTime) const;
		/*!
		\throw std::system_error 调用失败或不支持操作。
		\note 非 POSIX 平台：不支持操作。
		*/
		//@{
		/*!
		\brief 设置阻塞模式。
		\return 是否需要并改变设置。
		\see http://pubs.opengroup.org/onlinepubs/9699919799/functions/fcntl.html 。
		*/
		bool
			SetBlocking() const;
		//! \brief 设置旗标。
		void
			SetFlags(int) const;
		//! \brief 设置访问模式。
		void
			SetMode(mode_t) const;
		//@}
		/*!
		\throw std::system_error 设置失败。
		\note DS 平台：不支持操作。
		\note Win32 平台：要求打开的文件具有写权限。
		*/
		//@{
		//! \brief 设置修改时间。
		void
			SetModificationTime(FileTime) const;
		//! \brief 设置修改和访问时间。
		void
			SetModificationAndAccessTime(array<FileTime, 2>) const;
		//@}
		/*!
		\brief 设置非阻塞模式。
		\return 是否需要并改变设置。
		\throw std::system_error 调用失败或不支持操作。
		\note 非 POSIX 平台：不支持操作。
		\note 对不支持非阻塞的文件描述符， POSIX 未指定是否忽略 \c O_NONBLOCK 。
		\see http://pubs.opengroup.org/onlinepubs/9699919799/functions/fcntl.html 。
		*/
		bool
			SetNonblocking() const;
		//@{
		/*!
		\brief 调整文件至指定长度。
		\pre 指定文件需已经打开并可写。
		\note 不改变文件读写位置。

		若文件不足指定长度，扩展并使用空字节填充；否则保留起始指定长度的字节。
		*/
		bool
			SetSize(size_t) wnothrow;
		/*!
		\brief 设置翻译模式。
		\note 兼容 Win32 文本模式行为的平台：参数和返回值意义和语义同 \c setmode 函数。
		\note 其它平台：无作用。
		*/
		int
			SetTranslationMode(int) const wnothrow;

		/*!
		\brief 刷新。
		\throw std::system_error 调用失败。
		*/
		void
			Flush();

		/*!
		\pre 间接断言：文件有效。
		\pre 间接断言：指针参数非空。
		\note 每次读写首先清除 errno ；读写时遇 EINTR 时继续。
		*/
		//@{
		/*!
		\brief 循环读写文件。
		*/
		//@{
		/*!
		\note 每次读 0 字节时设置 errno 为 0 。
		\sa Read
		*/
		WB_NONNULL(2) size_t
			FullRead(void*, size_t) wnothrowv;

		/*!
		\note 每次写 0 字节时设置 errno 为 ENOSPC 。
		\sa Write
		*/
		WB_NONNULL(2) size_t
			FullWrite(const void*, size_t) wnothrowv;
		//@}

		/*!
		\brief 读写文件。
		\note 首先清除 errno 。
		\note Win64 平台：大小截断为 32 位。
		\return 若发生错误为 size_t(-1) ，否则为读取的字节数。
		*/
		//@{
		WB_NONNULL(2) size_t
			Read(void*, size_t) wnothrowv;

		WB_NONNULL(2) size_t
			Write(const void*, size_t) wnothrowv;
		//@}
		//@}
		//@}

		/*!
		\brief 第二参数内容写入第一参数指定的文件。
		\pre 断言：文件有效。
		\pre 最后参数指定的缓冲区大小不等于 0 。
		\throw std::system_error 文件读写失败。
		*/
		//@{
		//! \pre 参数指定的缓冲区非空且大小不等于 0 。
		static WB_NONNULL(3) void
			WriteContent(FileDescriptor, FileDescriptor, byte*, size_t);
		/*!
		\note 最后一个参数指定缓冲区大小的上限，若分配失败自动重新分配。
		\throw std::bad_alloc 缓冲区分配失败。
		*/
		static void
			WriteContent(FileDescriptor, FileDescriptor,
				size_t = wimpl(size_t(BUFSIZ << 4)));
		//@}

		/*!
		\pre 间接断言：文件有效。
		\note DS 平台：无作用。
		\note POSIX 平台：不使用 POSIX 文件锁而使用 BSD 代替，
		以避免无法控制的释放导致安全漏洞。
		*/
		//@{
		/*!
		\note Win32 平台：对内存映射文件为协同锁，其它文件为强制锁。
		\note 其它平台：协同锁。
		\warning 网络或分布式文件系统可能不能正确支持独占锁（如 Andrew File System ）。
		*/
		//@{
		//! \throw std::system_error 文件锁定失败。
		//@{
		void
			lock();

		void
			lock_shared();
		//@}

		//! \return 是否锁定成功。
		//@{
		bool
			try_lock() wnothrowv;

		bool
			try_lock_shared() wnothrowv;
		//@}
		//@}

		//! \pre 进程对文件访问具有所有权。
		//@{
		void
			unlock() wnothrowv;

		void
			unlock_shared() wnothrowv;
		//@}
		//@}
	};


	using UniqueFile = unique_ptr_from<FileDescriptor::Deleter>;


	/*!
	\brief 取默认权限。
	*/
	WF_API WB_STATELESS mode_t
		DefaultPMode() wnothrow;

	/*!
	\brief 设置标准库流二进制输入/输出模式。
	\pre 间接断言：参数非空。
	*/
	//@{
	WF_API void
		SetBinaryIO(std::FILE*) wnothrowv;

	/*!
	\warning 改变默认日志默认发送器前，不应使用 std::cerr 和 std::clog
	等依赖 \c stderr 的流，以避免导致同步问题。
	\sa FetchCommonLogger
	\sa Logger::DefaultSendLog
	*/
	inline PDefH(void, SetupBinaryStdIO, std::FILE* in = stdin,
		std::FILE* out = stdout, bool sync = {}) wnothrowv
		ImplExpr(SetBinaryIO(in), SetBinaryIO(out),
			std::ios_base::sync_with_stdio(sync))
		//@}


		/*!
		\brief ISO C++ 标准输入输出接口打开模式转换为 POSIX 文件打开模式。
		\return 若失败为 0 ，否则为对应的值。
		*/
		//@{
		//! \note 忽略二进制模式。
		WF_API WB_STATELESS int
		omode_conv(std::ios_base::openmode) wnothrow;

	/*!
	\note 扩展：不忽略二进制模式。
	\note POSIX 平台：同 omode_conv 。
	*/
	WF_API WB_STATELESS int
		omode_convb(std::ios_base::openmode) wnothrow;
	//@}

	/*!
	\pre 断言：第一参数非空。
	\note 若存储分配失败，设置 errno 为 \c ENOMEM 。
	\since build 669
	*/
	//@{
	/*!
	\brief 测试路径可访问性。
	\param path 路径，意义同 POSIX <tt>::open</tt> 。
	\param amode 模式，基本语义同 POSIX.1 2004 ，具体行为取决于实现。 。
	\note errno 在出错时会被设置，具体值由实现定义。
	*/
	//@{
	WF_API WB_NONNULL(1) int
		uaccess(const char* path, int amode) wnothrowv;
	WF_API WB_NONNULL(1) int
		uaccess(const char16_t* path, int amode) wnothrowv;
	//@}

	/*!
	\param filename 文件名，意义同 POSIX \c ::open 。
	\param oflag 打开旗标，基本语义同 POSIX.1 2004 ，具体行为取决于实现。
	\param pmode 打开模式，基本语义同 POSIX.1 2004 ，具体行为取决于实现。
	*/
	//@{
	//! \brief 以 UTF-8 文件名无缓冲打开文件。
	WF_API WB_NONNULL(1) int
		uopen(const char* filename, int oflag, mode_t pmode = DefaultPMode())
		wnothrowv;
	//! \brief 以 UCS-2 文件名无缓冲打开文件。
	WF_API WB_NONNULL(1) int
		uopen(const char16_t* filename, int oflag, mode_t pmode = DefaultPMode())
		wnothrowv;
	//@}

	//! \param filename 文件名，意义同 std::fopen 。
	//@{
	/*!
	\param mode 打开模式，基本语义同 ISO C11 ，具体行为取决于实现。
	\pre 断言：<tt>mode && *mode != 0</tt> 。
	*/
	//@{
	//! \brief 以 UTF-8 文件名打开文件。
	WF_API WB_NONNULL(1, 2) std::FILE*
		ufopen(const char* filename, const char* mode) wnothrowv;
	//! \brief 以 UCS-2 文件名打开文件。
	WF_API WB_NONNULL(1, 2) std::FILE*
		ufopen(const char16_t* filename, const char16_t* mode) wnothrowv;
	//@}
	//! \param mode 打开模式，基本语义与 ISO C++11 对应，具体行为取决于实现。
	//@{
	//! \brief 以 UTF-8 文件名打开文件。
	WF_API WB_NONNULL(1) std::FILE*
		ufopen(const char* filename, std::ios_base::openmode mode) wnothrowv;
	//! \brief 以 UCS-2 文件名打开文件。
	WF_API WB_NONNULL(1) std::FILE*
		ufopen(const char16_t* filename, std::ios_base::openmode mode) wnothrowv;
	//@}

	//! \note 使用 ufopen 二进制只读模式打开测试实现。
	//@{
	//! \brief 判断指定 UTF-8 文件名的文件是否存在。
	WF_API WB_NONNULL(1) bool
		ufexists(const char*) wnothrowv;
	//! \brief 判断指定 UCS-2 文件名的文件是否存在。
	WF_API WB_NONNULL(1) bool
		ufexists(const char16_t*) wnothrowv;
	//@}
	//@}

	/*!
	\brief 取当前工作目录复制至指定缓冲区中。
	\param size 缓冲区长。
	\return 若成功为第一参数，否则为空指针。
	\note 基本语义同 POSIX.1 2004 的 \c ::getcwd 。
	\note Win32 平台：当且仅当结果为根目录时以分隔符结束。
	\note 其它平台：未指定结果是否以分隔符结束。
	*/
	//@{
	//! \param buf UTF-8 缓冲区起始指针。
	WF_API WB_NONNULL(1) char*
		ugetcwd(char* buf, size_t size) wnothrowv;
	//! \param buf UCS-2 缓冲区起始指针。
	WF_API WB_NONNULL(1) char16_t*
		ugetcwd(char16_t* buf, size_t size) wnothrowv;
	//@}

	/*!
	\pre 断言：参数非空。
	\return 操作是否成功。
	\note errno 在出错时会被设置，具体值除以上明确的外，由实现定义。
	\note 参数表示路径，使用 UTF-8 编码。
	\note DS 使用 newlib 实现。 MinGW32 使用 MSVCRT 实现。 Android 使用 bionic 实现。
	其它 Linux 使用 GLibC 实现。
	*/
	//@{
	/*!
	\brief 切换当前工作路径至指定路径。
	\note POSIX 平台：除路径和返回值外语义同 \c ::chdir 。
	*/
	WF_API WB_NONNULL(1) bool
		uchdir(const char*) wnothrowv;

	/*!
	\brief 按路径以默认权限新建一个目录。
	\note 权限由实现定义： DS 使用最大权限； MinGW32 使用 \c ::_wmkdir 指定的默认权限。
	*/
	WF_API WB_NONNULL(1) bool
		umkdir(const char*) wnothrowv;

	/*!
	\brief 按路径删除一个空目录。
	\note POSIX 平台：除路径和返回值外语义同 \c ::rmdir 。
	*/
	WF_API WB_NONNULL(1) bool
		urmdir(const char*) wnothrowv;

	/*!
	\brief 按路径删除一个非目录文件。
	\note POSIX 平台：除路径和返回值外语义同 \c ::unlink 。
	\note Win32 平台：支持移除只读文件，但删除打开的文件总是失败。
	*/
	WF_API WB_NONNULL(1) bool
		uunlink(const char*) wnothrowv;

	/*!
	\brief 按路径移除一个文件。
	\note POSIX 平台：除路径和返回值外语义同 \c ::remove 。移除非空目录总是失败。
	\note Win32 平台：支持移除空目录和只读文件，但删除打开的文件总是失败。
	\see https://msdn.microsoft.com/en-us/library/kc07117k.aspx 。
	*/
	WF_API WB_NONNULL(1) bool
		uremove(const char*) wnothrowv;
	//@}
	//@}
	//@}


	/*!
	\ingroup workarounds
	*/
	//@{
#if __GLIBCXX__ || WB_IMPL_MSCPP
	/*!
	\note 扩展打开模式。
	*/
	//@{
#	if __GLIBCXX__
	//! \brief 表示仅打开已存在文件而不创建文件的模式。
	wconstexpr const auto ios_nocreate(
		std::ios_base::openmode(std::_Ios_Openmode::wimpl(_S_trunc << 1)));
	/*!
	\brief 表示仅创建不存在文件的模式。
	\note 可被 ios_nocreate 覆盖而不生效。
	*/
	wconstexpr const auto ios_noreplace(
		std::ios_base::openmode(std::_Ios_Openmode::wimpl(_S_trunc << 2)));
#	else
	wconstexpr const auto ios_nocreate(std::ios::_Nocreate);
	wconstexpr const auto ios_noreplace(std::ios::_Noreplace);
#	endif
	//@}


	template<typename _tChar, class _tTraits = std::char_traits<_tChar>>
	class basic_filebuf
#	if __GLIBCXX__
		: public wimpl(__gnu_cxx::stdio_filebuf<_tChar, _tTraits>)
#	else
		: public wimpl(std::basic_filebuf<_tChar, _tTraits>)
#	endif
	{
	public:
		using char_type = _tChar;
		using int_type = typename _tTraits::int_type;
		using pos_type = typename _tTraits::pos_type;
		using off_type = typename _tTraits::off_type;
		using traits = _tTraits;

		DefDeCtor(basic_filebuf)
#	if __GLIBCXX__
			using wimpl(__gnu_cxx::stdio_filebuf<_tChar, _tTraits>::stdio_filebuf);
#	else
			using wimpl(std::basic_filebuf<_tChar, _tTraits>::basic_filebuf);
#	endif
		DefDeCopyMoveCtorAssignment(basic_filebuf)

	public:
		/*!
		\note 忽略扩展模式。
		*/
		basic_filebuf<_tChar, _tTraits>*
			open(UniqueFile p, std::ios_base::openmode mode)
		{
			if (p)
			{
				mode &= ~(ios_nocreate | ios_noreplace);
#	if __GLIBCXX__
				this->_M_file.sys_open(*p.get(), mode);
				if (open_check(mode))
				{
					p.release();
					return this;
				}
#	else
				if (!this->is_open())
					if (const auto mode_str = white::openmode_conv(mode))
						if (open_file_ptr(::_fdopen(*p.get(), mode_str))) {
							p.release();
							return this;
						}
#	endif
			}
			return {};
		}
		template<typename _tPathChar>
		std::basic_filebuf<_tChar, _tTraits>*
			open(const _tPathChar* s, std::ios_base::openmode mode)
		{
			if (!this->is_open())
			{
#	if __GLIBCXX__
				this->_M_file.sys_open(uopen(s, omode_convb(mode)), mode);
				if (open_check(mode))
					return this;
#	else
				return open_file_ptr(std::_Fiopen(s, mode,
					int(std::ios_base::_Openprot)));
#	endif
			}
			return {};
		}
		template<class _tString,
			wimpl(typename = white::enable_for_string_class_t<_tString>)>
			std::basic_filebuf<_tChar, _tTraits>*
			open(const _tString& s, std::ios_base::openmode mode)
		{
			return open(s.c_str(), mode);
		}

	private:
#	if __GLIBCXX__
		bool
			open_check(std::ios_base::openmode mode)
		{
			if (this->is_open())
			{
				this->_M_allocate_internal_buffer();
				this->_M_mode = mode;
				wunseq(this->_M_reading = {}, this->_M_writing = {});
				this->_M_set_buffer(-1);
				wunseq(this->_M_state_cur = this->_M_state_beg,
					this->_M_state_last = this->_M_state_beg);
				if ((mode & std::ios_base::ate) && this->seekoff(0,
					std::ios_base::end) == pos_type(off_type(-1)))
					this->close();
				else
					return true;
				return true;
			}
			return {};
		}
#	else
		WB_NONNULL(1) basic_filebuf<_tChar, _tTraits>*
			open_file_ptr(::FILE* p_file)
		{
			if (p_file)
			{
				this->_Init(p_file, std::basic_filebuf<_tChar, _tTraits>::_Openfl);
				this->_Initcvt(
#if WB_IMPL_MSCPP < 1914 
					&
#endif
					std::use_facet<std::codecvt<_tChar, char,
					typename _tTraits::state_type>>(
						std::basic_streambuf<_tChar, _tTraits>::getloc()));
				return this;
			}
			return {};
		}
#	endif
	};


	//extern template class WF_API basic_filebuf<char>;
	//extern template class WF_API basic_filebuf<wchar_t>;

	using filebuf = basic_filebuf<char>;
	using wfilebuf = basic_filebuf<wchar_t>;


	//@{
	template<typename _tChar, class _tTraits = std::char_traits<_tChar>>
	class basic_ifstream : public std::basic_istream<_tChar, _tTraits>
	{
	public:
		using char_type = _tChar;
		using int_type = typename _tTraits::int_type;
		using pos_type = typename _tTraits::pos_type;
		using off_type = typename _tTraits::off_type;
		using traits_type = _tTraits;

	private:
		using base_type = std::basic_istream<char_type, traits_type>;

		mutable basic_filebuf<_tChar, _tTraits> fbuf{};

	public:
		basic_ifstream()
			: base_type(nullptr)
		{
			this->init(&fbuf);
		}
		template<typename _tParam,
			wimpl(typename = white::exclude_self_t<basic_ifstream, _tParam>)>
			explicit
			basic_ifstream(_tParam&& s,
				std::ios_base::openmode mode = std::ios_base::in)
			: base_type(nullptr)
		{
			this->init(&fbuf);
			this->open(wforward(s), mode);
		}
		DefDelCopyCtor(basic_ifstream)
			basic_ifstream(basic_ifstream&& rhs)
			: base_type(std::move(rhs)),
			fbuf(std::move(rhs.fbuf))
		{
			base_type::set_rdbuf(&fbuf);
		}
		DefDeDtor(basic_ifstream)

			DefDelCopyAssignment(basic_ifstream)
			basic_ifstream&
			operator=(basic_ifstream&& rhs)
		{
			base_type::operator=(std::move(rhs));
			fbuf = std::move(rhs.fbuf);
			return *this;
		}

		void
			swap(basic_ifstream& rhs)
		{
			base_type::swap(rhs),
				fbuf.swap(rhs.fbuf);
		}

		void
			close()
		{
			if (!fbuf.close())
				this->setstate(std::ios_base::failbit);
		}

		bool
			is_open() const
		{
			return fbuf.is_open();
		}

		template<typename _tParam>
		void
			open(_tParam&& s,
				std::ios_base::openmode mode = std::ios_base::in)
		{
			if (fbuf.open(wforward(s), mode))
				this->clear();
			else
				this->setstate(std::ios_base::failbit);
		}

		WB_ATTR_returns_nonnull std::basic_filebuf<_tChar, _tTraits>*
			rdbuf() const
		{
			return &fbuf;
		}
	};

	template<typename _tChar, class _tTraits>
	inline DefSwap(, basic_ifstream<_tChar WPP_Comma _tTraits>)


		template<typename _tChar, class _tTraits = std::char_traits<_tChar>>
	class basic_ofstream : public std::basic_ostream<_tChar, _tTraits>
	{
	public:
		using char_type = _tChar;
		using int_type = typename _tTraits::int_type;
		using pos_type = typename _tTraits::pos_type;
		using off_type = typename _tTraits::off_type;
		using traits_type = _tTraits;

	private:
		using base_type = std::basic_ostream<char_type, traits_type>;

		mutable basic_filebuf<_tChar, _tTraits> fbuf{};

	public:
		basic_ofstream()
			: base_type(nullptr)
		{
			this->init(&fbuf);
		}
		template<typename _tParam,
			wimpl(typename = white::exclude_self_t<basic_ofstream, _tParam>)>
			explicit
			basic_ofstream(_tParam&& s,
				std::ios_base::openmode mode = std::ios_base::out)
			: base_type(nullptr)
		{
			this->init(&fbuf);
			this->open(wforward(s), mode);
		}
		DefDelCopyCtor(basic_ofstream)
			basic_ofstream(basic_ofstream&& rhs)
			: base_type(std::move(rhs)),
			fbuf(std::move(rhs.fbuf))
		{
			base_type::set_rdbuf(&fbuf);
		}
		DefDeDtor(basic_ofstream)

			DefDelCopyAssignment(basic_ofstream)
			basic_ofstream&
			operator=(basic_ofstream&& rhs)
		{
			base_type::operator=(std::move(rhs));
			fbuf = std::move(rhs.fbuf);
			return *this;
		}

		void
			swap(basic_ofstream& rhs)
		{
			base_type::swap(rhs),
				fbuf.swap(rhs.fbuf);
		}

		void
			close()
		{
			if (!fbuf.close())
				this->setstate(std::ios_base::failbit);
		}

		bool
			is_open() const
		{
			return fbuf.is_open();
		}

		template<typename _tParam>
		void
			open(_tParam&& s, std::ios_base::openmode mode = std::ios_base::out)
		{
			if (fbuf.open(wforward(s), mode))
				this->clear();
			else
				this->setstate(std::ios_base::failbit);
		}

		WB_ATTR_returns_nonnull std::basic_filebuf<_tChar, _tTraits>*
			rdbuf() const
		{
			return &fbuf;
		}
	};

	template<typename _tChar, class _tTraits>
	inline DefSwap(, basic_ofstream<_tChar WPP_Comma _tTraits>)
		//@}


		template<typename _tChar, class _tTraits = std::char_traits<_tChar>>
	class basic_fstream : public std::basic_iostream<_tChar, _tTraits>
	{
	public:
		using char_type = _tChar;
		using int_type = typename _tTraits::int_type;
		using pos_type = typename _tTraits::pos_type;
		using off_type = typename _tTraits::off_type;
		using traits_type = _tTraits;

	private:
		using base_type = std::basic_iostream<char_type, traits_type>;

		mutable basic_filebuf<_tChar, _tTraits> fbuf{};

	public:
		basic_fstream()
			: base_type(nullptr)
		{
			this->init(&fbuf);
		}
		template<typename _tParam,
			wimpl(typename = white::exclude_self_t<basic_fstream, _tParam>)>
			explicit
			basic_fstream(_tParam&& s,
				std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out)
			: base_type(nullptr)
		{
			this->init(&fbuf);
			this->open(wforward(s), mode);
		}
		DefDelCopyCtor(basic_fstream)
			basic_fstream(basic_fstream&& rhs)
			: base_type(std::move(rhs)),
			fbuf(std::move(rhs.fbuf))
		{
			base_type::set_rdbuf(&fbuf);
		}
		DefDeDtor(basic_fstream)

			DefDelCopyAssignment(basic_fstream)
			basic_fstream&
			operator=(basic_fstream&& rhs)
		{
			base_type::operator=(std::move(rhs));
			fbuf = std::move(rhs.fbuf);
			return *this;
		}

		void
			swap(basic_fstream& rhs)
		{
			base_type::swap(rhs),
				fbuf.swap(rhs.fbuf);
		}

		void
			close()
		{
			if (!fbuf.close())
				this->setstate(std::ios_base::failbit);
		}

		bool
			is_open() const
		{
			return fbuf.is_open();
		}

		template<typename _tParam>
		void
			open(_tParam&& s,
				std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out)
		{
			if (fbuf.open(wforward(s), mode))
				this->clear();
			else
				this->setstate(std::ios_base::failbit);
		}

		WB_ATTR_returns_nonnull std::basic_filebuf<_tChar, _tTraits>*
			rdbuf() const
		{
			return &fbuf;
		}
	};

	template<typename _tChar, class _tTraits>
	inline DefSwap(, basic_fstream<_tChar WPP_Comma _tTraits>)


		//extern template class WF_API basic_fstream<char>;
		//extern template class WF_API basic_fstream<wchar_t>;

		//@{
		using ifstream = basic_ifstream<char>;
	using ofstream = basic_ofstream<char>;
	using fstream = basic_fstream<char>;
	using wifstream = basic_ifstream<wchar_t>;
	using wofstream = basic_ofstream<wchar_t>;
	//@}
	using wfstream = basic_fstream<wchar_t>;
#else
	// TODO: Use VC++ extensions to support %char16_t path initialization.
	using std::basic_filebuf;
	using std::filebuf;
	using std::wfilebuf;
	//! \since build 619
	//@{
	using std::basic_ifstream;
	using std::basic_ofstream;
	//! \since build 616
	using std::basic_fstream;
	using std::ifstream;
	using std::ofstream;
	//! \since build 616
	using std::fstream;
	using std::wifstream;
	using std::wofstream;
	//@}
	using std::wfstream;
#endif
	//@}


	//@{
	/*!
	\brief 尝试按指定的起始缓冲区大小取当前工作目录的路径。
	\pre 间接断言：参数不等于 0 。
	\note 未指定结果是否以分隔符结束。
	*/
	template<typename _tChar>
	basic_string<_tChar>
		FetchCurrentWorkingDirectory(size_t init)
	{
		return white::retry_for_vector<basic_string<_tChar>>(init,
			[](basic_string<_tChar>& res, size_t) -> bool {
			const auto r(platform::ugetcwd(&res[0], res.length()));

			if (r)
			{
				res = r;
				return {};
			}

			const int err(errno);

			if (err != ERANGE)
				white::throw_error(err, wfsig);
			return true;
		});
	}
#if WFL_Win32
	//! \note 参数被忽略。
	//@{
	template<>
	WF_API string
		FetchCurrentWorkingDirectory(size_t);
	template<>
	WF_API u16string
		FetchCurrentWorkingDirectory(size_t);
	//@}
#endif
	//@}


	//@{
	/*!
	\note 省略第一参数时为 std::system_error 。
	*/
	//@{
	/*!
	\brief 按错误值和指定参数抛出第一参数指定类型的对象。
	\note 先保存可能是左值的 errno 以避免参数中的副作用影响结果。
	*/
#define WCL_Raise_SysE(_t, _msg, _sig) \
	do \
	{ \
		const auto err_(errno); \
	\
		white::throw_error<_t>(err_, \
			platform::ComposeMessageWithSignature(_msg WPP_Comma _sig)); \
	}while(false)

	//! \note 按表达式求值，检查是否为零初始化的值。
#define WCL_RaiseZ_SysE(_t, _expr, _msg, _sig) \
	do \
	{ \
		const auto err_(_expr); \
	\
		if(err_ != decltype(err_)()) \
			white::throw_error<_t>(err_, \
				platform::ComposeMessageWithSignature(_msg WPP_Comma _sig)); \
	}while(false)
	//@}

	/*!
	\brief 跟踪 errno 取得的调用状态结果。
	*/
#define WCL_Trace_SysE(_lv, _fn, _sig) \
	do \
	{ \
		const int err_(errno); \
	\
		TraceDe(_lv, "Failed calling " #_fn " @ %s with error %d: %s.", \
			_sig, err_, std::strerror(err_)); \
	}while(false)

	/*!
	\brief 调用系统 C API 或其它可用 errno 取得调用状态的例程。
	\pre 系统 C API 返回结果类型满足 DefaultConstructible 和 LessThanComparable 要求。
	\note 比较返回默认构造的结果值，相等表示成功，小于表示失败且设置 errno 。
	\note 调用时直接使用实际参数，可指定非标识符的表达式，不保证是全局名称。
	*/
	//@{
	/*!
	\note 若失败抛出第一参数指定类型的对象。
	\note 省略第一参数时为 std::system_error 。
	\sa WCL_Raise_SysE
	*/
	//@{
#define WCL_WrapCall_CAPI(_t, _fn, ...) \
	[&](const char* sig_) WB_NONNULL(1){ \
		const auto res_(_fn(__VA_ARGS__)); \
	\
		if WB_UNLIKELY(res_ < decltype(res_)()) \
			WCL_Raise_SysE(_t, #_fn, sig_); \
		return res_; \
	}

#define WCL_Call_CAPI(_t, _fn, _sig, ...) \
	WCL_WrapCall_CAPI(_t, _fn, __VA_ARGS__)(_sig)

#define WCL_CallF_CAPI(_t, _fn, ...) WCL_Call_CAPI(_t, _fn, wfsig, __VA_ARGS__)
	//@}

	/*!
	\note 若失败跟踪 errno 的结果。
	\note 格式转换说明符置于最前以避免宏参数影响结果。
	\sa WCL_Trace_SysE
	*/
	//@{
#define WCL_TraceWrapCall_CAPI(_fn, ...) \
	[&](const char* sig_) WB_NONNULL(1){ \
		const auto res_(_fn(__VA_ARGS__)); \
	\
		if(WB_UNLIKELY(res_ < decltype(res_)())) \
			WCL_Trace_SysE(platform::Descriptions::Warning, _fn, sig_); \
		return res_; \
	}

#define WCL_TraceCall_CAPI(_fn, _sig, ...) \
	WCL_TraceWrapCall_CAPI(_fn, __VA_ARGS__)(_sig)

#define WCL_TraceCallF_CAPI(_fn, ...) \
	WCL_TraceCall_CAPI(_fn, wfsig, __VA_ARGS__)
	//@}
	//@}
	//@}


	//! \exception std::system_error 参数无效或文件时间查询失败。
	//@{
	/*!
	\sa FileDescriptor::GetAccessTime
	*/
	//@{
	inline WB_NONNULL(1) PDefH(FileTime, GetFileAccessTimeOf, std::FILE* fp)
		ImplRet(FileDescriptor(fp).GetAccessTime())
		/*!
		\pre 断言：第一参数非空。
		\note 最后参数表示跟随连接：若文件系统支持，访问链接的文件而不是链接自身。
		*/
		//@{
		WF_API WB_NONNULL(1) FileTime
		GetFileAccessTimeOf(const char*, bool = {});
	WF_API WB_NONNULL(1) FileTime
		GetFileAccessTimeOf(const char16_t*, bool = {});
	//@}
	//@}

	/*!
	\sa FileDescriptor::GetModificationTime
	*/
	//@{
	inline WB_NONNULL(1) PDefH(FileTime, GetFileModificationTimeOf, std::FILE* fp)
		ImplRet(FileDescriptor(fp).GetModificationTime())

		/*!
		\pre 断言：第一参数非空。
		\note 最后参数表示跟随连接：若文件系统支持，访问链接的文件而不是链接自身。
		*/
		//@{
		WF_API WB_NONNULL(1) FileTime
		GetFileModificationTimeOf(const char*, bool = {});
	WF_API WB_NONNULL(1) FileTime
		GetFileModificationTimeOf(const char16_t*, bool = {});
	//@}
	//@}

	/*!
	\sa FileDescriptor::GetModificationAndAccessTime
	*/
	//@{
	inline WB_NONNULL(1) PDefH(array<FileTime WPP_Comma 2>,
		GetFileModificationAndAccessTimeOf, std::FILE* fp)
		ImplRet(FileDescriptor(fp).GetModificationAndAccessTime())
		/*!
		\pre 断言：第一参数非空。
		\note 最后参数表示跟随连接：若文件系统支持，访问链接的文件而不是链接自身。
		*/
		//@{
		WF_API WB_NONNULL(1) array<FileTime, 2>
		GetFileModificationAndAccessTimeOf(const char*, bool = {});
	WF_API WB_NONNULL(1) array<FileTime, 2>
		GetFileModificationAndAccessTimeOf(const char16_t*, bool = {});
	//@}
	//@}
	//@}

	/*!
	\brief 取路径指定的文件链接数。
	\return 若成功为连接数，否则若文件不存在时为 0 。
	\note 最后参数表示跟随连接：若文件系统支持，访问链接的文件而不是链接自身。
	*/
	//@{
	WB_NONNULL(1) size_t
		FetchNumberOfLinks(const char*, bool = {});
	WB_NONNULL(1) size_t
		FetchNumberOfLinks(const char16_t*, bool = {});
	//@}


	/*!
	\brief 尝试删除指定路径的文件后再以指定路径和模式创建常规文件。
	\pre 间接断言：第一参数非空。
	\note 参数依次表示目标路径、打开模式、覆盖目标允许的链接数和是否允许共享覆盖。
	\note 第二参数被限制 Mode::User 内。
	\note 若目标已存在，创建或覆盖文件保证目标文件链接数不超过第三参数指定的值。
	\note 若目标已存在、链接数大于 1 且不允许写入共享则先删除目标。
	\note 忽略目标不存在导致的删除失败。
	\throw std::system_error 创建目标失败。
	*/
	WF_API WB_NONNULL(1) UniqueFile
		EnsureUniqueFile(const char*, mode_t = DefaultPMode(), size_t = 1, bool = {});
	//@}

	/*!
	\brief 比较文件内容相等。
	\throw std::system_error 文件按流打开失败。
	\warning 读取失败时即截断返回，因此需要另行比较文件大小。
	\sa IsNodeShared

	首先比较文件节点，若为相同文件直接相等，否则清除 errno ，打开文件读取内容并比较。
	*/
	//@{
	//! \note 间接断言：参数非空。
	//@{
	WF_API WB_NONNULL(1, 2) bool
		HaveSameContents(const char*, const char*, mode_t = DefaultPMode());
	//! \since build 701
	WF_API WB_NONNULL(1, 2) bool
		HaveSameContents(const char16_t*, const char16_t*, mode_t = DefaultPMode());
	//@}
	/*!
	\note 使用表示文件名称的字符串，仅用于在异常消息中显示（若为空则省略）。
	\note 参数表示的文件已以可读形式打开，否则按流打开失败。
	\note 视文件起始于当前读位置。
	*/
	WF_API bool
		HaveSameContents(UniqueFile, UniqueFile, const char*, const char*);
	//@}

	/*!
	\brief 判断参数是否表示共享的文件节点。
	\note 可能设置 errno 。
	*/
	//@{
	wconstfn WB_PURE PDefH(bool, IsNodeShared, const FileNodeID& x,
		const FileNodeID& y) wnothrow
		ImplRet(x != FileNodeID() && x == y)
		/*!
		\pre 间接断言：字符串参数非空。
		\note 最后参数表示跟踪连接。
		\since build 660
		*/
		//@{
		WF_API WB_NONNULL(1, 2) bool
		IsNodeShared(const char*, const char*, bool = true) wnothrowv;
	WF_API WB_NONNULL(1, 2) bool
		IsNodeShared(const char16_t*, const char16_t*, bool = true) wnothrowv;
	//@}
	/*!
	\note 取节点失败视为不共享。
	\sa FileDescriptor::GetNodeID
	*/
	bool
		IsNodeShared(FileDescriptor, FileDescriptor) wnothrow;
	//@}

} // namespace platform;

namespace platform_ex
{

	//@{
#if WFL_Win32
	/*!
	\brief 构造适合表示路径的 \c char16_t 字符串。
	\note 字符类型非 \c char16_t 时转换。
	*/
	//@{
	//! \pre 间接断言：参数非空。
	inline WB_NONNULL(1) PDefH(wstring, MakePathStringW, const wchar_t* s)
		ImplRet(platform::Nonnull(s))
		inline PDefH(const wstring&, MakePathStringW, const wstring& s)
		ImplRet(s)
		//! \pre 间接断言：参数非空。
		WF_API WB_NONNULL(1) wstring
		MakePathStringW(const char*);
	inline PDefH(wstring, MakePathStringW, string_view sv)
		ImplRet(MakePathStringW(sv.data()))
		//@}
#else
	/*!
	\brief 构造适合表示路径的 \c char16_t 字符串。
	\note 字符类型非 \c char16_t 时转换。
	*/
	//@{
	//! \pre 间接断言：参数非空。
	inline WB_NONNULL(1) PDefH(u16string, MakePathStringU, const char16_t* s)
		ImplRet(platform::Nonnull(s))
		inline PDefH(const u16string&, MakePathStringU, const u16string& s)
		ImplRet(s)
		//! \pre 间接断言：参数非空。
		WF_API WB_NONNULL(1) u16string
		MakePathStringU(const char*);
	inline PDefH(u16string, MakePathStringU, string_view sv)
		ImplRet(MakePathStringU(sv.data()))
		//@}
#endif
		//@}

} // namespace platform_ex;



#endif