/*!	\file optional.hpp
\ingroup WBase
\brief ��ѡֵ��װ���͡�
\see http://www.boost.org/doc/libs/1_57_0/libs/optional/doc/html/optional/reference.html ��


*/

#ifndef WBase_Optional_hpp
#define WBase_Optional_hpp 1

#include "functional.hpp"
#ifdef __cpp_lib_experimental_optional
#include <experimental/optional>
#else
#include <optional>
#endif

namespace white {
#ifdef __cpp_lib_experimental_optional
	using std::experimental::optional;
	using std::experimental::nullopt_t;
	using std::experimental::nullopt;
	using std::experimental::bad_optional_access;
#elif __cpp_lib_optional
	using std::optional;
	using std::nullopt_t;
	using std::nullopt;
	using std::bad_optional_access;
#else
#error "__cpp_lib_experimental_optional || __cpp_lib_optional"
#endif

	/*!
	\brief �ϲ���ѡֵ���С�
	\note ����ͬ Boost.Signal2 �� \c boost::optional_last_value ��
	*/
	//@{
	template<typename _type>
	struct optional_last_value
	{
		template<typename _tIn>
		optional<_type>
			operator()(_tIn first, _tIn last) const
		{
			return std::accumulate(first, last, optional<_type>(),
				[](optional<_type>&, decltype(*first) val) {
				return wforward(val);
			});
		}
	};

	template<>
	struct optional_last_value<void> : default_last_value<void>
	{};
	//@}
}

#endif
