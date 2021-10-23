#ifndef WBase_cwctype_h
#define WBase_cwctype_h

#include "cctype.h"
#include <cwctype>

namespace stdex
{
	//@{
	/*!
	\brief �����޹ص� std::isprint ʵ�֡�
	\note ��ǰʹ�ü��� Unicode �ļ�ʵ�֣��ο� MUSL libc ����
	\note ����Ϊ��� MSVCRT ��ʵ�ֵı�ͨ�����Ա��� iswprintf �� isprint ������ȱ�ݣ���
	\sa https://connect.microsoft.com/VisualStudio/feedback/details/799287/isprint-incorrectly-classifies-t-as-printable-in-c-locale
	*/
	WB_API bool
		iswprint(wchar_t);

	/*!
	\brief �����޹ص� std::iswspace ʵ�֡�
	\note ��ǰʹ�� Unicode 5.2 ʵ�֣��ο� newlib ����
	\note ����Ϊ��� Android 2.3.1 ��ʵ�ֵı�ͨ�����Ա��� iswspace �Ը����ַ����ܷ��� 8 ����
	*/
	WB_API bool
		iswspace(wchar_t);

	/*!
	\brief �����޹ص� std::iswgraph ʵ�֡�
	\see ISO C11 7.30.2.1.6 ��
	*/
	inline bool
		iswgraph(wchar_t wc)
	{
		return !iswspace(wc) && iswprint(wc);
	}
}

#endif
