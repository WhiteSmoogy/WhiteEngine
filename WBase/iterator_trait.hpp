/*!	\file iterator_trait.hpp
\ingroup WBase
\brief ������������
*/

#ifndef WBase_iterator_trait_hpp
#define WBase_iterator_trait_hpp 1


#include "type_traits.hpp" // for std::pair, std::declval, enable_if_t,
//	are_same;
#include <iterator> // for std::iterator_traits;

namespace white
{

	/*!
	\ingroup type_traits_operations
	\brief �ж����ɸ��������Ƿ�ָ�������
	\since build 1.4
	*/
	template<class _type, typename... _tIter>
	struct have_same_iterator_category : are_same<_type,
		typename std::iterator_traits<_tIter>::iterator_category...>
	{};

	/*!
	\ingroup metafunctions
	\brief ѡ����������͵��ض����ر�����������ͳ�ͻ��
	\sa enable_if_t
	\since build 1.4
	*/
	template<typename _tParam, typename _type = void, typename = wimpl(std::pair<
		indirect_t<_tParam&>, decltype(++std::declval<_tParam&>())>)>
		using enable_for_iterator_t = enable_if_t<
		is_same<decltype(++std::declval<_tParam&>()), _tParam&>::value, _type>;

} // namespace white;

#endif