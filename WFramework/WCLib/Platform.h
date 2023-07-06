/*!	\file Platform.h
\ingroup Framework
\brief 通用平台描述文件。
*/

#ifndef FrameWork_Platform_h
#define FrameWork_Platform_h 1

#include <WBase/wdef.h> // for std::uintmax_t,, stdex::octet,
//  ::ptrdiff_t, ::size_t, ::wint_t stdex::nullptr_t;
#include <WBase/winttype.hpp> // for white::byte

/*!
\def WF_DLL
\brief 使用 WFramework 动态链接库。
*/

#if defined(WF_DLL) && defined(WF_BUILD_DLL)
#	error "DLL could not be built and used at the same time."
#endif

#if WB_IMPL_MSCPP \
	|| (WB_IMPL_GNUCPP && (defined(__MINGW32__) || defined(__CYGWIN__)))
#	ifdef WF_DLL
#		define WF_API __declspec(dllimport)
#	elif defined(WF_BUILD_DLL)
#		define WF_API __declspec(dllexport)
#	else
#		define WF_API
#	endif
#elif defined(WF_BUILD_DLL) && (WB_IMPL_GNUCPP >= 40000 || WB_IMPL_CLANGPP)
#	define WF_API WF_ATTR(__visibility__("default"))
#else
#	define WF_API
#endif


//@{
/*!
\brief Win32 平台。
\note 注意 _WIN32 被预定义，但没有 _WIN64 。
*/
#define WF_Platform_Win32 0x6100

/*!
\brief Win32 x86_64 平台。
\note 通称 Win64 平台。注意 Win32 指 Windows32 子系统，并非体系结构名称。
\note 注意 _WIN32 和 _WIN64 被同时预定义。
*/
#define WF_Platform_Win64 0x6110

/*!
\brief MinGW32 平台。
\note 注意 MinGW-w64 和 MinGW.org 同时预定义了 __MINGW32__ ，但没有 __MINGW64__ 。
*/
#define WF_Platform_MinGW32 0x6102

/*!
\brief MinGW-w64 x86_64 平台。
\note 和 Win64 基本兼容的 MinGW32 平台。注意 MinGW32 指系统名称，并非体系结构名称。
\note 注意 MinGW-w64 在 x86_64 上同时预定义了 __MINGW32__ 和 __MINGW64__ 。
*/
#define WF_Platform_MinGW64 0x6112


/*!
\def WF_Platform
\brief 目标平台。
\note 注意顺序。
*/
#ifdef __MSYS__
#	error "MSYS is not currently supported. Use MinGW targets instead."
#elif defined(__MINGW64__)
#	define WF_Platform WF_Platform_MinGW64
#elif defined(__MINGW32__)
#	define WF_Platform WF_Platform_MinGW32
#elif defined(_WIN64)
#	define WF_Platform WF_Platform_Win64
#elif defined(_WIN32)
#	define WF_Platform WF_Platform_Win32
#elif defined(__ANDROID__)
// FIXME: Architecture detection.
#	define WF_Platform WF_Platform_Android_ARM
#elif defined(__linux__)
#	ifdef __i386__
#		define WF_Platform WF_Platform_Linux_x64
#	elif defined(__x86_64__)
#		define WF_Platform WF_Platform_Linux_x86
#	endif
#elif defined(__APPLE__)
#	ifdef __MACH__
#		define WF_Platform WF_Platform_OS_X
#	else
#		error "Apple platforms other than OS X is not supported."
#	endif
#elif !defined(WF_Platform)
//当前默认以 Win32 作为目标平台。
#	define WF_Platform WF_Platform_Win32
#endif
//@}

#if WF_Platform == WF_Platform_Win32
#	define WFL_Win32 1
#	define WF_Hosted 1
#elif WF_Platform == WF_Platform_MinGW32
#	define WFL_MinGW 1
#	define WFL_Win32 1
#	define WF_Hosted 1
#elif WF_Platform == WF_Platform_Win64
#	define WFL_Win32 1
#	define WFL_Win64 1
#	define WF_Hosted 1
#	ifndef LB_Use_POSIXThread
#		define LB_Use_POSIXThread 1
#	endif
#elif WF_Platform == WF_Platform_MinGW64
#	define WFL_MinGW 1
#	define WFL_Win32 1
#	define WFL_Win64 1
#	define WF_Hosted 1
#	ifndef LB_Use_POSIXThread
#		define LB_Use_POSIXThread 1
#	endif
#endif

#ifndef WFL_HostedUI_XCB
#	define WFL_HostedUI_XCB 0
#endif
#ifndef WFL_HostedUI
#	if WFL_HostedUI_XCB || WFL_Win32 || WFL_Android
#		define WFL_HostedUI 1
#	else
#		define WFL_HostedUI 0
#	endif
#endif

#if WFL_HostedUI_XCB && !defined(WF_Use_XCB)
#	define WF_Use_XCB 0x11100
#endif


#if __STDCPP_THREADS__
#	define WF_Multithread 1
#elif WFL_Win32 || WFL_Android || WFL_Linux || WFL_OS_X
#	define WF_Multithread 1
#else
#	define WF_Multithread 0
#endif

/*!
\brief X86 架构。
*/
#define WF_Arch_X86 0x70086

/*!
\brief AMD64 架构。
*/
#define WF_Arch_X64 0x76486

/*!
\brief ARM 架构。
*/
#define WF_Arch_ARM 0x70000

#ifdef WB_IMPL_MSCPP
//Visual C++
#if defined(_M_IX86)
#define WF_Arch WF_Arch_X86
#elif defined(_M_AMD64)
#define WF_Arch WF_Arch_X64
#elif defined(_M_ARM)
#define WF_Arch WF_Arch_ARM
#else
#error "unsupprot arch"
#endif
#else
//g++ clang++
#if defined(_X86_)
#define WF_Arch WF_Arch_X86
#elif defined(__amd64)
#define WF_Arch WF_Arch_X64
#endif
#endif

#if WF_Arch != WF_Arch_ARM
#define WF_SSE 1
#else
#define WF_NEON 1
#endif


namespace platform
{
	//! \since build 1.4
	inline namespace basic_types
	{
		/*!
		\brief 平台通用数据类型。
		*/
		//@{
		using white::byte;
		using stdex::octet;
		using stdex::ptrdiff_t;
		using stdex::size_t;
		using stdex::wint_t;
		using stdex::nullptr_t;
		//@}

	} // inline namespace basic_types;

} // namespace platform;

namespace platform_ex {
	using namespace platform::basic_types;
}

#endif


