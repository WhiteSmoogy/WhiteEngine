#ifndef WScheme_SDEF_H
#define WScheme_SDEF_H 1

#include <WBase/wdef.h>

#if defined(WS_DLL) && defined(WS_BUILD_DLL)
#	error "DLL could not be built and used at the same time."
#endif

#if WB_IMPL_MSCPP \
	|| (WB_IMPL_GNUCPP && (defined(__MINGW32__) || defined(__CYGWIN__)))
#	ifdef WS_DLL
#		define WS_API __declspec(dllimport)
#	elif defined(WS_BUILD_DLL)
#		define WS_API __declspec(dllexport)
#	else
#		define WS_API
#	endif
#elif defined(WS_BUILD_DLL) && (WB_IMPL_GNUCPP >= 40000 || WB_IMPL_CLANGPP)
#	define WS_API WB_ATTR(__visibility__("default"))
#else
#	define WS_API
#endif

#define YSLIB_COMMIT 9913de30226e25f06a47b5b418330c6567e382e5
#define YSLIB_BUILD 777
#define YSLIB_REV 11

#endif
