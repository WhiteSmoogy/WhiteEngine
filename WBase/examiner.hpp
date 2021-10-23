/*! \file examiner.hpp
\ingroup WBase
\brief C++类型操作检测。
*/

#ifndef WBase_examiner_hpp
#define WBase_examiner_hpp 1

#include "wdef.h"
#include <type_traits>

namespace white {
	namespace examiners
	{
		template<typename _type, typename _type2 = _type, class = std::void_t<>>
		struct has_equality_operator : std::false_type
		{};

		template<typename _type, typename _type2>
		struct has_equality_operator<_type, _type2,
			std::void_t<decltype(std::declval<_type>() == std::declval<_type2>())>> : std::true_type
		{};

		/*!
		\brief 基本等于操作检测。
		*/
		struct equal
		{
			template<typename _type1, typename _type2>
			static wconstfn WB_PURE auto
				are_equal(_type1&& x, _type2&& y)
				wnoexcept_spec(bool(x == y)) -> decltype(bool(x == y))
			{
				return bool(x == y);
			}
		};


		/*!
		\brief 基本等于操作检测：总是相等。
		*/
		struct always_equal
		{
			template<typename _type, typename _type2, wimpl(typename
				= std::enable_if_t<!has_equality_operator<_type&&, _type2&&>::value>)>
				static wconstfn WB_STATELESS bool
				are_equal(_type&&, _type2&&) wnothrow
			{
				return true;
			}
		};


		/*!
		\brief 等于操作检测。
		*/
		struct equal_examiner : public equal, public always_equal
		{
			using equal::are_equal;
			using always_equal::are_equal;
		};

	} // namespace examiners;
}


#endif