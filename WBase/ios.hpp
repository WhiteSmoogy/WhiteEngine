/*!	\file ios.hpp
\ingroup WBase
\brief ISO C++ ��׼������/�����������չ��
*/

#ifndef WBase_ios_hpp
#define WBase_ios_hpp 1


#include "wdef.h"
#include <ios> // for std::basic_ios, std::ios_base::iostate;

namespace white
{
	/*!
	\brief ������״̬��
	\note ���쳣�׳�����ֱ�ӵ��� std::basic_ios �� setstate ��Ա������ͬ��
	\note �����ñ�׼��ʵ�ֵ��ڲ��ӿڡ�
	*/
	template<typename _tChar, class _tTraits>
	void
		setstate(std::basic_ios<_tChar, _tTraits>& ios, std::ios_base::iostate state) wnothrow
	{
		const auto except(ios.exceptions());

		ios.exceptions(std::ios_base::goodbit);
		ios.setstate(state);
		ios.exceptions(except);
	}

	/*!
	\brief ������״̬�������׳���ǰ�쳣��
	\note һ����Ҫ����Ϊʵ�ֱ�׼��Ҫ��ĸ�ʽ/�Ǹ�ʽ����/���������
	\see WG21/N4567 27.7.2.2.1[istream.formatted.reqmts] ��
	\see WG21/N4567 27.7.2.3[istream.unformatted]/1 ��
	\see WG21/N4567 27.7.3.6.1[ostream.formatted.reqmts] ��
	\see WG21/N4567 27.7.3.7[ostream.unformatted]/1 ��
	\see http://wg21.cmeerw.net/lwg/issue91 ��
	*/
	template<typename _tChar, class _tTraits>
	WB_NORETURN
	void
		rethrow_badstate(std::basic_ios<_tChar, _tTraits>& ios,
			std::ios_base::iostate state = std::ios_base::badbit)
	{
		setstate(ios, state);
		throw;
	}

} // namespace white;

#endif
