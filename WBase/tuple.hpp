/*!	\file tuple.hpp
\ingroup WBase
\brief Ԫ�����ͺͲ�����
*/

#ifndef WBase_tuple_hpp
#define WBase_tuple_hpp 1

#include "integer_sequence.hpp" // for "integer_sequence.hpp", vseq::defer,
//	index_sequence;
#include <tuple>

namespace white
{
	inline namespace cpp2014
	{
		/*!
		\ingroup transformation_traits
		\since build 1.4
		*/
		//@{
#if __cpp_lib_tuple_element_t >= 201402 || __cplusplus > 201103L
		using std::tuple_element_t;
#else
		template<size_t _vIdx, typename _type>
		using tuple_element_t = _t<std::tuple_element<_vIdx, _type>>;
#endif
		//@}

	} // inline namespace cpp2014;

	  //! \since build 589
	  //@{
	namespace vseq
	{

		template<typename... _types>
		struct seq_size<std::tuple<_types...>>
			: std::tuple_size<std::tuple<_types...>>::type
		{};


		template<typename... _types, size_t... _vIdxSeq>
		struct project<std::tuple<_types...>, index_sequence<_vIdxSeq...>>
		{
		private:
			//! \since build 1.4
			using tuple_type = std::tuple<_types...>;

		public:
			using type = std::tuple<tuple_element_t<_vIdxSeq, tuple_type>...>;
		};


		template<typename _tInt, _tInt... _vSeq1, _tInt... _vSeq2>
		struct vec_add<std::tuple<integral_constant<_tInt, _vSeq1>...>,
			std::tuple<integral_constant<_tInt, _vSeq2>... >>
		{
			using type = std::tuple<integral_constant<_tInt, _vSeq1 + _vSeq2>...>;
		};


		template<typename _tInt, _tInt... _vSeq1, _tInt... _vSeq2>
		struct vec_subtract<std::tuple<integral_constant<_tInt, _vSeq1>...>,
			std::tuple<integral_constant<_tInt, _vSeq2>... >>
		{
			using type = std::tuple<integral_constant<_tInt, _vSeq1 - _vSeq2>...>;
		};

	} // namespace vseq;
	  //@}


	  /*!
	  \note ʹ�� ADL get ��
	  \see WG21 N4606 20.5.2.5[tuple.apply]/2 ��
	  \see WG21 P0209R2 ��
	  */
	  //@{
	namespace details
	{

		template<typename _type, class _tTuple, size_t... _vIdx>
		wconstfn _type
			make_from_tuple_impl(_tTuple&& t, index_sequence<_vIdx...>)
		{
			using std::get;

			return _type(get<_vIdx>(wforward(t))...);
		}

	} // namespace details;

	  //! \brief ��Ԫ�鹹��ָ�����͵�ֵ��
	template<typename _type, class _tTuple>
	wconstfn _type
		make_from_tuple(_tTuple&& t)
	{
		return details::make_from_tuple_impl<_type>(wforward(t),
			make_index_sequence<std::tuple_size<decay_t<_tTuple>>::value>());
	}
	//@}


	  /*!
	  \see WG21 P0088R0 ��
	  */
	  //@{
	  //! \brief ��ʾû���ҵ���������
	static wconstexpr const auto tuple_not_found(size_t(-1));

	namespace details
	{

		template<class _tSize, class _tIndex>
		using index_conv = integral_constant<size_t,
			_tSize::value == _tIndex::value ? tuple_not_found : _tIndex::value>;

	} // namespace details;

	  /*!
	  \ingroup metafunctions
	  \brief �������Ͷ�Ӧ������λ�ã�����������Ϊ tuple_not_found ��
	  \sa tuple_not_found
	  */
	  //@{
	template<typename, class>
	struct tuple_find;

	template<typename _type, class _tTuple>
	struct tuple_find<_type, const _tTuple> : tuple_find<_type, _tTuple>
	{};

	template<typename _type, class _tTuple>
	struct tuple_find<_type, volatile _tTuple> : tuple_find<_type, _tTuple>
	{};

	template<typename _type, class _tTuple>
	struct tuple_find<_type, const volatile _tTuple> : tuple_find<_type, _tTuple>
	{};

	template<typename _type, typename... _types>
	struct tuple_find<_type, std::tuple<_types...>>
		: details::index_conv<vseq::seq_size<std::tuple<_types...>>,
		vseq::find<std::tuple<_types...>, _type >>
	{};

	template<typename _type, typename _type1, typename _type2>
	struct tuple_find<_type, std::pair<_type1, _type2>>
		: tuple_find<_type, std::tuple<_type1, _type2>>
	{};
	//@}
	//@}
}

#endif