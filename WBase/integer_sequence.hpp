/*!	\file integer_sequence.hpp
\ingroup WBase
\brief ��������Ԫ��̽ӿڡ�
*/

#ifndef Lbase_interger_sequence_hpp
#define Lbase_interger_sequence_hpp 1

#include "variadic.hpp" //for "varaadic.hpp",empty_base,vseq::defer_i,_t,
//	common_type_t;

namespace white {

	inline namespace cpp2014
	{

		/*!
		\ingroup meta_types
		\see ISO C++14 20.5 [intseq] ��
		\since build 1.4
		*/
		//@{
#if __cpp_lib_integer_sequence >= 201304 || __cplusplus > 201103L
		using std::integer_sequence;
		using std::index_sequence;
#else
		template<typename _tInt, _tInt... _vSeq>
		struct integer_sequence
		{
			using value_type = _tInt;

			static wconstfn size_t
				size() wnoexcept
			{
				return sizeof...(_vSeq);
			}
		};

		template<size_t... _vSeq>
		using index_sequence = integer_sequence<size_t, _vSeq...>;
#endif
		//@}

	} // inline namespace cpp2014;

	  //! \since build 1.4
	  //@{
	namespace vseq
	{

		//! \since build 1.4
		//@{
		template<class _tSeq, size_t... _vIdxSeq>
		struct project<_tSeq, index_sequence<_vIdxSeq...>, enable_for_instances<_tSeq>>
		{
			using type = instance_apply_t<_tSeq,
				empty_base<at_t<params_of_t<_tSeq>, _vIdxSeq>... >> ;
		};


		//! \since build 1.4
		template<typename _tInt>
		struct bound_integer_sequence
		{
			template<typename... _types>
			using apply = identity<integer_sequence<_tInt, _types::value...>>;
		};


		template<typename _tInt, _tInt... _vSeq>
		struct ctor_of<integer_sequence<_tInt, _vSeq...>>
		{
			using type = bound_integer_sequence<_tInt>;
		};


		template<typename _tInt, _tInt... _vSeq>
		struct params_of<integer_sequence<_tInt, _vSeq...>>
		{
			using type = empty_base<integral_constant<_tInt, _vSeq>...>;
		};
		//@}


		template<typename _tInt, _tInt... _vSeq1, _tInt... _vSeq2>
		struct vec_add<integer_sequence<_tInt, _vSeq1...>,
			integer_sequence<_tInt, _vSeq2... >>
		{
			using type = integer_sequence<_tInt, (_vSeq1 + _vSeq2)...>;
		};


		template<typename _tInt, _tInt... _vSeq1, _tInt... _vSeq2>
		struct vec_subtract<integer_sequence<_tInt, _vSeq1...>,
			integer_sequence<_tInt, _vSeq2... >>
		{
			using type = integer_sequence<_tInt, (_vSeq1 - _vSeq2)...>;
		};


		/*!
		\brief �ɱ���������б��ӳ���ֵ��
		\sa defer_i
		\since build 1.4
		*/
		template<typename _tInt, template<_tInt...> class _gOp, _tInt... _vSeq>
		struct vdefer_i
			: vseq::defer_i<_tInt, _gOp, integer_sequence<_tInt, _vSeq...>>
		{};

	} // namespace vseq;
	  //@}

	//! \since build 1.4
	using vseq::vdefer_i;

	/*!
	\ingroup metafunctions
	\since build 1.4
	*/
	//@{
	//! \brief ȡ�������е���Ȼ����̡�
	//@{
	template<typename _tInt, _tInt, class>
	struct make_successor;

	template<typename _tInt, _tInt _vBase, class _tSeq>
	using make_successor_t = _t<make_successor<_tInt, _vBase, _tSeq>>;

	template<typename _tInt, _tInt _vBase, _tInt... _vSeq>
	struct make_successor<_tInt, _vBase, integer_sequence<_tInt, _vSeq...>>
	{
	private:
		using common_t = common_type_t<size_t, _tInt>;

	public:
		// XXX: Conversion to '_tInt' might be implementation-defined.
		using type
			= integer_sequence<_tInt, _vSeq..., static_cast<_tInt>(static_cast<
				common_t>(_vBase) + static_cast<common_t>(sizeof...(_vSeq)))>;
	};
	//@}


	//! \brief ȡƤ��ŵ����������������С�
	//@{
	template<typename _tInt, _tInt, size_t>
	struct make_peano_sequence;

	//! \since build 1.4
	template<typename _tInt, _tInt _vBase, size_t _vN>
	using make_peano_sequence_t
		= _t<make_peano_sequence<_tInt, _vBase, _vN>>;

	template<typename _tInt, _tInt _vBase, size_t _vN>
	struct make_peano_sequence
	{
		using type = make_successor_t<_tInt, _vBase,
			make_peano_sequence_t<_tInt, _vBase, _vN - 1 >> ;
	};

	template<typename _tInt, _tInt _vBase>
	struct make_peano_sequence<_tInt, _vBase, 0>
	{
		using type = integer_sequence<_tInt>;
	};
	//@}


	//! \brief ȡ��Ȼ�����С�
	//@{
	template<typename _tInt, size_t _vN>
	using make_natural_sequence = make_peano_sequence<_tInt, 0, _vN>;

	template<typename _tInt, size_t _vN>
	using make_natural_sequence_t
		= _t<make_natural_sequence<_tInt, _vN>>;
	//@}

	/*!
	\brief �ж��Ƿ������ͬ��ָ��ֵ��
	\since build 1.4
	*/
	template<typename _tInt, _tInt _vDefault, _tInt... _values>
	using same_value_as = is_same<integer_sequence<_tInt, _vDefault,
		_values...>, integer_sequence<_tInt, _values..., _vDefault >> ;
	//@}

	inline namespace cpp2014
	{
		/*!
		\ingroup metafunctions
		\see ISO C++14 20.5 [intseq] ��
		\since build 1.4
		*/
		//@{
#if __cpp_lib_integer_sequence >= 201304 || __cplusplus > 201103L
		using std::make_integer_sequence;
		using std::make_index_sequence;

		using std::index_sequence_for;
#else
		template<typename _tInt, size_t _vN>
		using make_integer_sequence = make_natural_sequence_t<_tInt, _vN>;

		template<size_t _vN>
		using make_index_sequence = make_integer_sequence<size_t, _vN>;


		template<typename... _types>
		using index_sequence_for = make_index_sequence<sizeof...(_types)>;
#endif
		//@}

	} // inline namespace cpp2014;
}

#endif
