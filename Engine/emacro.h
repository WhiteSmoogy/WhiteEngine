/*! \file Engine\emacro.h
\ingroup Engine
\brief Engine Common Macro
*/
#ifndef WE_emacro_H
#define WE_emacro_H 1


#include <WBase/cformat.h>
#include <WFramework/WCLib/Debug.h>

/*!
\def WE_DLL
\brief ʹ�� Engine ��̬���ӿ⡣
\since build 1.02
*/
/*!
\def WE_BUILD_DLL
\brief ���� Engine ��̬���ӿ⡣
\since build 1.02
*/
/*!
\def WE_API
\brief LBase Ӧ�ó����̽ӿڣ���������ļ�Լ�����ӡ�
\since build 1.02
\todo �ж���������֧�֡�
*/
#if defined(WE_DLL) && defined(WE_BUILD_DLL)
#	error "DLL could not be built and used at the same time."
#endif

#if WB_IMPL_MSCPP \
	|| (WB_IMPL_GNUCPP && (defined(__MINGW32__) || defined(__CYGWIN__)))
#	ifdef LB_DLL
#		define WE_API __declspec(dllimport)
#	elif defined(LB_BUILD_DLL)
#		define WE_API __declspec(dllexport)
#	else
#		define WE_API
#	endif
#elif defined(LB_BUILD_DLL) && (WB_IMPL_GNUCPP >= 40000 || WB_IMPL_CLANGPP)
#	define WE_API LB_ATTR(__visibility__("default"))
#else
#	define WE_API
#endif

#ifndef WE_LogError
#define WE_LogError(...) platform_ex::SendDebugString(platform::Descriptions::Err,white::sfmt(__VA_ARGS__).c_str())
#endif

#ifndef WE_LogWarning
#define WE_LogWarning(...) platform_ex::SendDebugString(platform::Descriptions::Warning,white::sfmt(__VA_ARGS__).c_str())
#endif

#define WE_EDITOR 1

#endif
