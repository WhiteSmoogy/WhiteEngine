/*!	\file functional.hpp
\ingroup WBase
\brief �����Ϳɵ��ö���
\par �޸�ʱ��:
2017-02-18 17:11 +0800
*/

#ifndef WBase_functional_hpp
#define WBase_functional_hpp 1

#include "type_op.hpp" // for "tuple.hpp", true_type, std::tuple,
//	is_convertible, vseq::at, bool_constant, index_sequence_for,
//	member_target_type_t, _t, std::tuple_size, vseq::join_n_t, std::swap,
//	common_nonvoid_t, false_type, integral_constant, is_nothrow_swappable,
//	make_index_sequence, exclude_self_t;
#include "functor.hpp"// for "ref.hpp", <functional>, std::function,
//	__cpp_lib_invoke, less, addressof_op, mem_get;
#include <numeric>// for std::accumulate;

#ifdef WB_IMPL_MSCPP
#pragma warning(push)
#pragma warning(disable:4003)
#endif

namespace white
{
	//@{
	namespace details
	{

		template<class, class, class>
		struct tuple_element_convertible;

		template<class _type1, class _type2>
		struct tuple_element_convertible<_type1, _type2, index_sequence<>> : true_
		{};

		template<typename... _types1, typename... _types2, size_t... _vSeq,
			size_t _vHead>
			struct tuple_element_convertible<std::tuple<_types1...>, std::tuple<_types2...>,
			index_sequence<_vHead, _vSeq...>>
		{
			static_assert(sizeof...(_types1) == sizeof...(_types2),
				"Mismatched sizes of tuple found.");

		private:
			using t1 = std::tuple<_types1...>;
			using t2 = std::tuple<_types2...>;

		public:
			static wconstexpr const bool value
				= is_convertible<vseq::at<t1, _vHead>, vseq::at<t2, _vHead>>::value
				&& tuple_element_convertible<t1, t2, index_sequence<_vSeq...>>::value;
		};

	} // namespace details;

	  /*!
	  \ingroup binary_type_traits
	  */
	  //@{
	  //! \brief �ж�ָ������֮���Ƿ�Э�䡣
	  //@{
	template<typename _tFrom, typename _tTo>
	struct is_covariant : is_convertible<_tFrom, _tTo>
	{};

	template<typename _tFrom, typename _tTo, typename... _tFromParams,
		typename... _tToParams>
		struct is_covariant<_tFrom(_tFromParams...), _tTo(_tToParams...)>
		: is_covariant<_tFrom, _tTo>
	{};

	template<typename... _tFroms, typename... _tTos>
	struct is_covariant<std::tuple<_tFroms...>, std::tuple<_tTos...>>
		: bool_<details::tuple_element_convertible<std::tuple<_tFroms...>,
		std::tuple<_tTos...>, index_sequence_for<_tTos...>>::value>
	{};

	template<typename _tFrom, typename _tTo, typename... _tFromParams,
		typename... _tToParams>
		struct is_covariant<std::function<_tFrom(_tFromParams...)>,
		std::function<_tTo(_tToParams...)>>
		: is_covariant<_tFrom(_tFromParams...), _tTo(_tToParams...)>
	{};
	//@}


	//! \brief �ж�ָ������֮���Ƿ���䡣
	//@{
	template<typename _tFrom, typename _tTo>
	struct is_contravariant : is_convertible<_tTo, _tFrom>
	{};

	template<typename _tResFrom, typename _tResTo, typename... _tFromParams,
		typename... _tToParams>
		struct is_contravariant<_tResFrom(_tFromParams...), _tResTo(_tToParams...)>
		: is_contravariant<std::tuple<_tFromParams...>, std::tuple<_tToParams...>>
	{};

	template<typename... _tFroms, typename... _tTos>
	struct is_contravariant<std::tuple<_tFroms...>, std::tuple<_tTos...>>
		: bool_<details::tuple_element_convertible<std::tuple<_tTos...>,
		std::tuple<_tFroms...>, index_sequence_for<_tTos...>>::value>
	{};

	template<typename _tResFrom, typename _tResTo, typename... _tFromParams,
		typename... _tToParams>
		struct is_contravariant<std::function<_tResFrom(_tFromParams...)>,
		std::function<_tResTo(_tToParams...)>>
		: is_contravariant<_tResFrom(_tFromParams...), _tResTo(_tToParams...)>
	{};
	//@}
	//@}


	//@{
	//! \brief ͳ�ƺ��������б��еĲ���������
	template<typename... _tParams>
	wconstfn size_t
		sizeof_params(_tParams&&...) wnothrow
	{
		return sizeof...(_tParams);
	}


	//@{
	//! \brief �䳤��������ģ�塣
	//@{
	template<size_t _vN>
	struct variadic_param
	{
		template<typename _type, typename... _tParams>
		static wconstfn auto
			get(_type&&, _tParams&&... args) wnothrow
			-> decltype(variadic_param<_vN - 1>::get(wforward(args)...))
		{
			static_assert(sizeof...(args) == _vN,
				"Wrong variadic arguments number found.");

			return variadic_param<_vN - 1>::get(wforward(args)...);
		}
	};

	template<>
	struct variadic_param<0U>
	{
		template<typename _type>
		static wconstfn auto
			get(_type&& arg) wnothrow -> decltype(wforward(arg))
		{
			return wforward(arg);
		}
	};
	//@}


	/*!
	\brief ȡָ��λ�õı䳤������
	\tparam _vN ��ʾ����λ�õķǸ���������ʼ��������һ������Ϊ 0 ��
	*/
	template<size_t _vN, typename... _tParams>
	wconstfn auto
		varg(_tParams&&... args) wnothrow
		-> decltype(variadic_param<_vN>::get(wforward(args)...))
	{
		static_assert(_vN < sizeof...(args),
			"Out-of-range index of variadic argument found.");

		return variadic_param<_vN>::get(wforward(args)...);
	}
	//@}


	//! \see ���ڵ��ò������ͣ� ISO C++11 30.3.1.2 [thread.thread.constr] ��
	//@{
	//! \brief ˳����ʽ���á�
	//@{
	template<typename _func>
	inline void
		chain_apply(_func&& f) wnothrow
	{
		return wforward(f);
	}
	template<typename _func, typename _type, typename... _tParams>
	inline void
		chain_apply(_func&& f, _type&& arg, _tParams&&... args)
		wnoexcept_spec(white::chain_apply(
			wforward(wforward(f)(wforward(arg))), wforward(args)...))
	{
		return white::chain_apply(wforward(wforward(f)(wforward(arg))),
			wforward(args)...);
	}
	//@}

	//! \brief ˳��ݹ���á�
	//@{
	template<typename _func>
	inline void
		seq_apply(_func&&) wnothrow
	{}
	template<typename _func, typename _type, typename... _tParams>
	inline void
		seq_apply(_func&& f, _type&& arg, _tParams&&... args)
		wnoexcept_spec(wimpl(wunseq(0, (void(wforward(f)(wforward(args))), 0)...)))
	{
		wforward(f)(wforward(arg));
		white::seq_apply(wforward(f), wforward(args)...);
	}
	//@}

	//! \brief ������á�
	template<typename _func, typename... _tParams>
	inline void
		unseq_apply(_func&& f, _tParams&&... args)
		wnoexcept_spec(wimpl(wunseq((void(wforward(f)(wforward(args))), 0)...)))
	{
		wunseq((void(wforward(f)(wforward(args))), 0)...);
	}
	//@}
	//@}

	// TODO: Blocked. Wait for upcoming ISO C++17 for %__cplusplus.
#if __cpp_lib_invoke >= 201411
	using std::invoke;
#else
	namespace details
	{

		template<typename _type, typename _tCallable>
		struct is_callable_target
			: is_base_of<member_target_type_t<_tCallable>, decay_t<_type>>
		{};

		template<typename _fCallable, typename _type>
		struct is_callable_case1 : and_<is_member_function_pointer<_fCallable>,
			is_callable_target<_type, _fCallable>>
		{};

		template<typename _fCallable, typename _type>
		struct is_callable_case2 : and_<is_member_function_pointer<_fCallable>,
			not_<is_callable_target<_type, _fCallable>>>
		{};

		template<typename _fCallable, typename _type>
		struct is_callable_case3 : and_<is_member_object_pointer<_fCallable>,
			is_callable_target<_type, _fCallable>>
		{};

		template<typename _fCallable, typename _type>
		struct is_callable_case4 : and_<is_member_object_pointer<_fCallable>,
			not_<is_callable_target<_type, _fCallable>>>
		{};

		template<typename _fCallable, typename _type, typename... _tParams>
		wconstfn auto
			invoke_impl(_fCallable&& f, _type&& obj, _tParams&&... args)
			-> enable_if_t<is_callable_case1<decay_t<_fCallable>, _type>::value,
			decltype((wforward(obj).*f)(wforward(args)...))>
		{
			return wconstraint(f), (wforward(obj).*f)(wforward(args)...);
		}
		template<typename _fCallable, typename _type, typename... _tParams>
		wconstfn auto
			invoke_impl(_fCallable&& f, _type&& obj, _tParams&&... args)
			-> enable_if_t<is_callable_case2<decay_t<_fCallable>, _type>::value,
			decltype(((*wforward(obj)).*f)(wforward(args)...))>
		{
			return wconstraint(f), ((*wforward(obj)).*f)(wforward(args)...);
		}
		template<typename _fCallable, typename _type>
		wconstfn auto
			invoke_impl(_fCallable&& f, _type&& obj)
			-> enable_if_t<is_callable_case3<decay_t<_fCallable>, _type>::value,
			decltype(wforward(obj).*f)>
		{
			return wconstraint(f), wforward(obj).*f;
		}
		template<typename _fCallable, typename _type>
		wconstfn auto
			invoke_impl(_fCallable&& f, _type&& obj)
			-> enable_if_t<is_callable_case4<decay_t<_fCallable>, _type>::value,
			decltype((*wforward(obj)).*f)>
		{
			return wconstraint(f), (*wforward(obj)).*f;
}
		template<typename _func, typename... _tParams>
		wconstfn auto
			invoke_impl(_func&& f, _tParams&&... args)
			-> enable_if_t<!is_member_pointer<decay_t<_func>>::value,
			decltype(wforward(f)(wforward(args)...))>
		{
			return wforward(f)(wforward(args)...);
		}

} // namespace details;

  /*!
  \brief ���ÿɵ��ö���
  \sa http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4169.html
  \see WG21 N4527 20.9.2[func.require] �� WG21 N4527 20.9.3[func.invoke] ��
  \see LWG 2013 ��
  \see CWG 1581 ��
  \see https://llvm.org/bugs/show_bug.cgi?id=23141 ��
  \todo ֧�����ð�װ
  */
	template<typename _fCallable, typename... _tParams>
	wimpl(wconstfn) invoke_result_t<_fCallable, _tParams&&...>
		invoke(_fCallable&& f, _tParams&&... args)
	{
		return details::invoke_impl(wforward(f), wforward(args)...);
	}
#endif

	namespace details
	{

		template<typename _fCallable, typename... _tParams>
		wconstfn pseudo_output
			invoke_nonvoid_impl(true_type, _fCallable&& f, _tParams&&... args)
		{
			return white::invoke(wforward(f), wforward(args)...), pseudo_output();
		}
		template<typename _fCallable, typename... _tParams>
		inline invoke_result_t<_fCallable&&,_tParams&&...>
			invoke_nonvoid_impl(false_type, _fCallable&& f, _tParams&&... args)
		{
			return white::invoke(wforward(f), wforward(args)...);
		}

	} // namespace details;

	  /*!
	  \brief ���ÿɵ��ö��󣬱�֤����ֵ�ǿա�
	  */
	template<typename _fCallable, typename... _tParams>
	wimpl(wconstfn) nonvoid_result_t<invoke_result_t<_fCallable&&,_tParams&&...>>
		invoke_nonvoid(_fCallable&& f, _tParams&&... args)
	{
		return details::invoke_nonvoid_impl(is_void<invoke_result_t<
			_fCallable&& , _tParams&&...>>(), wforward(f), wforward(args)...);
	}

	/*!
	\brief ʹ�� invoke ���÷ǿ�ֵ��ȡĬ��ֵ��
	\sa call_value_or
	\sa invoke
	*/
	//@{
	template<typename _type, typename _func>
	wconstfn auto
		invoke_value_or(_func f, _type&& p)
		-> decay_t<decltype(invoke(f, *wforward(p)))>
	{
		return p ? invoke(f, *wforward(p))
			: decay_t<decltype(invoke(f, *wforward(p)))>();
	}
	template<typename _tOther, typename _type, typename _func>
	wconstfn auto
		invoke_value_or(_func f, _type&& p, _tOther&& other)
		->wimpl(decltype(p ? invoke(f, *wforward(p)) : wforward(other)))
	{
		return p ? invoke(f, *wforward(p)) : wforward(other);
	}
	template<typename _tOther, typename _type, typename _func,
		typename _tSentinal = stdex::nullptr_t>
		wconstfn auto
		invoke_value_or(_func f, _type&& p, _tOther&& other, _tSentinal&& last)
		->wimpl(decltype(!bool(p == wforward(last))
			? invoke(f, *wforward(p)) : wforward(other)))
	{
		return !bool(p == wforward(last)) ? invoke(f, *wforward(p))
			: wforward(other);
	}
	//@}

	/*!
	\ingroup metafunctions
	\brief ȡ�����б�Ԫ�顣
	*/
	//@{
	template<typename>
	struct make_parameter_tuple;

	template<typename _fCallable>
	using make_parameter_tuple_t = _t<make_parameter_tuple<_fCallable>>;

	template<typename _fCallable>
	struct make_parameter_tuple<_fCallable&> : make_parameter_tuple<_fCallable>
	{};

	template<typename _fCallable>
	struct make_parameter_tuple<_fCallable&&> : make_parameter_tuple<_fCallable>
	{};

#define WB_Impl_Functional_ptuple_spec(_exp, _p, _q) \
	template<typename _tRet, _exp typename... _tParams> \
	struct make_parameter_tuple<_tRet _p (_tParams...) _q> \
	{ \
		using type = std::tuple<_tParams...>; \
	};

	WB_Impl_Functional_ptuple_spec(, , )
		WB_Impl_Functional_ptuple_spec(, (*), )

#define WB_Impl_Functional_ptuple_spec_mf(_q) \
	WB_Impl_Functional_ptuple_spec(class _tClass WPP_Comma, (_tClass::*), _q)

		WB_Impl_Functional_ptuple_spec_mf()
		//@{
		WB_Impl_Functional_ptuple_spec_mf(const)
		WB_Impl_Functional_ptuple_spec_mf(volatile)
		WB_Impl_Functional_ptuple_spec_mf(const volatile)
		//@}

#undef WB_Impl_Functional_ptuple_spec_mf

#undef WB_Impl_Functional_ptuple_spec

		template<typename _tRet, typename... _tParams>
	struct make_parameter_tuple<std::function<_tRet(_tParams...)>>
	{
		using type = std::tuple<_tParams...>;
	};
	//@}


	/*!
	\ingroup metafunctions
	\brief ȡ�������͡�
	*/
	//@{
	template<typename>
	struct return_of;

	template<typename _fCallable>
	using return_of_t = _t<return_of<_fCallable>>;

	template<typename _fCallable>
	struct return_of<_fCallable&> : return_of<_fCallable>
	{};

	template<typename _fCallable>
	struct return_of<_fCallable&&> : return_of<_fCallable>
	{};

#define WB_Impl_Functional_ret_spec(_exp, _p, _e, _q) \
	template<typename _tRet, _exp typename... _tParams> \
	struct return_of<_tRet _p (_tParams... WPP_Args _e) _q> \
	{ \
		using type = _tRet; \
	};

#define WB_Impl_Functional_ret_spec_f(_e) \
	WB_Impl_Functional_ret_spec(, , _e, ) \
	WB_Impl_Functional_ret_spec(, (*), _e, )

	WB_Impl_Functional_ret_spec_f()
		WB_Impl_Functional_ret_spec_f((, ...))

#undef WB_Impl_Functional_ret_spec_f

#define WB_Impl_Functional_ret_spec_mf(_e, _q) \
	WB_Impl_Functional_ret_spec(class _tClass WPP_Comma, (_tClass::*), \
		_e, _q)

#define WB_Impl_Functional_ret_spec_mfq(_e) \
	WB_Impl_Functional_ret_spec_mf(_e, ) \
	WB_Impl_Functional_ret_spec_mf(_e, const) \
	WB_Impl_Functional_ret_spec_mf(_e, volatile) \
	WB_Impl_Functional_ret_spec_mf(_e, const volatile)


		WB_Impl_Functional_ret_spec_mfq()
		WB_Impl_Functional_ret_spec_mfq((, ...))

#undef WB_Impl_Functional_ret_spec_mfq
#undef WB_Impl_Functional_ret_spec_mf

#undef WB_Impl_Functional_ret_spec

		template<typename _tRet, typename... _tParams>
	struct return_of<std::function<_tRet(_tParams...)>>
	{
		using type = _tRet;
	};
	//@}


	/*!
	\ingroup metafunctions
	\brief ȡָ�������Ĳ������͡�
	*/
	//@{
	template<size_t _vIdx, typename _fCallable>
	struct parameter_of
	{
		using type = tuple_element_t<_vIdx,
			_t<make_parameter_tuple<_fCallable>>>;
	};

	template<size_t _vIdx, typename _fCallable>
	using parameter_of_t = _t<parameter_of<_vIdx, _fCallable>>;
	//@}


	/*!
	\ingroup metafunctions
	\brief ȡ�����б��С��
	*/
	template<typename _fCallable>
	struct paramlist_size : size_t_<std::tuple_size<typename
		make_parameter_tuple<_fCallable>::type>::value>
	{};


	/*!
	\ingroup metafunctions
	*/
	//@{
	//! \brief ȡָ���������ͺ�Ԫ��ָ���������͵ĺ������͡�
	//@{
	template<typename, class>
	struct make_function_type;

	template<typename _tRet, class _tTuple>
	using make_function_type_t = _t<make_function_type<_tRet, _tTuple>>;

	template<typename _tRet, typename... _tParams>
	struct make_function_type<_tRet, std::tuple<_tParams...>>
	{
		using type = _tRet(_tParams...);
	};
	//@}


	/*!
	\brief ���ñ������ء�
	*/
	template<template<typename...> class _gOp, typename _func, typename... _tParams>
	using enable_fallback_t = enable_if_t<!is_detected<_gOp, _tParams&&...>::value,
		decltype(std::declval<_func>()(std::declval<_tParams&&>()...))>;


	//! \brief ȡָ��ά����ָ���������͵Ķ�Ԫӳ����չ��Ⱥ������͡�
	template<typename _type, size_t _vN = 1, typename _tParam = _type>
	using id_func_t
		= make_function_type_t<_type, vseq::join_n_t<_vN, std::tuple<_tParam>>>;

	//! \brief ȡָ��ά���� const ��ֵ���ò������͵Ķ�Ԫӳ����չ��Ⱥ������͡�
	template<typename _type, size_t _vN = 1>
	using id_func_clr_t = id_func_t<_type, _vN, const _type&>;

	//! \brief ȡָ��ά���� const ��ֵ���ò������͵Ķ�Ԫӳ����չ��Ⱥ������͡�
	template<typename _type, size_t _vN = 1>
	using id_func_rr_t = id_func_t<_type, _vN, _type&&>;
	//@}


	/*!
	\brief ���ϵ��� std::bind �� std::placeholders::_1 ��
	\note ISO C++ Ҫ�� std::placeholders::_1 ��ʵ��֧�֡�
	*/
	//@{
	template<typename _func, typename... _tParams>
	inline auto
		bind1(_func&& f, _tParams&&... args) -> decltype(
			std::bind(wforward(f), std::placeholders::_1, wforward(args)...))
	{
		return std::bind(wforward(f), std::placeholders::_1, wforward(args)...);
	}
	template<typename _tRes, typename _func, typename... _tParams>
	inline auto
		bind1(_func&& f, _tParams&&... args) -> decltype(
			std::bind<_tRes>(wforward(f), std::placeholders::_1, wforward(args)...))
	{
		return std::bind<_tRes>(wforward(f), std::placeholders::_1, wforward(args)...);
	}
	//@}

	/*!
	\brief ���ϵ��� white::bind1 �� std::placeholders::_2 ��ʵ��ֵ�����á�
	\note ���ҵ������Ӧ�ò�����
	\note ISO C++ Ҫ�� std::placeholders::_2 ��ʵ��֧�֡�
	*/
	template<typename _func, typename _func2, typename... _tParams>
	inline auto
		bind_forward(_func&& f, _func2&& f2, _tParams&&... args)
		-> decltype(white::bind1(wforward(f), std::bind(wforward(f2),
			std::placeholders::_2, wforward(args)...)))
	{
		return white::bind1(wforward(f), std::bind(wforward(f2),
			std::placeholders::_2, wforward(args)...));
	}


	//@{
	//! \brief ���Ϻ�����
	template<typename _func, typename _func2>
	struct composed
	{
		_func f;
		_func2 g;

		/*!
		\note ÿ������ֻ�ں������ñ��ʽ�г���һ�Ρ�
		*/
		template<typename... _tParams>
		wconstfn auto
			operator()(_tParams&&... args) const wnoexcept_spec(f(g(wforward(args)...)))
			-> decltype(f(g(wforward(args)...)))
		{
			return f(g(wforward(args)...));
		}
	};

	/*!
	\brief �������ϡ�
	\note ���һ���������ȱ����ã�����Ϊ��Ԫ���������������ϵĺ�����Ҫ��֤��һ��������
	\relates composed
	\return ���ϵĿɵ��ö���
	*/
	//@{
	template<typename _func, typename _func2>
	wconstfn composed<_func, _func2>
		compose(_func f, _func2 g)
	{
		return composed<_func, _func2>{f, g};
	}
	template<typename _func, typename _func2, typename _func3, typename... _funcs>
	wconstfn auto
		compose(_func f, _func2 g, _func3 h, _funcs... args)
		-> decltype(white::compose(white::compose(f, g), h, args...))
	{
		return white::compose(white::compose(f, g), h, args...);
	}
	//@}
	//@}


	//@{
	//! \brief ��Ԫ�ַ��ĸ��Ϻ�����
	template<typename _func, typename _func2>
	struct composed_n
	{
		_func f;
		_func2 g;

		//! \note �ڶ������ᱻ�ַ�����γ����ں������ñ��ʽ�С�
		template<typename... _tParams>
		wconstfn auto
			operator()(_tParams&&... args) const wnoexcept_spec(f(g(wforward(args))...))
			-> decltype(f(g(wforward(args))...))
		{
			return f(g(wforward(args))...);
		}
	};

	/*!
	\brief ��һ���ɵĶ�Ԫ�������ϡ�
	\note ��һ��������󱻵��ã�����Ϊ��Ԫ���������������ϵĺ�����Ҫ��֤��һ��������
	\relates composed_n
	\return ��һ���ɵĶ�Ԫ���ϵĿɵ��ö���
	*/
	//@{
	template<typename _func, typename _func2>
	wconstfn composed_n<_func, _func2>
		compose_n(_func f, _func2 g)
	{
		return composed_n<_func, _func2>{f, g};
	}
	template<typename _func, typename _func2, typename _func3, typename... _funcs>
	wconstfn auto
		compose_n(_func f, _func2 g, _func3 h, _funcs... args)
		-> decltype(white::compose_n(white::compose_n(f, g), h, args...))
	{
		return white::compose_n(white::compose_n(f, g), h, args...);
	}
	//@}


	//! \brief ��Ԫ���Ϻ�����
	template<typename _func, typename... _funcs>
	struct generalized_composed
	{
		_func f;
		std::tuple<_funcs...> g;

		template<typename... _tParams>
		wconstfn auto
			operator()(_tParams&&... args) const wnoexcept_spec(wimpl(call(
				index_sequence_for<_tParams...>(), wforward(args)...))) -> decltype(
					wimpl(call(index_sequence_for<_tParams...>(), wforward(args)...)))
		{
			return call(index_sequence_for<_tParams...>(), wforward(args)...);
		}

	private:
		template<size_t... _vSeq, typename... _tParams>
		wconstfn auto
			call(index_sequence<_vSeq...>, _tParams&&... args) const
			wnoexcept_spec(f(std::get<_vSeq>(g)(wforward(args))...))
			-> decltype(f(std::get<_vSeq>(g)(wforward(args))...))
		{
			return f(std::get<_vSeq>(g)(wforward(args))...);
		}
	};

	/*!
	\brief ��Ԫ�������ϡ�
	\relates generalized_composed
	\return �Զ�Ԫ�������ϵĿɵ��ö���
	*/
	template<typename _func, typename... _funcs>
	wconstfn generalized_composed<_func, std::tuple<_funcs...>>
		generalized_compose(_func f, _funcs... args)
	{
		return generalized_composed<_func,
			std::tuple<_funcs...>>{f, make_tuple(args...)};
	}
	//@}


	/*!
	\brief ����һ�εĺ�����װģ�塣
	\pre ��̬���ԣ���������ͽ��ת���Լ�Ĭ��״̬�����״̬�������׳��쳣��
	\todo �Ż� std::function �ȿɿ����͵�ʵ�֡�
	\todo ���þ�̬���ԡ�
	\todo ��ת��ʵ�֡�
	*/
	//@{
	template<typename _func, typename _tRes = void, typename _tState = bool>
	struct one_shot
	{
		static_assert(is_nothrow_move_constructible<_func>::value,
			"Invalid target type found.");
		static_assert(is_nothrow_move_constructible<_tRes>::value,
			"Invalid result type found.");
		static_assert(is_nothrow_default_constructible<_tState>::value,
			"Invalid state type found.");
		static_assert(is_nothrow_swappable<_tState>::value ,
			"Invalid state type found.");

		_func func;
		mutable _tRes result;
		mutable _tState fresh{};

		one_shot(_func f, _tRes r = {}, _tState s = {})
			: func(f), result(r), fresh(s)
		{}
		one_shot(one_shot&& f) wnothrow
			: func(std::move(f.func)), result(std::move(f.result))
		{
			using std::swap;

			swap(fresh, f.fresh);
		}

		template<typename... _tParams>
		wconstfn_relaxed auto
			operator()(_tParams&&... args) const
			wnoexcept(noexcept(func(wforward(args)...)))
			-> decltype(func(wforward(args)...))
		{
			if (fresh)
			{
				result = func(wforward(args)...);
				fresh = {};
			}
			return std::forward<_tRes>(result);
		}
	};

	template<typename _func, typename _tState>
	struct one_shot<_func, void, _tState>
	{
		static_assert(is_nothrow_move_constructible<_func>::value,
			"Invalid target type found.");
		static_assert(is_nothrow_default_constructible<_tState>::value,
			"Invalid state type found.");
		static_assert(is_nothrow_swappable<_tState>::value,
			"Invalid state type found.");

		_func func;
		mutable _tState fresh{};

		one_shot(_func f, _tState s = {})
			: func(f), fresh(s)
		{}
		one_shot(one_shot&& f) wnothrow
			: func(std::move(f.func))
		{
			using std::swap;

			swap(fresh, f.fresh);
		}

		template<typename... _tParams>
		wconstfn_relaxed void
			operator()(_tParams&&... args) const
			wnoexcept(noexcept(func(wforward(args)...)))
		{
			if (fresh)
			{
				func(wforward(args)...);
				fresh = {};
			}
		}
	};

	template<typename _func, typename _tRes>
	struct one_shot<_func, _tRes, void>
	{
		static_assert(is_nothrow_move_constructible<_func>::value,
			"Invalid target type found.");
		static_assert(is_nothrow_move_constructible<_tRes>::value,
			"Invalid result type found.");


		mutable _func func;
		mutable _tRes result;

		one_shot(_func f, _tRes r = {})
			: func(f), result(r)
		{}

		template<typename... _tParams>
		wconstfn_relaxed auto
			operator()(_tParams&&... args) const
			wnoexcept(noexcept(func(wforward(args)...)))
			-> decltype(func(wforward(args)...) && noexcept(func = {}))
		{
			if (func)
			{
				result = func(wforward(args)...);
				func = {};
			}
			return std::forward<_tRes>(result);
		}
	};

	template<typename _func>
	struct one_shot<_func, void, void>
	{
		static_assert(is_nothrow_move_constructible<_func>::value,
			"Invalid target type found.");

		mutable _func func;

		one_shot(_func f)
			: func(f)
		{}

		template<typename... _tParams>
		wconstfn_relaxed void
			operator()(_tParams&&... args) const
			wnoexcept(noexcept(func(wforward(args)...) && noexcept(func = {})))
		{
			if (func)
			{
				func(wforward(args)...);
				func = {};
			}
		}
	};
	//@}


	/*!
	\ingroup functors
	*/
	//@{
	//! \brief get ��Ա���ڷº�����
	//@{
	template<typename _type = void>
	struct get_equal_to
		: composed_n<equal_to<_type*>, composed<addressof_op<_type>, mem_get<>>>
	{};

	template<>
	struct get_equal_to<void>
		: composed_n<equal_to<>, composed<addressof_op<>, mem_get<>>>
	{};
	//@}

	//! \brief get ��ԱС�ڷº�����
	//@{
	template<typename _type = void>
	struct get_less
		: composed_n<less<_type*>, composed<addressof_op<_type>, mem_get<>>>
	{};

	template<>
	struct get_less<void>
		: composed_n<less<>, composed<addressof_op<>, mem_get<>>>
	{};
	//@}
	//@}


	namespace details
	{

		template<typename _type, typename _fCallable, typename... _tParams>
		_type
			call_for_value(true_, _type&& val, _fCallable&& f, _tParams&&... args)
		{
			white::invoke(wforward(f), wforward(args)...);
			return wforward(val);
		}

		template<typename _type, typename _fCallable, typename... _tParams>
		auto
			call_for_value(false_, _type&&, _fCallable&& f, _tParams&&... args)
			-> invoke_result_t<_fCallable,_tParams...>
		{
			return white::invoke(wforward(f), wforward(args)...);
		}

	} // unnamed namespace;

	  /*!
	  \brief ���õڶ���������ָ���ĺ������������ؿ�������ʹ�õ�һ��������ֵΪ����ֵ��
	  */
	template<typename _type, typename _fCallable, typename... _tParams>
	auto
		call_for_value(_type&& val, _fCallable&& f, _tParams&&... args)
		-> common_nonvoid_t<invoke_result_t<_fCallable,_tParams...>, _type>
	{
		return details::call_for_value(is_void<invoke_result_t<_fCallable,
			_tParams...>>(), wforward(val), wforward(f), wforward(args)...);
	}


	/*!
	\brief ����ͶӰ����ԭ���ô�������ָ����λ�õĲ�����
	*/
	//@{
	template<class, class>
	struct call_projection;

	template<typename _tRet, typename... _tParams, size_t... _vSeq>
	struct call_projection<_tRet(_tParams...), index_sequence<_vSeq...>>
	{
		//@{
		template<typename _func>
		static wconstfn auto
			apply_call(_func&& f, std::tuple<_tParams...>&& args, wimpl(decay_t<
				decltype(wforward(f)(std::get<_vSeq>(wforward(args))...))>* = {}))
			->wimpl(decltype(wforward(f)(std::get<_vSeq>(wforward(args))...)))
		{
			return wforward(f)(std::get<_vSeq>(wforward(args))...);
		}

		template<typename _fCallable>
		static wconstfn auto
			apply_invoke(_fCallable&& f, std::tuple<_tParams...>&& args,
				wimpl(decay_t<decltype(white::invoke(wforward(f),
					std::get<_vSeq>(wforward(args))...))>* = {}))->wimpl(decltype(
						white::invoke(wforward(f), std::get<_vSeq>(wforward(args))...)))
		{
			return white::invoke(wforward(f), std::get<_vSeq>(wforward(args))...);
		}
		//@}

		template<typename _func>
		static wconstfn auto
			call(_func&& f, _tParams&&... args)
			->wimpl(decltype(call_projection::apply_call(wforward(f),
				std::forward_as_tuple(wforward(args)...))))
		{
			return call_projection::apply_call(wforward(f),
				std::forward_as_tuple(wforward(args)...));
		}

		template<typename _fCallable>
		static wconstfn auto
			invoke(_fCallable&& f, _tParams&&... args)
			->wimpl(decltype(call_projection::apply_invoke(wforward(f),
				std::forward_as_tuple(wforward(args)...))))
		{
			return call_projection::apply_invoke(wforward(f),
				std::forward_as_tuple(wforward(args)...));
		}
	};

	template<typename _tRet, typename... _tParams, size_t... _vSeq>
	struct call_projection<std::function<_tRet(_tParams...)>,
		index_sequence<_vSeq...>> : private
		call_projection<_tRet(_tParams...), index_sequence<_vSeq...>>
	{
		using call_projection<_tRet(_tParams...),
			index_sequence<_vSeq...>>::apply_call;
		using call_projection<_tRet(_tParams...),
			index_sequence<_vSeq...>>::apply_invoke;
		using call_projection<_tRet(_tParams...), index_sequence<_vSeq...>>::call;
		using call_projection<_tRet(_tParams...), index_sequence<_vSeq...>>::invoke;
	};

	/*!
	\note ����Ҫ��ʽָ���������͡�
	*/
	template<typename... _tParams, size_t... _vSeq>
	struct call_projection<std::tuple<_tParams...>, index_sequence<_vSeq...>>
	{
		template<typename _func>
		static wconstfn auto
			apply_call(_func&& f, std::tuple<_tParams...>&& args)
			->wimpl(decltype(wforward(f)(std::get<_vSeq>(wforward(args))...)))
		{
			return wforward(f)(std::get<_vSeq>(wforward(args))...);
		}

		template<typename _fCallable>
		static wconstfn auto
			apply_invoke(_fCallable&& f, std::tuple<_tParams...>&& args)->wimpl(
				decltype(white::invoke(wforward(f), std::get<_vSeq>(args)...)))
		{
			return white::invoke(wforward(f), std::get<_vSeq>(args)...);
		}

		template<typename _func>
		static wconstfn auto
			call(_func&& f, _tParams&&... args)
			-> decltype(call_projection::apply_call(wforward(f),
				std::forward_as_tuple(wforward(wforward(args))...)))
		{
			return call_projection::apply_call(wforward(f),
				std::forward_as_tuple(wforward(wforward(args))...));
		}

		template<typename _fCallable>
		static wconstfn auto
			invoke(_fCallable&& f, _tParams&&... args)
			->wimpl(decltype(call_projection::apply_invoke(
				wforward(f), std::forward_as_tuple(wforward(args)...))))
		{
			return call_projection::apply_invoke(wforward(f),
				std::forward_as_tuple(wforward(args)...));
		}
	};
	//@}


	/*!
	\brief Ӧ�ú�������Ͳ���Ԫ�顣
	\tparam _func �����������������͡�
	\tparam _tTuple Ԫ�鼰���������͡�
	\see WG21 N4606 20.5.2.5[tuple.apply]/1 ��
	\see http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4023.html#tuple.apply ��
	*/
	template<typename _func, class _tTuple>
	wconstfn auto
		apply(_func&& f, _tTuple&& args)
		->wimpl(decltype(call_projection<_tTuple, make_index_sequence<
			std::tuple_size<decay_t<_tTuple>>::value>>::call(wforward(f),
				wforward(args))))
	{
		return call_projection<_tTuple, make_index_sequence<std::tuple_size<
			decay_t<_tTuple>>::value>>::call(wforward(f), wforward(args));
	}

	//@{
	template<typename _fCallable, size_t _vLen = paramlist_size<_fCallable>::value>
	struct expand_proxy : private call_projection<_fCallable,
		make_index_sequence<_vLen>>, private expand_proxy<_fCallable, _vLen - 1>
	{
		/*!
		\note Ϊ�������壬��ֱ��ʹ�� using ������
		\see CWG 1393 ��
		\see EWG 102 ��
		*/
		//@{
		//@{
		using call_projection<_fCallable, make_index_sequence<_vLen>>::apply_call;
		template<typename... _tParams>
		static auto
			apply_call(_tParams&&... args) -> decltype(
				expand_proxy<_fCallable, _vLen - 1>::apply_call(wforward(args)...))
		{
			return expand_proxy<_fCallable, _vLen - 1>::apply_call(
				wforward(args)...);
		}

		using call_projection<_fCallable, make_index_sequence<_vLen>>::apply_invoke;
		template<typename... _tParams>
		static auto
			apply_invoke(_tParams&&... args) -> decltype(
				expand_proxy<_fCallable, _vLen - 1>::apply_invoke(wforward(args)...))
		{
			return expand_proxy<_fCallable, _vLen - 1>::apply_invoke(
				wforward(args)...);
		}
		//@}

		using call_projection<_fCallable, make_index_sequence<_vLen>>::call;
		template<typename... _tParams>
		static auto
			call(_tParams&&... args) -> decltype(
				expand_proxy<_fCallable, _vLen - 1>::call(wforward(args)...))
		{
			return expand_proxy<_fCallable, _vLen - 1>::call(wforward(args)...);
		}

		//@{
		using call_projection<_fCallable, make_index_sequence<_vLen>>::invoke;
		template<typename... _tParams>
		static auto
			invoke(_tParams&&... args) -> decltype(
				expand_proxy<_fCallable, _vLen - 1>::invoke(wforward(args)...))
		{
			return expand_proxy<_fCallable, _vLen - 1>::invoke(wforward(args)...);
		}
		//@}
		//@}
	};

	template<typename _fCallable>
	struct expand_proxy<_fCallable, 0>
		: private call_projection<_fCallable, index_sequence<>>
	{
		using call_projection<_fCallable, index_sequence<>>::call;
	};
	//@}


	/*!
	\brief ѭ���ظ����ã�����ֱ��ʹ�� do-while ����Ա������������������ı�����
	\tparam _fCond �ж�������
	\tparam _fCallable �ɵ��ö������͡�
	\tparam _tParams �������͡�
	\note �������ܵ��ý����û�в�����
	\sa object_result_t
	*/
	template<typename _fCond, typename _fCallable, typename... _tParams>
	invoke_result_t<_fCallable,_tParams...>
		retry_on_cond(_fCond cond, _fCallable&& f, _tParams&&... args)
	{
		using res_t = invoke_result_t<_fCallable ,_tParams...>;
		using obj_t = object_result_t<res_t>;
		obj_t res;

		do
			res = white::invoke_nonvoid(wforward(f), wforward(args)...);
		while (expand_proxy<bool(obj_t&)>::call(cond, res));
		return res_t(res);
	}


	/*!
	\brief ������������Ŀɵ��ö���
	\todo ֧�� ref-qualifier ��
	*/
	template<typename _fHandler, typename _fCallable>
	struct expanded_caller
	{
		static_assert(is_object<_fCallable>::value, "Callable object type is needed.");

		_fCallable caller;

		template<typename _fCaller,
			wimpl(typename = exclude_self_t<expanded_caller, _fCaller>)>
			expanded_caller(_fCaller&& f)
			: caller(wforward(f))
		{}

		template<typename... _tParams>
		auto
			operator()(_tParams&&... args) const -> decltype(
				expand_proxy<_fHandler>::call(caller, wforward(args)...))
		{
			return expand_proxy<_fHandler>::call(caller, wforward(args)...);
		}
	};

	/*!
	\ingroup helper_functions
	\brief ���������������Ŀɵ��ö���
	\relates expanded_caller
	*/
	template<typename _fHandler, typename _fCallable>
	wconstfn expanded_caller<_fHandler, decay_t<_fCallable>>
		make_expanded(_fCallable&& f)
	{
		return expanded_caller<_fHandler, decay_t<_fCallable>>(wforward(f));
	}


	/*!
	\brief �ϲ�ֵ���С�
	\note ����ͬ Boost.Signal2 �� \c boost::last_value ���Է� \c void ����ʹ��Ĭ��ֵ��
	*/
	//@{
	template<typename _type>
	struct default_last_value
	{
		template<typename _tIn>
		_type
			operator()(_tIn first, _tIn last, const _type& val = {}) const
		{
			return std::accumulate(first, last, val,
				[](const _type&, decltype(*first) v) {
				return wforward(v);
			});
		}
	};

	template<>
	struct default_last_value<void>
	{
		template<typename _tIn>
		void
			operator()(_tIn first, _tIn last) const
		{
			for (; first != last; ++first)
				*first;
		}
	};
	//@}

} // namespace white;


#ifdef WB_IMPL_MSCPP
#pragma warning(pop)
#endif
#endif