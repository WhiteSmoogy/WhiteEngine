/*!	\file Host.h
\ingroup Framework
\brief 宿主平台公共扩展。
*/

#ifndef FrameWork_Host_h
#define FrameWork_Host_h 1

#include <WFramework/WCLib/FContainer.h>
#include <WFramework/Core/WException.h>
#include <WFramework/WCLib/FReference.h>
#include <system_error>
#if !WFL_Win32
#include <WBase/FFileIO.h>
#else
using HANDLE = void *;
#endif

#if WF_Hosted

namespace platform_ex
{
	/*!
	\ingroup exceptions
	\brief 宿主异常。
	*/
	class WB_API Exception : public std::system_error
	{
	private:
		platform::Descriptions::RecordLevel level = platform::Descriptions::RecordLevel::Emergent;

	public:
		/*!
		\pre 间接断言：字符串参数对应的数据指针非空。
		*/
		//@{
		WB_NONNULL(3)
			Exception(std::error_code, const char* = "unknown host exception",
				platform::Descriptions::RecordLevel = platform::Descriptions::RecordLevel::Emergent);
		Exception(std::error_code, string_view,
			platform::Descriptions::RecordLevel = platform::Descriptions::RecordLevel::Emergent);
		WB_NONNULL(4)
			Exception(int, const std::error_category&, const char*
				= "unknown host exception", platform::Descriptions::RecordLevel = platform::Descriptions::RecordLevel::Emergent);
		Exception(int, const std::error_category&, string_view,
			platform::Descriptions::RecordLevel = platform::Descriptions::RecordLevel::Emergent);
		//@}
		DefDeCopyCtor(Exception)
			/*!
			\brief 虚析构：类定义外默认实现。
			*/
			~Exception() override;

		white::RecordLevel GetLevel() const wnothrow;
	};


	/*!
	\brief 设置环境变量。
	\pre 断言：参数非空。
	\warning 不保证线程安全。
	\throw std::system_error 设置失败。
	*/
	WF_API WB_NONNULL(1, 2) void
		SetEnvironmentVariable(const char*, const char*);

#	if WFL_Win32
	/*!
	\brief 句柄删除器。
	*/
	struct WB_API HandleDelete
	{
		/*!
		\warning 只检查空句柄作为空值。对不同的 Win32 API 可能需要额外检查。
		\see http://blogs.msdn.com/b/oldnewthing/archive/2004/03/02/82639.aspx 。
		*/
		using pointer = ::HANDLE;

		void
			operator()(pointer) const wnothrow;
	};
//#	elif WFL_API_Has_unistd_h
//	using HandleDelete = platform::FileDescriptor::Deleter;
#	else
#		error "Unsupported platform found."
#	endif

	using UniqueHandle = platform::references::unique_ptr_from<HandleDelete>;


	//@{
	//! \brief 默认命令缓冲区大小。
	wconstexpr const size_t DefaultCommandBufferSize(wimpl(4096));

	/*!
	\brief 取命令在标准输出上的执行结果。
	\pre 间接断言：第一参数非空。
	\return 读取的二进制存储和关闭管道的返回值（可来自被调用的命令）。
	\exception std::system_error 读取失败。
	\throw std::invalid_argument 第二参数的值等于 \c 0 。
	\throw std::system_error 表示读取失败的派生类异常对象。
	\note 第一参数指定命令；第二参数指定每次读取的缓冲区大小，先于执行命令进行检查。
	*/
	WB_API WB_NONNULL(1) pair<string, int>
		FetchCommandOutput(const char*, size_t = DefaultCommandBufferSize);


	//! \brief 命令和命令执行结果的缓冲区类型。
	using CommandCache = platform::containers::unordered_map<platform::containers::string, platform::containers::string>;

	/*!
	\brief 锁定命令执行缓冲区。
	\return 静态对象的非空锁定指针。
	*/
	WB_API white::locked_ptr<CommandCache>
		LockCommandCache();

	//@{
	//! \brief 取缓冲的命令执行结果。
	WB_API const string&
		FetchCachedCommandResult(const string&, size_t = DefaultCommandBufferSize);

	//! \brief 取缓冲的命令执行结果字符串。
	inline PDefH(string, FetchCachedCommandString, const string& cmd,
		size_t buf_size = DefaultCommandBufferSize)
		ImplRet(white::trail(string(FetchCachedCommandResult(cmd, buf_size))))
		//@}
		//@}

		/*!
		\brief 创建管道。
		\throw std::system_error 表示创建失败的派生类异常对象。
		*/
		WB_API pair<UniqueHandle, UniqueHandle>
		MakePipe();


	/*!
	\brief 从外部环境编码字符串参数或解码为外部环境字符串参数。
	\pre Win32 平台可能间接断言参数非空。

	对 Win32 平台调用当前代码页的 platform::MBCSToMBCS 编解码字符串，其它直接传递参数。
	此时和 platform::MBCSToMBCS 不同，参数可转换为 \c string_view 时长度通过 NTCTS 计算。
	若需要使用 <tt>const char*</tt> 指针，可直接使用 <tt>&arg[0]</tt> 的形式。
	*/
	//@{
#	if WFL_Win32
	WB_API WB_NONNULL(1) string
		DecodeArg(const char*);
	inline PDefH(string, DecodeArg, string_view arg)
		ImplRet(DecodeArg(&arg[0]))
#	endif
		template<typename _type
#if WFL_Win32
		, wimpl(typename = white::enable_if_t<
			!std::is_constructible<string_view, _type&&>::value>)
#endif
		>
		wconstfn auto
		DecodeArg(_type&& arg) -> decltype(wforward(arg))
	{
		return wforward(arg);
	}

#	if WFL_Win32
	WB_API WB_NONNULL(1) string
		EncodeArg(const char*);
	inline PDefH(string, EncodeArg, const string& arg)
		ImplRet(EncodeArg(&arg[0]))
#	endif
		template<typename _type
#if WFL_Win32
		, wimpl(typename = white::enable_if_t<!std::is_constructible<string,
			_type&&>::value>)
#endif
		>
		wconstfn auto
		EncodeArg(_type&& arg) -> decltype(wforward(arg))
	{
		return wforward(arg);
	}
	//@}


#	if !YCL_Android
	//@{
	/*!
	\brief 终端数据。
	\note 非公开实现。
	*/
	class TerminalData;

	/*!
	\brief 终端。
	\note 非 Win32 平台使用 \c tput 实现，多终端改变当前屏幕时可能引起未预期的行为。
	\warning 非虚析构。
	*/
	class WB_API Terminal
	{
	public:
		/*!
		\brief 终端界面状态守护。
		*/
		class WB_API Guard final
		{
		private:
			tidy_ptr<Terminal> p_terminal;

		public:
			template<typename _fCallable, typename... _tParams>
			Guard(Terminal& te, _fCallable f, _tParams&&... args)
				: p_terminal(make_observer(&te))
			{
				if (te)
					white::invoke(f, te, wforward(args)...);
			}

			DefDeMoveCtor(Guard)

			//! \brief 析构：重置终端属性，截获并记录错误。
			~Guard();

			DefDeMoveAssignment(Guard)

			DefCvt(const wnothrow, white::add_ptr_t<
					std::ios_base&(std::ios_base&)>, white::id<std::ios_base&>())
		};

	private:
		unique_ptr<TerminalData> p_data;

	public:
		/*!
		\brief 构造：使用标准流对应的文件指针。
		\pre 间接断言：参数非空。
		\note 非 Win32 平台下关闭参数指定的流的缓冲以避免 \tput 不同步。
		*/
		Terminal(std::FILE* = stdout);
		~Terminal();

		//! \brief 判断终端有效或无效。
		DefBoolNeg(explicit, bool(p_data))


			//@{
			//! \brief 清除显示的内容。
			bool
			Clear();

		/*!
		\brief 临时固定前景色。
		\sa UpdateForeColor
		*/
		Guard
			LockForeColor(std::uint8_t);
		//@}

			bool
			RestoreAttributes();

		bool
			UpdateForeColor(std::uint8_t);
	};
	//@}

	/*!
	\brief 根据等级设置终端的前景色。
	\return 终端是否有效。
	\note 当终端无效时忽略。
	\relates Terminal
	*/
	WB_API bool
		UpdateForeColorByLevel(Terminal&, platform::Descriptions::RecordLevel);
#	endif

} // namespace platform_ex;

#endif

#endif