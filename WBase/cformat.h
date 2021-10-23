#pragma once

#include "cassert.h" // for wconstraint
#include <cstdarg> // for std::va_list;
#include <stdexcept>

namespace white
{
	/*!
	\brief ����ָ����ʽ�ַ��������ռ�õ��ֽ�����
	\pre ���ԣ���һ�����ǿա�
	\return �ɹ�Ϊ�ֽ���������Ϊ size_t(-1) ��
	\note �ֱ�ʹ�� std::vsnprintf �� std::vswprintf ʵ�֡�
	*/
	//@{
	WB_API WB_NONNULL(1) size_t
		vfmtlen(const char*, std::va_list) wnothrow;
	WB_API WB_NONNULL(1) size_t
		vfmtlen(const wchar_t*, std::va_list) wnothrow;
	//@}


	/*!
	\pre ��Ӷ��ԣ���һ�����ǿա�
	\since build 1.4
	\bug \c char �����ģ���������ȷʵ�֡�
	*/
	//@{
	/*!
	\brief �� C ��׼�����ʽ����� std::basic_string ʵ���Ķ���
	\throw std::runtime_error ��ʽ���ַ������ʧ�ܡ�
	\note �� _tString �����쳣������
	*/
	template<typename _tChar, class _tString = std::basic_string<_tChar>>
	WB_NONNULL(1) _tString
		vsfmt(const _tChar* fmt, std::va_list args)
	{
		std::va_list ap;

		va_copy(ap, args);

		const auto l(white::vfmtlen(fmt, ap));

		va_end(ap);
		if (l == size_t(-1))
			throw std::runtime_error("Failed to write formatted string.");

		_tString str(l, _tChar());

		if (l != 0)
		{
			wassume(str.length() > 0 && str[0] == _tChar());
			if constexpr (std::is_same_v<wchar_t, _tChar>)
				std::vswprintf(&str[0], l + 1, fmt, args);
			else
				std::vsprintf(&str[0], fmt, args);
		}
		return str;
	}

	/*!
	\brief �� C ��׼�����ʽ����� std::basic_string ʵ���Ķ���
	\note ʹ�� ADL ���ʿɱ������
	\note Clang++ ��ģ������ attribute ֱ����ʾ��ʽ�ַ������ʹ���
	*/
	template<typename _tChar>
	WB_NONNULL(1) auto
		sfmt(const _tChar* fmt, ...)
		-> decltype(vsfmt(fmt, std::declval<std::va_list>()))
	{
		std::va_list args;

		va_start(args, fmt);
		try
		{
			auto str(vsfmt(fmt, args));

			va_end(args);
			return str;
		}
		catch (...)
		{
			va_end(args);
			throw;
		}
	}
	//@}

#if WB_IMPL_MSCPP
#else
	template WB_ATTR(format(printf, 1, 2)) std::string
		sfmt<char>(const char*, ...);
#endif
}