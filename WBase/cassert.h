/*! \file cassert.h
\ingroup WBase
\breif ISO C ����/���Ը�����չ��
\note Ĭ�����ʹ�ò���ϵͳ������Ϣ����ӿ�
\todo ���ڽӿ���չ����̨������ļ����
*/

#ifndef WBase_cassert_h
#define WBase_cassert_h 1

#include "wdef.h"
#include <cstdio>

namespace platform {
	/*!
	\brief WBase Ĭ��Debug�������
	\note �ú���ʹ�ò���ϵͳ������Ϣ����ӿ�
	\note ����û�еĽӿڣ����ᶨλ��wcerr/cerr/stderr
	\note �ú����������"����"�������Ϣ[�����������Ϣ����]
	*/
	WB_API void
		wdebug(const char*, ...) wnothrow;

	WB_API void
		wdebug(const wchar_t*, ...) wnothrow;
}


namespace white
{
	/*!
	\brief WBase Ĭ�϶��Ժ�����
	\note ������� WB_Use_WAssert ������ 0 ʱ���� WAssert �����ɴ˺���ʵ�֡�
	\note ��������Ϊ�����ʽ���ļ������кź���Ϣ�ı���
	\note �����ָ���������Ϊδ֪��
	\note ���� std::terminate ��ֹ����
	*/
	WB_NORETURN WB_API void
		wassert(const char*, const char*, int, const char*) wnothrow;

#if WB_Use_WTrace
	/*!
	\brief WBase ���Ը��ٺ�����
	\note ������� WB_Use_YTrace ������ 0 ʱ���� WTrace �����ɴ˺���ʵ�֡�
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
  \brief Լ�����ӿ����塣
  \note ����ͨ�������ǿ���ӿ���Լ������ֲ�ض���ƽ̨ʵ��ʱӦ�����ر�ע�⡣
  \note ��֤���� ISO C++11 constexpr ģ�塣
  \see $2015-10 @ %Documentation::Workflow::Annual2015.

  ����ʱ���Ľӿ�����Լ�����ԡ�������˶��Ե���Ϊ�ǽӿ���ȷ��δ����ģ���Ϊ����Ԥ�⡣
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
  \brief �ٶ����������塣
  \note ����ͨ�������ǿ���ǽӿ���Լ������ֲ�ض���ƽ̨ʵ��ʱӦ�����ر�ע�⡣

  ����ʱ���Ļ�������Լ�����ԡ�������ȷ�ط� wconstraint ���õ����Ρ�
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
  \brief WBase ��չ���Ը��١�
  \note ʹ���Զ���ĵ��Ը��ټ���
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
