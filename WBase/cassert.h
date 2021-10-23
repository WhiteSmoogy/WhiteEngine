/*! \file cassert.h
\ingroup WBase
\breif ISO C 断言/调试跟踪扩展。
\note 默认输出使用操作系统调试信息输出接口
\todo 基于接口扩展控制台输出，文件输出
*/

#ifndef WBase_cassert_h
#define WBase_cassert_h 1

#include "wdef.h"
#include <cstdio>

namespace platform {
	/*!
	\brief WBase 默认Debug输出函数
	\note 该函数使用操作系统调试信息输出接口
	\note 对于没有的接口，将会定位至wcerr/cerr/stderr
	\note 该函数用于输出"鸡肋"级别的信息[会输出所有信息至此]
	*/
	WB_API void
		wdebug(const char*, ...) wnothrow;

	WB_API void
		wdebug(const wchar_t*, ...) wnothrow;
}


namespace white
{
	/*!
	\brief WBase 默认断言函数。
	\note 当定义宏 WB_Use_WAssert 不等于 0 时，宏 WAssert 操作由此函数实现。
	\note 参数依次为：表达式、文件名、行号和消息文本。
	\note 允许空指针参数，视为未知。
	\note 调用 std::terminate 终止程序。
	*/
	WB_NORETURN WB_API void
		wassert(const char*, const char*, int, const char*) wnothrow;

#if WB_Use_WTrace
	/*!
	\brief WBase 调试跟踪函数。
	\note 当定义宏 WB_Use_YTrace 不等于 0 时，宏 WTrace 操作由此函数实现。
	*/
	WB_API  void
		wtrace(std::FILE*, std::uint8_t, std::uint8_t, const char*, int, const char*,
			...) wnothrow;
#endif

} // namespace white;

#include <cassert>

#undef wconstraint
#undef wassume

  /*!
  \ingroup WBase_pseudo_keyword
  \def wconstraint
  \brief 约束：接口语义。
  \note 和普通断言相比强调接口契约。对移植特定的平台实现时应予以特别注意。
  \note 保证兼容 ISO C++11 constexpr 模板。
  \see $2015-10 @ %Documentation::Workflow::Annual2015.

  运行时检查的接口语义约束断言。不满足此断言的行为是接口明确地未定义的，行为不可预测。
  */
#ifdef NDEBUG
#define wdebug(format,...) (void)0
#else
#define wdebug(format,...) ::platform::wdebug(format,__VA_ARGS__)
#endif

#ifdef NDEBUG
#	define wconstraint(_expr) WB_ASSUME(_expr)
#else
#	define wconstraint(_expr) \
	((_expr) ? void(0) : white::wassert(#_expr, __FILE__, __LINE__, \
		"Constraint violation."))
#endif

  /*!
  \ingroup WBase_pseudo_keyword
  \def wassume
  \brief 假定：环境语义。
  \note 和普通断言相比强调非接口契约。对移植特定的平台实现时应予以特别注意。

  运行时检查的环境条件约束断言。用于明确地非 wconstraint 适用的情形。
  */
#ifdef NDEBUG
#	define wassume(_expr) WB_ASSUME(_expr)
#else
#	define wassume(_expr) assert(_expr)
#endif


#ifndef WAssert
#	if WB_Use_WAssert
#		define WAssert(_expr, _msg) \
	((_expr) ? void(0) : white::wassert(#_expr, __FILE__, __LINE__, _msg))
#	else
#		define WAssert(_expr, _msg) wassume(_expr)
#	endif
#endif
#pragma warning(disable:4800)
#ifndef WAssertNonnull
#	define WAssertNonnull(_expr) WAssert(bool(_expr), "Null reference found.")
#endif

#ifndef WTrace
  //@{
  /*!
  \brief WBase 扩展调试跟踪。
  \note 使用自定义的调试跟踪级别。
  \sa wtrace
  */
#	if WB_Use_WTrace
#		define WTrace(_stream, _lv, _t, _msg, ...) \
	white::wtrace(_stream, _lv, _t, __FILE__, __LINE__, _msg, __VA_ARGS__)
#	else
#		define WTrace(...)
#	endif
#endif
#endif
