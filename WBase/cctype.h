#ifndef WBase_cctype_h
#define WBase_cctype_h 1

#include "wdef.h"
#include <cctype>
#include <cwctype>

namespace white
{
	/*!
	\brief 使用 US-ASCII 字符集的 std::isprint 实现。
	\see ISO C11 7.4/3 脚注。
	*/
	wconstfn bool
		isprint_ASCII(char c)
	{
		return c >= 0x20 && c < 0x7F;
	}


	//! \brief 使用 ISO/IEC 8859-1 字符集的 std::isprint 实现。
	wconstfn bool
		isprint_ISO8859_1(char c)
	{
		return isprint_ASCII(c) || unsigned(c) > 0xA0;
	}

	/*!
	\brief 区域无关的 std::isprint 实现。
	\note 当前使用基于 ISO/IEC 8859-1 的 Unicode 方案实现。
	\note 可作为替代 MSVCRT 的实现的变通。
	\sa https://connect.microsoft.com/VisualStudio/feedback/details/799287/isprint-incorrectly-classifies-t-as-printable-in-c-locale
	*/
	wconstfn bool
		isprint(char c)
	{
		return wimpl(isprint_ISO8859_1(c));
	}

	/*!
	\brief 转换大小写字符。
	\note 和 ISO C 标准库对应接口不同而和 ISO C++ \<locale\> 中的接口类似，
	参数和返回值是字符类型而不是对应的整数类型。
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
