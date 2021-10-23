/*! \file deref_op.hpp
\ingroup WBase
\brief �����ò�����
\par �޸�ʱ��:
2016-12-10 01:51 +0800
*/

#ifndef WBase_deref_op_hpp
#define WBase_deref_op_hpp 1

#include "type_traits.hpp"

using stdex::nullptr_t;

namespace white {
	/*!	\defgroup is_undereferenceable Is Undereferenceable Iterator
	\brief �жϵ�����ʵ���Ƿ񲻿ɽ����á�
	\tparam _tIter ���������͡�
	\note ע�ⷵ�� \c false ����ʾ����ʵ�ʿɽ����á�
	*/
	//@{
	template<typename _tIter>
	wconstfn bool
		is_undereferenceable(const _tIter&) wnothrow
	{
		return{};
	}
	template<typename _type>
	wconstfn bool
		is_undereferenceable(_type* p) wnothrow
	{
		return !p;
	}
	//@}


	//! \since build 1.4
	//@{
	//! \brief ȡ�ǿ����û�Ĭ��ֵ��
	//@{
	template<typename _type>
	wconstfn auto
		nonnull_or(_type&& p) -> decay_t<decltype(wforward(p))>
	{
		return p ? wforward(p) : decay_t<decltype(wforward(p))>();
	}
	template<typename _tOther, typename _type>
	wconstfn auto
		nonnull_or(_type&& p, _tOther&& other)
		->wimpl(decltype(p ? wforward(p) : wforward(other)))
	{
		return p ? wforward(p) : wforward(other);
	}
	template<typename _tOther, typename _type, typename _tSentinal = nullptr_t>
	wconstfn auto
		nonnull_or(_type&& p, _tOther&& other, _tSentinal&& last)->wimpl(
			decltype(!bool(p == wforward(last)) ? wforward(p) : wforward(other)))
	{
		return !bool(p == wforward(last)) ? wforward(p) : wforward(other);
	}
	//@}

	/*!
	\brief ���÷����û�Ĭ��ֵ��
	*/
	//@{
	template<typename _type, typename _func>
	wconstfn auto
		call_nonnull_or(_func f, _type&& p) -> decay_t<decltype(f(wforward(p)))>
	{
		return p ? f(wforward(p)) : decay_t<decltype(f(wforward(p)))>();
	}
	template<typename _tOther, typename _type, typename _func>
	wconstfn auto
		call_nonnull_or(_func f, _type&& p, _tOther&& other)
		->wimpl(decltype(p ? f(wforward(p)) : wforward(other)))
	{
		return p ? f(wforward(p)) : wforward(other);
	}
	template<typename _tOther, typename _type, typename _func,
		typename _tSentinal = nullptr_t>
		wconstfn auto
		call_nonnull_or(_func f, _type&& p, _tOther&& other, _tSentinal&& last)
		->wimpl(
			decltype(!bool(p == wforward(last)) ? f(wforward(p)) : wforward(other)))
	{
		return !bool(p == wforward(last)) ? f(wforward(p)) : wforward(other);
	}
	//@}

	//! \brief ȡ�ǿ�ֵ��Ĭ��ֵ��
	//@{
	template<typename _type>
	wconstfn auto
		value_or(_type&& p) -> decay_t<decltype(*wforward(p))>
	{
		return p ? *wforward(p) : decay_t<decltype(*wforward(p))>();
	}
	template<typename _tOther, typename _type>
	wconstfn auto
		value_or(_type&& p, _tOther&& other)
		->wimpl(decltype(p ? *wforward(p) : wforward(other)))
	{
		return p ? *wforward(p) : wforward(other);
	}
	template<typename _tOther, typename _type, typename _tSentinal = nullptr_t>
	wconstfn auto
		value_or(_type&& p, _tOther&& other, _tSentinal&& last)->wimpl(
			decltype(!bool(p == wforward(last)) ? *wforward(p) : wforward(other)))
	{
		return !bool(p == wforward(last)) ? *wforward(p) : wforward(other);
	}
	//@}


	//! \brief ���÷ǿ�ֵ��ȡĬ��ֵ��
	//@{
	template<typename _type, typename _func>
	wconstfn auto
		call_value_or(_func f, _type&& p) -> decay_t<decltype(f(*wforward(p)))>
	{
		return p ? f(*wforward(p)) : decay_t<decltype(f(*wforward(p)))>();
	}
	template<typename _tOther, typename _type, typename _func>
	wconstfn auto
		call_value_or(_func f, _type&& p, _tOther&& other)
		->wimpl(decltype(p ? f(*wforward(p)) : wforward(other)))
	{
		return p ? f(*wforward(p)) : wforward(other);
	}
	template<typename _tOther, typename _type, typename _func,
		typename _tSentinal = nullptr_t>
		wconstfn auto
		call_value_or(_func f, _type&& p, _tOther&& other, _tSentinal&& last)->wimpl(
			decltype(!bool(p == wforward(last)) ? f(*wforward(p)) : wforward(other)))
	{
		return !bool(p == wforward(last)) ? f(*wforward(p)) : wforward(other);
	}
	//@}
	//@}
}

#endif