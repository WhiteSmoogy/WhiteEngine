/*!	\file NLS.h
\ingroup Framework
\ingroup Win32
\brief Win32 平台自然语言处理支持扩展接口。
*/

#ifndef FrameWork_Win32_NLS_h
#define FrameWork_Win32_NLS_h 1

#include <WFramework/Win32/WCLib/Mingw32.h>

namespace platform_ex {

	namespace Windows {
		
		/*!	\defgroup native_encoding_conv Native Encoding Conversion
		\brief 本机文本编码转换。
		\exception white::LoggedEvent 长度为负数或溢出 int 。

		转换第一个 \c unsigned 参数指定编码的字符串为第二个 \c unsigned 参数指定的编码。
		*/
		//@{
		//! \pre 间接断言：字符串指针参数非空。
		WB_API WB_NONNULL(1) string
			MBCSToMBCS(const char*, unsigned = CP_UTF8, unsigned = CP_ACP);
		//! \pre 长度参数非零且不上溢 \c int 时间接断言：字符串指针参数非空。
		WB_API string
			MBCSToMBCS(string_view, unsigned = CP_UTF8, unsigned = CP_ACP);

		//! \pre 间接断言：字符串指针参数非空。
		WB_API WB_NONNULL(1) wstring
			MBCSToWCS(const char*, unsigned = CP_ACP);
		//! \pre 长度参数非零且不上溢 \c int 时间接断言：字符串指针参数非空。
		WB_API wstring
			MBCSToWCS(string_view, unsigned = CP_ACP);

		//! \pre 间接断言：字符串指针参数非空。
		WB_API WB_NONNULL(1) string
			WCSToMBCS(const wchar_t*, unsigned = CP_ACP);
		//! \pre 长度参数非零且不上溢 \c int 时间接断言：字符串指针参数非空。
		WB_API string
			WCSToMBCS(wstring_view, unsigned = CP_ACP);

		//! \pre 间接断言：字符串指针参数非空。
		inline WB_NONNULL(1) PDefH(wstring, UTF8ToWCS, const char* str)
			ImplRet(MBCSToWCS(str, CP_UTF8))
			//! \pre 长度参数非零且不上溢 \c int 时间接断言：字符串指针参数非空。
			inline PDefH(wstring, UTF8ToWCS, string_view sv)
			ImplRet(MBCSToWCS(sv, CP_UTF8))

			//! \pre 间接断言：字符串指针参数非空。
			inline WB_NONNULL(1) PDefH(string, WCSToUTF8, const wchar_t* str)
			ImplRet(WCSToMBCS(str, CP_UTF8))
			//! \pre 长度参数非零且不上溢 \c int 时间接断言：字符串指针参数非空。
			inline PDefH(string, WCSToUTF8, wstring_view sv)
			ImplRet(WCSToMBCS(sv, CP_UTF8))
	}
}

#endif
