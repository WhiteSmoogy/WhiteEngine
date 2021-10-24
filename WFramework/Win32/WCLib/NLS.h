/*!	\file NLS.h
\ingroup Framework
\ingroup Win32
\brief Win32 ƽ̨��Ȼ���Դ���֧����չ�ӿڡ�
*/

#ifndef FrameWork_Win32_NLS_h
#define FrameWork_Win32_NLS_h 1

#include <WFramework/Win32/WCLib/Mingw32.h>

namespace platform_ex {

	namespace Windows {
		
		/*!	\defgroup native_encoding_conv Native Encoding Conversion
		\brief �����ı�����ת����
		\exception white::LoggedEvent ����Ϊ��������� int ��

		ת����һ�� \c unsigned ����ָ��������ַ���Ϊ�ڶ��� \c unsigned ����ָ���ı��롣
		*/
		//@{
		//! \pre ��Ӷ��ԣ��ַ���ָ������ǿա�
		WB_API WB_NONNULL(1) string
			MBCSToMBCS(const char*, unsigned = CP_UTF8, unsigned = CP_ACP);
		//! \pre ���Ȳ��������Ҳ����� \c int ʱ��Ӷ��ԣ��ַ���ָ������ǿա�
		WB_API string
			MBCSToMBCS(string_view, unsigned = CP_UTF8, unsigned = CP_ACP);

		//! \pre ��Ӷ��ԣ��ַ���ָ������ǿա�
		WB_API WB_NONNULL(1) wstring
			MBCSToWCS(const char*, unsigned = CP_ACP);
		//! \pre ���Ȳ��������Ҳ����� \c int ʱ��Ӷ��ԣ��ַ���ָ������ǿա�
		WB_API wstring
			MBCSToWCS(string_view, unsigned = CP_ACP);

		//! \pre ��Ӷ��ԣ��ַ���ָ������ǿա�
		WB_API WB_NONNULL(1) string
			WCSToMBCS(const wchar_t*, unsigned = CP_ACP);
		//! \pre ���Ȳ��������Ҳ����� \c int ʱ��Ӷ��ԣ��ַ���ָ������ǿա�
		WB_API string
			WCSToMBCS(wstring_view, unsigned = CP_ACP);

		//! \pre ��Ӷ��ԣ��ַ���ָ������ǿա�
		inline WB_NONNULL(1) PDefH(wstring, UTF8ToWCS, const char* str)
			ImplRet(MBCSToWCS(str, CP_UTF8))
			//! \pre ���Ȳ��������Ҳ����� \c int ʱ��Ӷ��ԣ��ַ���ָ������ǿա�
			inline PDefH(wstring, UTF8ToWCS, string_view sv)
			ImplRet(MBCSToWCS(sv, CP_UTF8))

			//! \pre ��Ӷ��ԣ��ַ���ָ������ǿա�
			inline WB_NONNULL(1) PDefH(string, WCSToUTF8, const wchar_t* str)
			ImplRet(WCSToMBCS(str, CP_UTF8))
			//! \pre ���Ȳ��������Ҳ����� \c int ʱ��Ӷ��ԣ��ַ���ָ������ǿա�
			inline PDefH(string, WCSToUTF8, wstring_view sv)
			ImplRet(WCSToMBCS(sv, CP_UTF8))
	}
}

#endif
