/*!	\file algorithm.hpp
\ingroup WBase
\brief �����㷨��
*/

#ifndef WBase_algorithm_hpp
#define WBase_algorithm_hpp 1

#include "iterator_trait.hpp" // for have_same_iterator_category;
#include "functor.hpp" // for <algorithm>,
//	__cpp_lib_robust_nonmodifying_seq_ops, is_equal, std::bind,
//	std::placeholders::_1, less;
#include "cassert.h" // for wconstraint;
#include "deref_op.hpp" // for is_undereferenceable;
#include <cstring> // for std::memcpy, std::memmove;
#include <numeric> // for std::accumulate;

namespace white {
	using stdex::byte;

	/*!	\defgroup algorithms Gerneral Algorithms
	\brief �㷨��
	\note ��Χ���������������ڽ�����ȡ��������������Ҫ��μ� ISO C++ Clause 23 ��
	\see WG21 N3936 25.1[algorithms.general] ��
	\since build 1.3
	*/
	//@{
	/*!	\defgroup nonmodifying_algorithms Non-modifying Sequence Operations
	\brief ���޸����в�����
	\see WG21 N3936 25.2[alg.nonmodifying] ��
	\since build 1.4
	*/
	//@{
	/*!
	\brief ���������жϲ�����
	\note ��֤��������ʼ�жϣ�����ȷ������ֵ����֤���ɸ�ֵ���ж�һ�Σ�֮���ֵ�����жϡ�
	\since build 1.4
	*/
	//@{
	//! \note ������ֵ��ͬ std::all_of �ķ��ؽ����
	template<typename _tIn, typename _fPred>
	bool
		fast_all_of(_tIn first, _tIn last, _fPred pred)
	{
		while (first != last && bool(pred(*first)))
			++first;
		return first == last;
	}

	//! \note ������ֵ��ͬ std::any_of �ķ��ؽ����
	template<typename _tIn, typename _fPred>
	bool
		fast_any_of(_tIn first, _tIn last, _fPred pred)
	{
		while (first != last && !bool(pred(*first)))
			++first;
		return first != last;
	}

	//! \note ������ֵ��ͬ std::none_of �ķ��ؽ����
	template<typename _tIn, typename _fPred>
	bool
		fast_none_of(_tIn first, _tIn last, _fPred pred)
	{
		return !white::fast_any_of(first, last, pred);
	}
	//@}


	/*!
	\brief �ϸ������жϲ�����
	\note ��֤��������ʼ�жϣ�����ʱ�����е�ÿ��ֵ���ж�һ�Ρ�
	\since build 1.4
	*/
	//@{
	//! \note ������ֵ��ͬ std::all_of �ķ��ؽ����
	template<typename _tIn, typename _fPred>
	bool
		strict_all_of(_tIn first, _tIn last, _fPred pred)
	{
		return std::accumulate(first, last, true,
			[pred](bool b, decltype(*first) val) {
			return b && bool(pred(wforward(val)));
		});
	}


	//! \note ������ֵ��ͬ std::any_of �ķ��ؽ����
	template<typename _tIn, typename _fPred>
	bool
		strict_any_of(_tIn first, _tIn last, _fPred pred)
	{
		return std::accumulate(first, last, false,
			[pred](bool b, decltype(*first) val) {
			return b || bool(pred(wforward(val)));
		});
	}

	//! \note ������ֵ��ͬ std::none_of �ķ��ؽ����
	template<typename _tIn, typename _fPred>
	bool
		strict_none_of(_tIn first, _tIn last, _fPred pred)
	{
		return !white::strict_any_of(first, last, pred);
	}
	//@}


	/*!
	\tparam _func ���ڱ�����Χ�Ĳ����Ŀɵ������͡�
	\param first ���뷶Χ��ʼ��������
	\param last ���뷶Χ��ֹ��������
	\param f ����������
	\pre _func ���� MoveConstructible Ҫ��
	\pre _type ���� EqualityComparable Ҫ��
	\return ת�Ƶ� f ��
	\note ʱ�临�Ӷȣ� ������ <tt>last - first</tt> �� \c f �ĵ��á�
	\see WG21 N3936 25.2.4[alg.foreach] ��
	\see �����ο�ʵ�֣� http://stackoverflow.com/questions/234482/using-stl-to-find-all-elements-in-a-vector ��

	�ֱ�Ӧ�ò�������Χ <tt>[first, last)</tt> �ڵĽ�Ӧ�õĵ����� \c i ��������������
	\li <tt>*i == value</tt>
	\li <tt>pred(*i) != false</tt>
	*/
	//@{
	template<typename _tIn, typename _func, typename _type>
	_func
		for_each_equal(_tIn first, _tIn last, const _type& value, _func f)
	{
		for (first = std::find(first, last, value); first != last; first = std::find(++first, last, value))
			f(*first);
	}

	template<typename _tIn, typename _func, typename _fPred>
	_func
		for_each_if(_tIn first, _tIn last, _fPred pred, _func f)
	{
		for (first = std::find_if(first, last, pred); first != last; first = std::find_if(++first, last, pred))
			f(*first);
		return f;
	}
	//@}

	//! \since build 1.4
	inline namespace cpp2014
	{
		using std::equal;
#if !(__cpp_lib_robust_nonmodifying_seq_ops >= 201304 || __cplusplus > 201103L)
		template<typename _tIn1, typename _tIn2, typename _fBiPred>
		inline bool
			equal(_tIn1 first1, _tIn1 last1, _tIn2 first2, _tIn2 last2,
				_fBiPred binary_pred)
		{
			if (have_same_iterator_category<std::random_access_iterator_tag, _tIn1,
				_tIn2>())
			{
				if (std::distance(first1, last1) != std::distance(first2, last2))
					return{};
				return std::equal(first1, last1, first2, binary_pred);
			}
			for (; first1 != last1 && first2 != last2; ++first1, ++first2)
				if (!bool(binary_pred(*first1, *first2)))
					return{};
			return first1 == last1 && first2 == last2;
		}
		template<typename _tIn1, typename _tIn2>
		inline bool
			equal(_tIn1 first1, _tIn1 last1, _tIn2 first2, _tIn2 last2)
		{
			return white::equal(first1, last1, first2, last2, is_equal());
		}
#endif

	} // inline namespace cpp2014;


	  //! \note ֻ�����ǿս�����������ָ�����
	  //@{
	  /*!
	  \brief ��ָ���ָ����ָ����С�
	  \since build 1.3
	  */
	template<typename _fPred, typename _fInsert, typename _tIn>
	void
		split(_tIn b, _tIn e, _fPred is_delim, _fInsert insert)
	{
		while (b != e)
		{
			_tIn i(std::find_if_not(b, e, is_delim));

			b = std::find_if(i, e, is_delim);
			if (i != b)
				insert(i, b);
			else
				break;
		}
	}
	

	/*!
	\brief �����������ν�ʵ�ָ���ָ����ָ����С�
	\since build 1.4
	*/
	template<typename _fPred, typename _fInsert, typename _func, typename _tIn>
	void
		split_if(_tIn b, _tIn e, _fPred is_delim, _fInsert insert, _func pred)
	{
		while (b != e)
		{
			_tIn i(b);

			while (i != e && is_delim(*i) && pred(i))
				++i;
			for (b = i; b != e; ++b)
			{
				b = std::find_if(b, e, is_delim);
				if (b == e || pred(b))
					break;
			}
			if (i != b)
				insert(i, b);
			else
				break;
		}
	}


	/*!
	\brief ��ָ���ָ����ָ����ʼ�ָ��������С�
	\note �����޷�ƥ�䣬������ʼ�ָ�����
	\since build 1.3
	*/
	template<typename _fPred, typename _fInsert, typename _tIn>
	_tIn
		split_l(_tIn b, _tIn e, _fPred is_delim, _fInsert insert)
	{
		_tIn i(b);

		while (b != e)
		{
			if (is_delim(*b) && i != b)
			{
				insert(i, b);
				i = b;
			}
			++b;
		}
		if (i != b)
			insert(i, b);
		return i;
	}
	//@}
	//@}


	/*!	\defgroup mutating_algorithms Mutating Sequence Operations
	\brief �ɱ����в�����
	\see WG21 N3936 25.3[alg.modifying.operations] ��
	\since build 1.4
	*/
	//@{
	/*!
	\tparam _tIn ������������͡�
	\tparam _tOut ������������͡�
	\tparam _fPred ν�����͡�
	\param first ���������е���ʼ�����������
	\param result �����������
	\param pred һԪν�ʡ�
	\pre �������ɽ����á�
	\since build 1.4
	*/
	//@{
	/*!
	\brief �任����ν�ʵ����е�����Ԫ�ء�
	\tparam _fOp �任�������͡�
	\param op һԪ�任������
	*/
	template<typename _tIn, typename _tOut, typename _fPred, typename _fOp>
	_tOut
		transform_when(_tIn first, _tOut result, _fPred pred, _fOp op)
	{
		wunseq((wconstraint(!is_undereferenceable(first)), 0),
			(wconstraint(!is_undereferenceable(result)), 0));

		for (; pred(*first); wunseq((++first, 0), (++result, 0)))
			*result = op(*first);
		return result;
	}

	//! \brief ��������ν�ʵ����е�����Ԫ�ء�
	template<typename _tIn, typename _tOut, typename _fPred>
	_tOut
		copy_when(_tIn first, _tOut result, _fPred pred)
	{
		return white::transform_when(first, result, pred,
			[](decltype(*first)& v) wnothrow{
			return static_cast<decltype(*first)&>(v);
		});
	}
	//@}


	/*!
	\brief ָ�����������б任��
	\tparam _fOp ���б任�������͡�
	\tparam _tOut ��ʾ�����������������͡�
	\tparam _tIns ������������͡�
	\pre �������ɽ����á�
	\warning �����Խ�硣
	\since build 1.3
	*/
	template<typename _fOp, typename _tOut, typename... _tIns>
	void
		transform_n(_fOp op, _tOut result, size_t n, _tIns... iters)
	{
		while (n-- != 0)
		{
			wunseq((wconstraint(!is_undereferenceable(result)), 0),
				(wconstraint(!is_undereferenceable(iters)), void(iters), 0)...);
			*result = op((*iters)...);
			wunseq(++result, ++iters...);
		}
	}

	/*!	\defgroup trivial_type_operations Trivial Type Operations
	\brief ƽ�����Ͳ�����
	\tparam _type ָ���������͡�
	\pre ��̬���ԣ� <tt>is_trivial<_type>()</tt> ��
	\pre ��Χ�е�ָ����Ч��
	\pre ���ԣ���Χ��ʼָ������ <tt>!is_undereferenceable</tt> ��
	\since build 1.4
	\todo ʹ�� \c is_trivially_copyable ���� \c is_trivial ��
	*/
	//@{
	/*!
	\brief ����ֽ����С�
	\pre ָ�����뷶ΧҪ��ͬ std::memset ��
	*/
	//@{
	template<typename _type>
	inline _type*
		trivially_fill_n(_type* first, size_t n = 1, byte value = {}) wnothrowv
	{
		static_assert(is_trivial<_type>(), "Non-trivial type found.");

		wconstraint(!is_undereferenceable(first));
		std::memset(first, value, sizeof(_type) * n);
		return first + n;
	}

	template<typename _type>
	inline void
		trivially_fill(_type* first, _type* last, byte value = {}) wnothrowv
	{
		white::trivially_fill_n(first, last - first, value);
	}
	//@}

	/*!
	\brief ���Ʋ����ǵ����С�
	\pre ��̬���ԣ� <tt>is_copy_assignment<_type>()</tt> ��
	\pre ָ�����뷶ΧҪ��ͬ std::memcpy ��
	*/
	//@{
	template<typename _type>
	inline _type*
		trivially_copy_n(const _type* first, size_t n, _type* result) wnothrowv
	{
		static_assert(is_trivial<_type>(), "Non-trivial type found.");
		static_assert(is_copy_assignable<_type>(),
			"Type shall meet CopyAssignable.");

		wunseq((wconstraint(!is_undereferenceable(result)), 0),
			(wconstraint(!is_undereferenceable(first)), 0));
		std::memcpy(result, first, sizeof(_type) * n);
		return result + n;
	}

	template<typename _type>
	inline _type*
		trivially_copy(const _type* first, const _type* last, _type* result) wnothrowv
	{
		return white::trivially_copy_n(first, last - first, result);
	}
	//@}

	/*!
	\brief ���ƿ��ܸ��ǵ����С�
	\pre ��̬���ԣ� <tt>is_copy_assignment<_type>()</tt> ��
	\pre ָ�����뷶ΧҪ��ͬ std::memmove ��
	*/
	//@{
	template<class _type>
	inline _type*
		trivially_move_n(const _type* first, size_t n, _type* result)
	{
		static_assert(is_trivial<_type>(), "Non-trivial type found.");
		static_assert(is_copy_assignable<_type>(),
			"Type shall meet CopyAssignable.");

		wunseq((wconstraint(!is_undereferenceable(result)), 0),
			(wconstraint(!is_undereferenceable(first)), 0));
		std::memmove(result, first, sizeof(_type) * n);
		return result + n;
	}

	template<class _type>
	inline _type*
		trivially_move(const _type* first, const _type* last, _type* result)
	{
		return white::trivially_move_n(first, last - first, result);
	}
	//@}
	//@}


	/*!
	\brief ȥ��������ָ���ķ�Χ�е��ظ�Ԫ�أ��Ҳ��ı�Ԫ��֮������˳��
	\param first ���뷶Χ��ʼ��������
	\param last ���뷶Χ��ֹ��������
	\return �����Χ��ֹ��������
	\note ����������Χ����ʼ��������ͬ��
	\note �����ΧԪ��֮������˳�������ķ�Χ����һ�¡�
	\note ʱ�临�Ӷȣ� O(n^2) ������ n ���� <tt>std::advance(first, n) == last</tt> ��
	\note ʹ�� ADL ������
	\see WG21 N3936 25.3.9[alg.unique] ��
	\since build 1.3
	*/
	//@{
	template<typename _tFwd>
	_tFwd
		stable_unique(_tFwd first, _tFwd last)
	{
		_tFwd result(first);

		for (_tFwd i(first); i != last; ++i)
			if (std::find(first, result, *i) == result)
			{
				using std::swap;

				swap(*i, *result);
				++result;
			}
		return result;
	}
	//! \param pred �Ƚϵȼ۵Ķ�Ԫν�ʡ�
	template<typename _tFwd, typename _fPred>
	_tFwd
		stable_unique(_tFwd first, _tFwd last, _fPred pred)
	{
		_tFwd result(first);

		for (_tFwd i(first); i != last; ++i)
			if (std::find_if(first, result,
				std::bind(pred, std::ref(*i), std::placeholders::_1)) == result)
			{
				using std::swap;

				swap(*i, *result);
				++result;
			}
		return result;
	}
	//@}
	//@}


	/*!	\defgroup sorting_and_related_algorithms Sorting and Related Operations
	\brief ������ز�����
	\see WG21 N3936 25.4[alg.sorting] ��
	\since build 1.3
	*/
	//@{
	/*!
	\brief ����ָ�����з�Χ���������ظ�Ԫ�ص����䡣
	\tparam _tRandom ������������͡�
	\param first ���뷶Χ��ʼ��������
	\param last ���뷶Χ��ֹ��������
	\note ʱ�临�Ӷȣ��� N = last - first �� O(N log(N)) + N - 1 �αȽϡ�
	\return ���ظ�Ԫ�ص�����ĩβ��
	\since build 414
	*/
	template<typename _tRandom>
	inline _tRandom
		sort_unique(_tRandom first, _tRandom last)
	{
		std::sort(first, last);
		return std::unique(first, last);
	}


	/*!
	\brief ȡ��С��Ԫ�ء�
	\note ����ͬ ISO C++14 std::min �Ĵ� constexpr �����ء�
	\since build 1.4
	*/
	//@{
	template<typename _type, typename _fComp = less<_type>>
	wconstfn const _type&
		min(const _type& a, const _type& b, _fComp comp = less<_type>())
	{
		return comp(b, a) ? b : a;
	}
	template<typename _type, typename _fComp = less<_type>>
	wconstfn const _type&
		min(std::initializer_list<_type> t, _fComp comp = less<_type>(),
			wimpl(size_t n = 0))
	{
		return n + 1 < t.size() ? white::min(*(t.begin() + n),
			white::min(t, comp, n + 1)) : *(t.begin() + n);
	}
	//@}

	/*!
	\brief ȡ�ϴ��Ԫ�ء�
	\note ����ͬ ISO C++14 std::max �Ĵ� constexpr �����ء�
	\since build 1.4
	*/
	//@{
	template<typename _type, typename _fComp = less<_type>>
	wconstfn const _type&
		max(const _type& a, const _type& b, _fComp comp = less<_type>())
	{
		return comp(a, b) ? b : a;
	}
	template<typename _type, typename _fComp = less<_type>>
	wconstfn const _type&
		max(std::initializer_list<_type> t, _fComp comp = less<_type>(),
			wimpl(size_t n = 0))
	{
		return n + 1 < t.size() ? white::max(*(t.begin() + n),
			white::max(t, comp, n + 1)) : *(t.begin() + n);
	}
	//@}

	/*!
	\brief ȡԼ����Χ��ֵ��
	\sa http://www.open-std.org/JTC1/SC22/WG21/docs/papers/2015/n4536.html
	\since build 1.4
	*/
	//@{
	template<typename _type, typename _fComp = less<>>
	wconstfn const _type&
		clamp(const _type& v, const _type& lo, const _type& hi, _fComp comp = _fComp())
	{
		return wconstraint(!comp(hi, lo)), comp(v, lo) ? lo : comp(hi, v) ? hi : v;
	}

	template<typename _tIn, typename _tOut, typename _fComp = less<>>
	wconstfn _tOut
		clamp_range(_tIn first, _tIn last, _tOut result, const typename
			std::iterator_traits<_tIn>::value_type& lo, const typename
			std::iterator_traits<_tIn>::value_type& hi, _fComp comp = _fComp())
	{
		using arg_type = decltype(lo);

		return std::transform(first, last, result, [&](arg_type val) -> arg_type {
			return white::clamp(val, lo, hi, comp);
		});
	}
	//@}
	//@}
	//@}
}

#endif