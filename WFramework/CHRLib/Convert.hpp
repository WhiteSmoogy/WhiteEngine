/*!	\file Convert.hpp
\ingroup CHRLib
\brief ת��ģ�塣
\version r113
\author FrankHB <frankhb1989@gmail.com>
\since build 400
\par ����ʱ��:
2015-12-29 10:18:20 +0800
\par �޸�ʱ��:
2016-06-19 leohawke port to vs2015
\par �ı�����:
UTF-8
\par ģ������:
CHRLib::Convert
*/

#ifndef LBase_CHRLib_Convert_hpp
#define LBase_CHRLib_Convert_hpp 1

#include "WBase/any_iterator.hpp"
#include "WFramework/CHRLib/CharacterMapping.h"

namespace CHRLib
{

	template<typename _tIn, typename _tOut, typename _fConv>
	ConversionResult
		ConvertCharacter(_fConv f, _tOut&& obj, _tIn&& i, ConversionState&& st)
	{
		return f(wforward(obj),white::input_monomorphic_iterator(std::ref(i)),
			std::move(st));
	}
	template<typename _tIn, typename _fConv>
	ConversionResult
		ConvertCharacter(_fConv f, _tIn&& i, ConversionState&& st)
	{
		return f(white::input_monomorphic_iterator(std::ref(i)), std::move(st));
	}

	using white::remove_reference_t;
	using white::decay_t;

	/*!
	\brief ��ָ֤���ַ������Ƿ��ת��Ϊ UCS-2 �ַ�����
	\pre ָ����Χ�ڵĵ������ɽ����á�
	\note ʹ�� ADL \c MBCToUC ָ��ת�������������̣���һ����Ӧ���������������һλ�á�
	*/
	//@{
	template<typename _tIn>
	bool
		VerifyUC(_tIn&& first, remove_reference_t<_tIn> last, Encoding enc)
	{
		auto res(ConversionResult::OK);

		while (first != last && *first != 0
			&& (res = MBCToUC(first, enc), res == ConversionResult::OK))
			;
		return res == ConversionResult::OK || *first == char();
	}
	/*!
	\pre ������Ϊ�����������
	\note ʹ��ָ���������߽硣
	*/
	template<typename _tIn>
	bool
		VerifyUC(_tIn&& first, typename std::iterator_traits<decay_t<_tIn>>
			::difference_type n, Encoding enc)
	{

		if (n != 0)
		{
			auto res(ConversionResult::OK);
			const auto i(first);

			while (*first != 0 && (res = MBCToUC(first, first + n, enc),
				res == ConversionResult::OK))
				;
			return res == ConversionResult::OK
				|| (res == ConversionResult::BadSource && first - i == n)
				|| *first == char();
		}
		return true;
	}
	//@}

} // namespace CHRLib;

#endif
