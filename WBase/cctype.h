#ifndef WBase_cctype_h
#define WBase_cctype_h 1

#include "wdef.h"
#include <cctype>
#include <cwctype>

namespace white
{
	/*!
	\brief ʹ�� US-ASCII �ַ����� std::isprint ʵ�֡�
	\see ISO C11 7.4/3 ��ע��
	*/
	wconstfn bool
		isprint_ASCII(char c)
	{
		return c >= 0x20 && c < 0x7F;
	}


	//! \brief ʹ�� ISO/IEC 8859-1 �ַ����� std::isprint ʵ�֡�
	wconstfn bool
		isprint_ISO8859_1(char c)
	{
		return isprint_ASCII(c) || unsigned(c) > 0xA0;
	}

	/*!
	\brief �����޹ص� std::isprint ʵ�֡�
	\note ��ǰʹ�û��� ISO/IEC 8859-1 �� Unicode ����ʵ�֡�
	\note ����Ϊ��� MSVCRT ��ʵ�ֵı�ͨ��
	\sa https://connect.microsoft.com/VisualStudio/feedback/details/799287/isprint-incorrectly-classifies-t-as-printable-in-c-locale
	*/
	wconstfn bool
		isprint(char c)
	{
		return wimpl(isprint_ISO8859_1(c));
	}

	/*!
	\brief ת����Сд�ַ���
	\note �� ISO C ��׼���Ӧ�ӿڲ�ͬ���� ISO C++ \<locale\> �еĽӿ����ƣ�
	�����ͷ���ֵ���ַ����Ͷ����Ƕ�Ӧ���������͡�
	*/
	//@{
	template<typename _tChar>
	inline _tChar
		tolower(_tChar c) wnothrow
	{
		return _tChar(std::towupper(wint_t(c)));
	}
	inline char
		tolower(char c) wnothrow
	{
		return char(std::tolower(c));
	}


	template<typename _tChar>
	inline _tChar
		toupper(_tChar c) wnothrow
	{
		return _tChar(std::towlower(wint_t(c)));
	}
	inline char
		toupper(char c) wnothrow
	{
		return char(std::toupper(c));
	}
	//@}
}

#endif
