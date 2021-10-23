/*!\file variadic.hpp
\ingroup WBase
\brief C++ �䳤������ز�����
*/

#ifndef WBase_variadic_hpp
#define WBase_variadic_hpp 1

#include "type_traits.hpp" // for "type_traits.hpp", _t, size_t, empty_base,
//	false_type, is_same, true_type, are_same, integral_constant, conditional_t,
//	cond_t;

namespace white
{
	/*!	\defgroup vseq_operations Variadic Sequence Operations
	\ingroup meta_operations
	\brief �䳤������ǵ�������ص�Ԫ������
	\since build 1.4

	��ʽΪģ�������������ػ�����ز������������������������͡�
	���˴����ػ��⣬�������������͵��ػ���
	�ػ������豣֤���б�ʾ�ʹ˴��ػ�������ͬ�� type ���ͳ�Ա����������������Ա��
	�Է�����Ԫ�أ���Ա value ��ʾ�������Ա type ��ʾ��Ӧ���������͡�
	������Ԫ�����������һ�� void Ĭ��ģ�����ʱ�����е���ģ����������û������ػ���
	���һ���������� SFINAE ��
	*/

	//!\since build 1.4
	namespace vseq
	{

		/*!
		\ingroup vseq_operations
		\since build 1.4
		*/
		//@{
		/*!
		\brief Ԫ����Ӧ�á�

		Ҫ��Ӧ�õĺ����Ǿ��п���ΪԪ�������õ�Ƕ�׳�Ա apply �����͡�
		*/
		//@{
		template<class _func, typename... _tParams>
		using apply = typename _func::template apply<_tParams...>;

		template<class _func, typename... _tParams>
		using apply_t = _t<apply<_func, _tParams...>>;
		//@}


#define WB_Impl_Variadic_SeqOp(_n, _tparams, _targs) \
	template<_tparams, typename = void> \
	struct _n : _n<_targs, when<true>> \
	{}; \
	\
	template<_tparams, typename _tEnabler = void> \
	using _n##_t = _t<_n<_targs, _tEnabler>>;
#define WB_Impl_Variadic_SeqOpU(_n) \
	WB_Impl_Variadic_SeqOp(_n, class _tSeq, _tSeq)
#define WB_Impl_Variadic_SeqOpB(_n) WB_Impl_Variadic_SeqOp(_n, class _tSeq1 \
	WPP_Comma class _tSeq2, _tSeq1 WPP_Comma _tSeq2)
#define WB_Impl_Variadic_SeqOpI(_n) WB_Impl_Variadic_SeqOp(_n, class _tSeq \
	WPP_Comma size_t _vIdx, _tSeq WPP_Comma _vIdx)
#define WB_Impl_Variadic_SeqOpN(_n) WB_Impl_Variadic_SeqOp(_n, size_t _vN \
	WPP_Comma class _tSeq, _vN WPP_Comma _tSeq)


		/*!
		\brief �ӳ���ֵ��
		\since build 1.4
		*/
		//@{
		WB_Impl_Variadic_SeqOp(defer, template<typename...> class _gOp WPP_Comma class
			_tSeq, _gOp WPP_Comma _tSeq)

			template<template<typename...> class _gOp, template<typename...> class _gSeq,
			typename... _types>
		struct defer<_gOp, _gSeq<_types...>, when_valid<_gOp<_types...>>>
		{
			using type = _gOp<_types...>;
		};


		WB_Impl_Variadic_SeqOp(defer_i, typename _type WPP_Comma template<_type...>
		class _gOp WPP_Comma class _tSeq, _type WPP_Comma _gOp WPP_Comma _tSeq)

			template<typename _tInt, template<_tInt...> class _gOp,
			template<typename, _tInt...> class _gSeq, _tInt... _vSeq>
		struct defer_i<_tInt, _gOp, _gSeq<_tInt, _vSeq...>, when_valid<_gOp<_vSeq...>>>
		{
			using type = _gOp<_vSeq...>;
		};
		//@}


		/*!
		\brief �ɱ���������б��ӳ���ֵ��
		\sa defer
		\since build 1.4
		*/
		template<template<typename...> class _gOp, typename... _types>
		struct vdefer : defer<_gOp, empty_base<_types...>>
		{};

		//! \since build 1.4
		template<class _func, class _tParamsList>
		using defer_apply_t = _t<defer_t<_func::template apply, _tParamsList>>;
		//@}


		/*!	\defgroup metafunction_composition Metafunction Compositions
		\ingroup metafunctions
		\brief Ԫ������ϲ�����
		\sa vseq::apply
		\since build 1.4

		�������Ԫ����Ӧ�����������͵�ģ�塣
		Ҫ�������Ԫ����������Ԫ����Ӧ�����������͡�
		*/
		//@{
		//! \brief ����Ӧ�á�
		//@{
		template<class _func, typename... _tParams>
		struct bind_back
		{
			template<typename... _types>
			using apply = vseq::apply<_func, _types..., _tParams...>;
		};


		template<class _func, typename... _tParams>
		struct bind_front
		{
			template<typename... _types>
			using apply = vseq::apply<_func, _tParams..., _types...>;
		};
		//@}


		/*!
		\note vdefer �Ǳ�Ҫ�ģ��������ģ����ΪԪ����ʱ�޷��Ƶ�������
		\sa vdefer
		\see http://wg21.cmeerw.net/lwg/issue1430 ��
		*/
		//@{
		/*!
		\brief ������� \c apply ��Ա��һ��Ԫ������
		\since build 1.4
		*/
		template<template<typename...> class _gOp>
		struct _a
		{
			template<typename... _types>
			using apply = vdefer<_gOp, _types...>;
		};


		/*!
		\brief ���ã��ӳ���ֵ�任��
		\since build 1.4
		\todo ֧��û�� \c apply ��Ա�ķ�Ԫ������
		*/
		template<class _func>
		struct _q
		{
			// NOTE: Ideally, the template argument should be limited to enable check.
			//	However this is impossible since the arity of '_func::apply' is
			//	unknown.
			template<typename... _types>
			using apply = vdefer<apply, _func, _types...>;
		};
		//@}
		//@}


		//! \since build 1.4
		//@{
		/*!
		\note ����ƥ�������Ĭ��ʵ�ֲ������������в�����
		\note ������Ϊ���Ͳ�������ģ���ⲻ�ṩĬ��ʵ�֣���Ծ��������ػ���
		*/
		//@{
		/*!
		\brief ȡ���͹����������� apply ��Ա�������Ƕ�� type ����Ϊ�������͵�Ԫ������
		\note Ĭ��ʵ�ֲ����ģ��ʵ�����Ƴ����Ͳ�����
		\sa _a
		*/
		//@{
		WB_Impl_Variadic_SeqOpU(ctor_of)

			template<template<typename...> class _gSeq, typename... _types>
		struct ctor_of<_gSeq<_types...>>
		{
			using type = _a<_gSeq>;
		};
		//@}


		/*!
		\brief ȡ���͹�����������
		\note Ĭ��ʵ�ֲ����ģ��ʵ�����Ƴ����Ͳ�����
		\sa ctor_of
		\sa empty_base
		*/
		//@{
		WB_Impl_Variadic_SeqOpU(params_of)

			template<template<typename...> class _gSeq, typename... _types>
		struct params_of<_gSeq<_types...>>
		{
			using type = empty_base<_types...>;
		};
		//@}
		//@}


		//! \brief ʹ�ù�����Ӧ��ָ���б������
		template<class _tSeq, class _tParamsList>
		using instance_apply_t = defer_apply_t<ctor_of_t<_tSeq>, _tParamsList>;


		/*!
		\brief �ж��Ƿ��ƥ������ʵ����
		\note ��Ϊʹ�� apply ����ģ�������ֵ�� is_decomposable ��һ�㻯��
		\sa apply_t
		\sa is_decomposable
		*/
		//@{
		template<class, typename = void>
		struct is_instance : false_type
		{};

		template<class _tSeq>
		struct is_instance<_tSeq, enable_if_t<is_same<_tSeq, _t<
			vdefer<instance_apply_t, _tSeq, params_of_t<_tSeq>> >> ::value >> : true_type
		{};
		//@}


		template<class... _tSeqs>
		using same_instance = are_same<ctor_of_t<_tSeqs>...>;


		/*!
		\sa enable_when
		\sa is_instance
		\sa same_instance
		*/
		template<typename... _tSeqs>
		using enable_for_instances = enable_when<and_<is_instance<_tSeqs>...,
			same_instance<_tSeqs... >> ::value>;
		//@}


		//! \note �������в�����Ĭ��ʵ�ֲ���������ƥ�䡢���졢ȡ������Ӧ���������Ԫ������
		//@{
		//! \brief ��λԪ��ȡ�����С�
		//@{
		WB_Impl_Variadic_SeqOpU(clear)

			//! \since build 684
			template<class _tSeq>
		struct clear<_tSeq, when<true>>
		{
			using type = apply_t<ctor_of_t<_tSeq>>;
		};
		//@}


		/*!
		\brief ���ӣ��ϲ����С�
		\note Ĭ��ʵ�ֺϲ���ģ��ʵ�������Ͳ�����
		*/
		//@{
		WB_Impl_Variadic_SeqOpB(concat)

			//! \since build 1.4
			//@{
			template<typename... _types1, typename... _types2>
		struct concat<empty_base<_types1...>, empty_base<_types2...>>
		{
			using type = empty_base<_types1..., _types2...>;
		};

		template<class _tSeq1, class _tSeq2>
		struct concat<_tSeq1, _tSeq2, enable_for_instances<_tSeq1, _tSeq2>>
		{
			using type = instance_apply_t<_tSeq1,
				concat_t<params_of_t<_tSeq1>, params_of_t<_tSeq2>>>;
		};
		//@}


		/*!
		\brief ����ӳ�䣺ʹ��ָ�����͹����������в��������µ����͡�
		\since build 1.4
		*/
		//@{
		WB_Impl_Variadic_SeqOp(fmap, class _tCtor WPP_Comma class _tSeq,
			_tCtor WPP_Comma _tSeq)

			//! \since build 684
			template<class _tCtor, typename... _types>
		struct fmap<_tCtor, empty_base<_types...>>
		{
			using type = apply_t<_tCtor, _types...>;
		};

		//! \since build 1.4
		template<class _tCtor, class _tSeq>
		struct fmap<_tCtor, _tSeq, enable_for_instances<_tSeq>>
		{
			using type = defer_apply_t<_tCtor, params_of_t<_tSeq>>;
		};
		//@}


		/*!
		\brief ȡ���е�һ��Ԫ�ء�
		\note Ĭ��ʵ��ȡ��ģ��ĵ�һ�����Ͳ�����
		*/
		//@{
		WB_Impl_Variadic_SeqOpU(front)

			//! \since build 1.4
			template<typename _tHead, typename... _tTail>
		struct front<empty_base<_tHead, _tTail...>>
		{
			using type = _tHead;
		};

		//! \since build 1.4
		template<class _tSeq>
		struct front<_tSeq, enable_for_instances<_tSeq>>
		{
			using type = front_t<params_of_t<_tSeq>>;
		};
		//@}


		/*!
		\brief ȡ���е�һ��Ԫ�������Ԫ�ص����С�
		\note Ĭ��ʵ���Ƴ���ģ��ĵ�һ�����Ͳ�����
		*/
		//@{
		WB_Impl_Variadic_SeqOpU(pop_front)

			//! \since build 1.4
			template<typename _tHead, typename... _tTail>
		struct pop_front<empty_base<_tHead, _tTail...>>
		{
			using type = empty_base<_tTail...>;
		};

		//! \since build 1.4
		template<class _tSeq>
		struct pop_front<_tSeq, enable_for_instances<_tSeq>>
		{
			using type = instance_apply_t<_tSeq, pop_front_t<params_of_t<_tSeq>>>;
		};
		//@}


		/*!
		\brief ȡ������ĩβ����һ��Ԫ�ص����С�
		\note Ĭ��ʵ������ģ��Ĳ����б��������Ͳ�����
		*/
		//@{
		WB_Impl_Variadic_SeqOp(push_back, class _tSeq WPP_Comma typename _tItem,
			_tSeq WPP_Comma _tItem)

			//! \since build 1.4
			template<typename... _types, typename _type>
		struct push_back<empty_base<_types...>, _type>
		{
			using type = empty_base<_types..., _type>;
		};

		//! \since build 1.4
		template<class _tSeq, typename _type>
		struct push_back<_tSeq, _type, enable_for_instances<_tSeq>>
		{
			using type
				= instance_apply_t<_tSeq, push_back_t<params_of_t<_tSeq>, _type>>;
		};
		//@}


		/*!
		\brief ȡ���е�һ��Ԫ�������Ԫ�ص����С�
		\note Ĭ��ʵ������ģ��Ĳ����б�ǰ������Ͳ�����
		*/
		//@{
		//! \brief ȡ��������ʼ����һ��Ԫ�ص����С�
		WB_Impl_Variadic_SeqOp(push_front, class _tSeq WPP_Comma typename _tItem,
			_tSeq WPP_Comma _tItem)

			//! \since build 1.4
			template<typename... _types, typename _type>
		struct push_front<empty_base<_types...>, _type>
		{
			using type = empty_base<_type, _types...>;
		};

		//! \since build 1.4
		template<class _tSeq, typename _type>
		struct push_front<_tSeq, _type, enable_for_instances<_tSeq>>
		{
			using type
				= instance_apply_t<_tSeq, push_front_t<params_of_t<_tSeq>, _type>>;
		};
		//@}


		/*!
		\brief ȡ����Ԫ������
		\note Ĭ��ʵ�ֲ����ģ��ʵ����ȡ���Ͳ����ĸ�����
		*/
		//@{
		WB_Impl_Variadic_SeqOpU(seq_size)

			//! \since build 1.4
			template<typename... _types>
		struct seq_size<empty_base<_types...>>
			: integral_constant<size_t, sizeof...(_types)>
		{};

		//! \since build 1.4
		template<class _tSeq>
		struct seq_size<_tSeq, enable_for_instances<_tSeq>>
			: seq_size_t<params_of_t<_tSeq>>
		{};
		//@}
		//@}


		/*!
		\brief ȡָ��λ�õ�Ԫ�ء�
		\note Ĭ��ʵ������ front �� pop_front ��
		*/
		//@{
		WB_Impl_Variadic_SeqOpI(at)

			//! \since build 1.4
			template<class _tSeq, size_t _vIdx>
		struct at<_tSeq, _vIdx, when<true>>
			: at<pop_front_t<_tSeq>, _vIdx - 1>
		{};

		//! \since build 1.4
		template<class _tSeq>
		struct at<_tSeq, 0, when<true>> : front<_tSeq>
		{};
		//@}


		/*!
		\brief ȡ�������һ��Ԫ�ء�
		\note Ĭ��ʵ������ seq_size �� at ��
		*/
		//@{
		WB_Impl_Variadic_SeqOpU(back)

			//! \since build 1.4
			template<class _tSeq>
		struct back<_tSeq, when<true>> : at<_tSeq, seq_size<_tSeq>::value - 1>
		{};
		//@}


		/*!
		\brief �������ǰ����Ԫ�ء�
		\note Ĭ��ʵ������ clear �� front �� push_front �� pop_front �� concat ��
		*/
		//@{
		WB_Impl_Variadic_SeqOpN(split_n)

			//! \since build 1.4
			//@{
			//! \note ʹ�ö���ʵ�ּ��ٵݹ�ʵ������ȡ�
			template<size_t _vN, class _tSeq>
		struct split_n<_vN, _tSeq, when<true>>
		{
		private:
			using half = split_n<_vN / 2, _tSeq>;
			using last = split_n<_vN - _vN / 2, typename half::tail>;

		public:
			using type = concat_t<_t<half>, _t<last>>;
			using tail = typename last::tail;
		};

		template<class _tSeq>
		struct split_n<0, _tSeq, when<true>>
		{
			using type = clear_t<_tSeq>;
			using tail = _tSeq;
		};

		template<class _tSeq>
		struct split_n<1, _tSeq, when<true>>
		{
			using type = push_front_t<clear_t<_tSeq>, front_t<_tSeq>>;
			using tail = pop_front_t<_tSeq>;
		};
		//@}
		//@}


		/*!
		\brief ȡ�������Ԫ�������Ԫ�ص����С�
		\note Ĭ��ʵ������ seq_size �� split ��
		*/
		//@{
		WB_Impl_Variadic_SeqOpU(pop_back)

			//! \since build 1.4
			template<class _tSeq>
		struct pop_back<_tSeq, when<true>> : split_n<seq_size<_tSeq>::value - 1, _tSeq>
		{};
		//@}


		/*!
		\brief ɾ��ָ��λ�õ�Ԫ�ء�
		\note Ĭ��ʵ������ split �� concat ��
		*/
		//@{
		WB_Impl_Variadic_SeqOp(erase, class _tSeq WPP_Comma size_t _vIdx WPP_Comma
			size_t _vEnd = _vIdx + 1, _tSeq WPP_Comma _vIdx WPP_Comma _vEnd)

			//! \since build 1.4
			template<class _tSeq, size_t _vIdx, size_t _vEnd>
		struct erase<_tSeq, _vIdx, _vEnd, when<true>>
		{
			static_assert(_vIdx <= _vEnd, "Invalid range found.");

		public:
			using type = concat_t<split_n_t<_vIdx, _tSeq>,
				typename split_n<_vEnd, _tSeq>::tail>;
		};
		//@}


		/*!
		\brief ����Ԫ�ء�
		\note Ĭ��ʵ������ front �� pop_front ��
		*/
		//@{
		WB_Impl_Variadic_SeqOp(find, class _tSeq WPP_Comma typename _type,
			_tSeq WPP_Comma _type)

		namespace details
		{

			template<size_t _vN, class _tSeq, typename _type>
			struct find
			{
				static wconstexpr const size_t value = is_same<front_t<_tSeq>, _type>::value
					? 0 : find<_vN - 1, pop_front_t<_tSeq>, _type>::value + 1;
			};

			template<class _tSeq, typename _type>
			struct find<0, _tSeq, _type>
			{
				static wconstexpr const size_t value = 0;
			};

		} // namespace details;

		  //! \since build 1.4
		template<class _tSeq, typename _type>
		struct find<_tSeq, _type, when<true>> : integral_constant<size_t,
			details::find<seq_size<_tSeq>::value, _tSeq, _type>::value>
		{};
		//@}


		/*!
		\brief ȡ�ϲ���ͬԪ�غ�����С�
		\note Ĭ��ʵ������ seq_size �� pop_front �� front �� find �� push_front ��
		\note Ĭ��ʵ�ֱ���β��Ԫ�ء�
		*/
		//@{
		WB_Impl_Variadic_SeqOpU(deduplicate)

		namespace details
		{

			template<size_t, class _tSeq>
			struct deduplicate
			{
			private:
				using head = front_t<_tSeq>;
				using tail = pop_front_t<_tSeq>;
				using sub = deduplicate_t<tail>;

			public:
				using type = conditional_t<vseq::find<tail, head>::value
					== seq_size<tail>::value, push_front_t<sub, head>, sub>;
			};

			template<class _tSeq>
			struct deduplicate<0, _tSeq>
			{
				using type = _tSeq;
			};

			template<class _tSeq>
			struct deduplicate<1, _tSeq>
			{
				using type = _tSeq;
			};

		} // namespace details;

		  //! \since build 1.4
		template<class _tSeq>
		struct deduplicate<_tSeq, when<true>>
		{
			using type = _t<details::deduplicate<seq_size<_tSeq>::value, _tSeq>>;
		};
		//@}


		/*!
		\brief �ظ���������Ԫ�ء�
		\note Ĭ��ʵ������ clear �� concat ��
		*/
		//@{
		WB_Impl_Variadic_SeqOpN(join_n)

			/*!
			\note ʹ�ö���ʵ�ּ��ٵݹ�ʵ������ȡ�
			\since build 1.4
			*/
			template<size_t _vN, class _tSeq>
		struct join_n<_vN, _tSeq, when<true>>
		{
		private:
			using unit = _tSeq;
			using half = join_n_t<_vN / 2, unit>;

		public:
			using type = concat_t<concat_t<half, half>, join_n_t<_vN % 2, unit>>;
		};

		template<class _tSeq>
		struct join_n<0, _tSeq>
		{
			using type = clear_t<_tSeq>;
		};

		template<class _tSeq>
		struct join_n<1, _tSeq>
		{
			using type = _tSeq;
		};
		//@}


		/*!
		\brief ͶӰ������
		\note Ĭ��ʵ������ at �� index_sequence ���μ�ģ�� IntegerSequence ��
		*/
		WB_Impl_Variadic_SeqOp(project, class _tSeq WPP_Comma class _tIdxSeq,
			_tSeq WPP_Comma _tIdxSeq)


			/*!
			\brief ȡ�����С�
			\note Ĭ��ʵ������ seq_size �� clear �� pop_front ��
			*/
			//@{
			WB_Impl_Variadic_SeqOpU(reverse)

			//! \since build 1.4
			template<class _tSeq>
		struct reverse<_tSeq, when<true>>
		{
			using type = conditional_t<seq_size<_tSeq>::value == 0, clear_t<_tSeq>,
				concat_t<reverse_t<pop_front_t<_tSeq>>, front_t<_tSeq>>>;
		};
		//@}


		/*!
		\brief ȡ�ϲ�������ͬԪ�غ�����С�
		\note Ĭ��ʵ������ seq_size �� clear �� pop_front �� concat ��
		*/
		//@{
		WB_Impl_Variadic_SeqOpU(unique)

		namespace details
		{

			/*!
			\note ʹ�ö���ʵ�ּ��ٵݹ�ʵ������ȡ�
			\since build 1.4
			*/
			//@{
			template<class _tSeq, size_t _vN = seq_size<_tSeq>::value>
			struct unique
			{
			private:
				using split = split_n<_vN / 2, _tSeq>;
				using half = _t<unique<typename split::type>>;
				using last = _t<unique<typename split::tail>>;
				using half_back = back_t<half>;
				using last_front = front_t<last>;

			public:
				using type = concat_t<half, cond_t<is_same<half_back, last_front>,
					pop_front_t<last>, last >> ;
			};

			template<class _tSeq>
			struct unique<_tSeq, 0>
			{
				using type = clear_t<_tSeq>;
			};

			template<class _tSeq>
			struct unique<_tSeq, 1>
			{
				using type = _tSeq;
			};
			//@}

		} // namespace details;

		  //! \since build 1.4
		template<class _tSeq>
		struct unique<_tSeq, when<true>>
		{
			using type = _t<details::unique<_tSeq>>;
		};


		/*!
		\brief ��Ԫ�����ϲ�Ӧ�á�
		\pre ��Ԫ�������Ͻ����ɺͽ���ɡ�
		\note Ĭ��ʵ������ seq_size �� front �� split_n ��
		*/
		//@{
		WB_Impl_Variadic_SeqOp(fold, class _fBinary WPP_Comma typename _tState WPP_Comma
		class _tSeq, _fBinary WPP_Comma _tState WPP_Comma _tSeq)

			//! \since build 1.4
			//@{
		namespace details
		{

			//! \note ʹ�ö���ʵ�ּ��ٵݹ�ʵ������ȡ�
			template<class _fBinary, typename _tState, typename _tSeq, size_t _vSize>
			struct fold
			{
			private:
				using parts = split_n<_vSize / 2, _tSeq>;
				using head = _t<parts>;
				using tail = typename parts::tail;

			public:
				using type = fold_t<_fBinary, fold_t<_fBinary, _tState, head>, tail>;
			};

			template<class _fBinary, typename _tState, typename _tSeq>
			struct fold<_fBinary, _tState, _tSeq, 0>
			{
				using type = _tState;
			};

			template<class _fBinary, typename _tState, typename _tSeq>
			struct fold<_fBinary, _tState, _tSeq, 1>
			{
				using type = apply_t<_fBinary, _tState, front_t<_tSeq>>;
			};

		} // namespace details;

		template<class _fBinary, typename _tState, typename _tSeq>
		struct fold<_fBinary, _tState, _tSeq, enable_for_instances<_tSeq>>
			: details::fold<_fBinary, _tState, _tSeq, seq_size_t<_tSeq>::value>
		{};
		//@}
		//@}


		//! \note ���ṩĬ��ʵ�֣���Ծ��������ػ���
		//@{
		//! \brief ������Ϊ�����ļӷ���
		WB_Impl_Variadic_SeqOpB(vec_add)


			//! \brief ������Ϊ�����ļ�����
			WB_Impl_Variadic_SeqOpB(vec_subtract)
			//@}

#undef WB_Impl_Variadic_SeqOpN
#undef WB_Impl_Variadic_SeqOpI
#undef WB_Impl_Variadic_SeqOpU
#undef WB_Impl_Variadic_SeqOp
			//@}

	} // namespace vseq;

	  //! \since build 1.4
	using vseq::vdefer;
}

#endif