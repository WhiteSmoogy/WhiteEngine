/*!	\file Platform.h
\ingroup Framework
\brief 平台相关的公共组件无关函数与宏定义集合。
*/

#ifndef FrameWork_Common_h
#define FrameWork_Common_h 1

#include <WFramework/WCLib/Platform.h>
#include <WBase/functional.hpp>
#include <WBase/cassert.h>
#include <WBase/cwctype.h>
#include <WBase/cstring.h>
#include <WBase/wmacro.h>

namespace platform
{

	/*!
	\ingroup Platforms
	*/
	//@{
	using ID = std::uintmax_t;

	template<ID _vN>
	using MetaID = white::integral_constant<ID, _vN>;

	//! \brief 平台标识的公共标记类型：指定任意平台。
	struct IDTagBase
	{};

#if WB_IMPL_CLANGPP
#	pragma GCC diagnostic push
#	pragma GCC diagnostic ignored "-Wweak-vtables"
#endif

	//! \brief 宿主平台标识的公共标记类型：指定任意宿主平台。
	struct HostedIDTagBase : virtual IDTagBase
	{};

	//! \brief 平台标识的整数标记类型。
	template<ID _vN>
	struct IDTag : virtual IDTagBase, virtual MetaID<_vN>
	{};

	//! \brief 指定特定基类类型的 IDTag 特化。
#define WFL_IDTag_Spec(_n, _base) \
	template<> \
	struct IDTag<WF_Platform_##_n> : virtual _base, \
		virtual MetaID<WF_Platform_##_n> \
	{};

	//! \brief 指定宿主平台的 IDTag 特化。
#define WFL_IDTag_SpecHost(_n) \
	WFL_IDTag_Spec(_n, HostedIDTagBase)

	//! \brief 指定基于特定平台的 IDTag 特化。
#define WFL_IDTag_SpecOnBase(_n, _b) \
	WFL_IDTag_Spec(_n, IDTag<WF_Platform_##_b>)

	WFL_IDTag_SpecHost(Win32)
		WFL_IDTag_SpecOnBase(Win64, Win32)
		WFL_IDTag_SpecOnBase(MinGW32, Win32)
		WFL_IDTag_SpecOnBase(MinGW64, Win64)
		//WFL_IDTag_SpecHost(Linux)
		//WFL_IDTag_SpecOnBase(Linux_x86, Linux)
		//WFL_IDTag_SpecOnBase(Linux_x64, Linux)
		//WFL_IDTag_SpecOnBase(Android, Linux)
		//WFL_IDTag_SpecOnBase(Android_ARM, Android)

		//! \brief 取平台标识的整数标记集合类型。
		template<ID... _vN>
	struct IDTagSet : virtual IDTag<_vN>...
	{};
	//@}

#if WB_IMPL_CLANGPP
#	pragma GCC diagnostic pop
#endif

#define WCL_Tag_constfn inline


	/*!
	\ingroup PlatformEmulation
	\brief 定义用于平台模拟的传递模板。
	*/
#define WCL_DefPlatformFwdTmpl(_n, _fn) \
	DefFwdTmplAuto(_n, _fn(platform::IDTag<WF_Platform>(), wforward(args)...))


	/*!
	\brief 声明检查存在合式调用的辅助宏。
	\sa white::is_detected
	*/
	//@{
#define WCL_CheckDecl_t(_fn) CheckDecl##_fn##_t
#define WCL_DeclCheck_t(_fn, _call) \
	template<typename... _tParams> \
	using WCL_CheckDecl_t(_fn) \
		= decltype(_call(std::declval<_tParams&&>()...));
	//@}


	/*!
	\brief 异常终止函数。
	*/
	WB_NORETURN WB_API void
		terminate() wnothrow;


	  /*!
	  \brief 检查默认区域下指定字符是否为可打印字符。
	  \note MSVCRT 的 isprint/iswprint 实现缺陷的变通。
	  \sa https://connect.microsoft.com/VisualStudio/feedback/details/799287/isprint-incorrectly-classifies-t-as-printable-in-c-locale
	  */
	  //@{
	inline PDefH(bool, IsPrint, char c)
		ImplRet(white::isprint(c))
		inline PDefH(bool, IsPrint, wchar_t c)
		ImplRet(stdex::iswprint(c))
		template<typename _tChar>
	bool
		IsPrint(_tChar c)
	{
		return platform::IsPrint(wchar_t(c));
	}
	//@}


	//@{
	inline PDefH(white::uchar_t*, ucast, wchar_t* p) wnothrow
		ImplRet(white::replace_cast<white::uchar_t*>(p))
		inline PDefH(const white::uchar_t*, ucast, const wchar_t* p) wnothrow
		ImplRet(white::replace_cast<const white::uchar_t*>(p))
		template<typename _tChar>
	_tChar*
		ucast(_tChar* p) wnothrow
	{
		return p;
	}

	inline PDefH(wchar_t*, wcast, white::uchar_t* p) wnothrow
		ImplRet(white::replace_cast<wchar_t*>(p))
		inline PDefH(const wchar_t*, wcast, const white::uchar_t* p) wnothrow
		ImplRet(white::replace_cast<const wchar_t*>(p))
		template<typename _tChar>
	_tChar*
		wcast(_tChar* p) wnothrow
	{
		return p;
	}
	//@}

	/*!
	\brief 调用并捕获异常。
	*/
	template<typename _func, typename... _tParams>
	white::invoke_result_t<_func,_tParams...>
		CallNothrow(const white::invoke_result_t<_func,_tParams...>& v, _func f,
			_tParams&&... args) wnothrowv
	{
		TryRet(f(wforward(args)...))
			CatchExpr(std::bad_alloc&, errno = ENOMEM)
			CatchIgnore(...)
			return v;
	}

	/*!
	\brief 循环重复操作。
	*/
	template<typename _func, typename _tErrorRef,
		typename _tError = white::decay_t<_tErrorRef>,
		typename _type = white::invoke_result_t<_func&>>
		_type
		RetryOnError(_func f, _tErrorRef&& err, _tError e = _tError())
	{
		return white::retry_on_cond([&](_type res) {
			return res < _type() && _tError(err) == e;
		}, f);
	}

	template<typename _func, typename _type = white::invoke_result_t<_func&>>
	inline _type
		RetryOnInterrupted(_func f)
	{
		return RetryOnError(f, errno, EINTR);
	}

	/*!
	\brief 执行 UTF-8 字符串的环境命令。
	\note 使用 std::system 实现；若参数为空则和 std::system 行为一致。
	*/
	WB_API int
		usystem(const char*);



	//@{
	/*!
	\brief 系统配置选项。
	\note 以 Max 起始的名称表示可具有的最大值。
	*/
	enum class SystemOption
	{
		/*!
		\brief 内存页面字节数。
		\note 0 表示不支持分页。
		*/
		PageSize,
		//@{
		//! \brief 一个进程中可具有的信号量的最大数量。
		MaxSemaphoreNumber,
		//! \brief 可具有的信号量的最大值。
		MaxSemaphoreValue,
		//@}
		//! \brief 在路径解析中符号链接可被可靠遍历的最大次数。
		MaxSymlinkLoop
	};


	/*!
	\brief 取限制配置。
	\return 选项存在且值能被返回类型表示时为转换后的对应选项值，否则为默认常数值。
	\note 若无 WB_STATELESS 修饰，当找不到项时，可能使用跟踪输出警告。
	\note 若无 WB_STATELESS 修饰，当找不到项时，可能设置 errno 为 EINVAL 。
	\sa SystemOption
	\sa TraceDe

	以配置选项值作为参数，指定查询特定的限制值。
	某些平台调用结果总是翻译时确定的常数，调用此函数无副作用，使用 WB_STATELESS 修饰；
	其它平台可能由运行时确定，通过查询宿主环境或语言运行时提供的接口，
	确定选项是否存在，以及具体的配置的值。
	若选项存在，且对应的值可以 size_t 表示，则返回选项对应的值；
	否则，返回一个默认的常数值作为回退，表示在特定环境下能可靠依赖的默认限制。
	注意回退值可能平台相关；
	但对 POSIX 提供最小值且符合选项含义的情形，使用 POSIX 最小值以提升可移植性。
	回退值可能不等于实现实际支持的值。
	对表示可具有的最大值的限制，回退值为 \c size_t(-1) ，此时实现实际支持的值可能超过
	size_t 的表示范围，或没有指定显式限制（仅受存储或地址空间限制）；
	其它回退值取决于选项的具体含义。
	*/
	WF_API
		size_t
		FetchLimit(SystemOption) wnothrow;
	//@}

} // namespace platform;

#endif