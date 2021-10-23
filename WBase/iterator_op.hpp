/*!	\file iterator_op.hpp
\ingroup WBase
\brief ������������
*/

#ifndef WBase_iterator_op_hpp
#define WBase_iterator_op_hpp 1

#include "range.hpp" // for std::next, std::prev, std::make_move_iterator,
//	std::reverse_iterator, std::make_pair, begin, end;
#include "cassert.h" // for wconstraint;
#include "deref_op.hpp" // for is_undereferenceable;
#include "operators.hpp" // for input_iteratable, output_iteratable,
//	forward_iteratable, bidirectional_iteratable, random_access_iteratable;

namespace white
{
	/*!	\defgroup iterator_operations Iterator Operations
	\ingroup iterators
	\brief ������������
	\since build 1.4
	*/
	//@{
	/*!
	\brief ������ָ���ֵ��������ʱȡ�ڽӵ�����������ȡԭֵ��
	\param i ָ���ĵ�������
	\pre �������ɽ����ã��̺����ԣ�<tt>!is_undereferenceable(i)</tt> ��
	*/
	//@{
	template<typename _tIn, typename _fPred>
	_tIn
		next_if(_tIn i, _fPred f,
			typename std::iterator_traits<_tIn>::difference_type n = 1)
	{
		wconstraint(!is_undereferenceable(i));
		return f(*i) ? std::next(i, n) : i;
	}
	template<typename _tIn, typename _type>
	_tIn
		next_if_eq(_tIn i, const _type& val,
			typename std::iterator_traits<_tIn>::difference_type n = 1)
	{
		wconstraint(!is_undereferenceable(i));
		return *i == val ? std::next(i, n) : i;
	}

	/*!
	\brief ������ָ���ֵ��������ʱȡ�����ڽӵ�����������ȡԭֵ��
	\pre �������ɽ����á�
	*/
	//@{
	template<typename _tBi, typename _fPred>
	_tBi
		prev_if(_tBi i, _fPred f,
			typename std::iterator_traits<_tBi>::difference_type n = 1)
	{
		return f(*i) ? std::prev(i, n) : i;
	}
	template<typename _tBi, typename _type>
	_tBi
		prev_if_eq(_tBi i, const _type& val,
			typename std::iterator_traits<_tBi>::difference_type n = 1)
	{
		return *i == val ? std::prev(i, n) : i;
	}
	//@}
	//@}

	//! \ingroup helper_functions
	//@{
	/*!
	\brief ���췴���������
	\see WG21 N3936 24.5.1.3.21[reverse.iter.make] ��
	\see http://wg21.cmeerw.net/lwg/issue2285 ��
	\since build 1.4
	*/
#if __cpp_lib_make_reverse_iterator >= 201402 || __cplusplus >= 201402L
	using std::make_reverse_iterator;
#else
	template<typename _tIter>
	std::reverse_iterator<_tIter>
		make_reverse_iterator(_tIter i)
	{
		return std::reverse_iterator<_tIter>(i);
	}
#endif

	/*!
	\brief ����ת�Ƶ������ԡ�
	*/
	template<typename _tIterator1, typename _tIterator2>
	inline auto
		make_move_iterator_pair(_tIterator1 it1, _tIterator2 it2) -> decltype(
			std::make_pair(std::make_move_iterator(it1), std::make_move_iterator(it2)))
	{
		return std::make_pair(std::make_move_iterator(it1),
			std::make_move_iterator(it2));
	}

	/*!
	\brief ����ָ�����з�Χ�����������������ڽ�����ȣ���ת�Ƶ������ԡ�
	\note ʹ�� ADL \c begin �� \c end ָ����Χ��������
	\bug decltype ָ���ķ������Ͳ���ʹ�� ADL ��
	\since build 1.4
	*/
	template<typename _tRange>
	inline auto
		make_move_iterator_pair(_tRange& c)
		-> decltype(white::make_move_iterator_pair(begin(c), end(c)))
	{
		return white::make_move_iterator_pair(begin(c), end(c));
	}

	namespace details
	{

		template<typename _tIter, typename _tTraits,
			typename _tTag = typename _tTraits::iterator_category>
		struct iterator_operators;

		template<typename _tIter, typename _tTraits>
		struct iterator_operators<_tIter, _tTraits, std::input_iterator_tag>
		{
			using type = input_iteratable<_tIter, typename _tTraits::reference>;
		};

		template<typename _tIter, typename _tTraits>
		struct iterator_operators<_tIter, _tTraits, std::output_iterator_tag>
		{
			using type = output_iteratable<_tIter, typename _tTraits::reference>;
		};

		template<typename _tIter, typename _tTraits>
		struct iterator_operators<_tIter, _tTraits, std::forward_iterator_tag>
		{
			using type = forward_iteratable<_tIter, typename _tTraits::reference>;
		};

		template<typename _tIter, typename _tTraits>
		struct iterator_operators<_tIter, _tTraits, std::bidirectional_iterator_tag>
		{
			using type = bidirectional_iteratable<_tIter, typename _tTraits::reference>;
		};

		template<typename _tIter, typename _tTraits>
		struct iterator_operators<_tIter, _tTraits, std::random_access_iterator_tag>
		{
			using type = random_access_iteratable<_tIter,
				typename _tTraits::difference_type, typename _tTraits::reference>;
		};

	} // namespace details;


	  /*!
	  \ingroup metafunctions
	  \brief �����������ȡ��ʵ�ֵ����������ز��������ϵ�ʵ�֡�
	  \note ��ʹ�õڶ��������ض���Ա�������Ǽ��� std::iterator ��ʵ�������͡�
	  \warning ʵ����Ϊʵ�ֵ������Ļ���ʱ��Ӧʹ��Ĭ�ϲ�������Ϊ��ʱ�޷����ʳ�Ա���͡�
	  */
	template<typename _tIter, typename _tTraits = std::iterator_traits<_tIter>>
	using iterator_operators_t = _t<details::iterator_operators<_tIter, _tTraits>>;
}

#endif
