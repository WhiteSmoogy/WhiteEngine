/*!	\file mixin.hpp
\ingroup WBase
\brief ������̳еĻ���ӿڡ�
*/

#ifndef WBase_mixin_Hpp
#define WBase_mixin_Hpp 1

#include "tuple.hpp"
#include "utility.hpp"

namespace white {

	namespace details {
		//@{
		/*!
		\brief �������
		\warning ����ʽ���������Ƿ�Ϊ��̬��ȡ���ڲ�����
		*/
		template<size_t N, class... _tBases>
		class mixin : public _tBases...
		{
		public:
			using tuple_type = std::tuple<_tBases...>;

			wconstfn
				mixin() = default;
			template<typename _tParam,
				wimpl(typename = exclude_self_t<mixin, _tParam>)>
				wconstfn
				mixin(_tParam&& arg)
				: _tBases(wforward(arg))...
			{}
			template<typename _tParam1, typename _tParam2, typename... _tParams>
			wconstfn
				mixin(_tParam1&& arg1, _tParam2&& arg2, _tParams&&... args)
				: mixin(std::forward_as_tuple(wforward(arg1), wforward(arg2),
					wforward(args)...))
			{}
			template<typename... _tParams>
			wconstfn
				mixin(const std::tuple<_tParams...>& tp,
					wimpl(enable_if_t<(sizeof...(_tBases) > 1)>* = {}))
				: mixin(index_sequence_for<_tParams...>(), tp)
			{}
			template<typename... _tParams>
			wconstfn
				mixin(std::tuple<_tParams...>&& tp,
					wimpl(enable_if_t<(sizeof...(_tBases) > 1)>* = {}))
				: mixin(index_sequence_for<_tParams...>(), std::move(tp))
			{}
			template<size_t... _vSeq, typename... _tParams>
			wconstfn
				mixin(index_sequence<_vSeq...>, const std::tuple<_tParams...>& tp)
				: _tBases(wforward(std::get<_vSeq>(tp)))...
			{}
			template<size_t... _vSeq, typename... _tParams>
			wconstfn
				mixin(index_sequence<_vSeq...>, std::tuple<_tParams...>&& tp)
				: _tBases(wforward(std::get<_vSeq>(tp)))...
			{}
			wconstfn
				mixin(const mixin&) = default;
			wconstfn
				mixin(mixin&&) = default;

			tuple_type
				to_tuple() const
			{
				return this->template to_tuple(index_sequence_for<_tBases...>());
			}
			template<size_t... _vSeq>
			tuple_type
				to_tuple(index_sequence<_vSeq...>) const
			{
				return tuple_type(
					static_cast<const tuple_element_t<_vSeq, tuple_type>&>(*this)...);
			}
		};

		template<class... _tBases>
		class mixin<1, _tBases...> : public _tBases...
		{
		public:
			using tuple_type = std::tuple<_tBases...>;

			wconstfn
				mixin() = default;
			template<typename _tParam,
				wimpl(typename = exclude_self_t<mixin, _tParam>)>
				wconstfn
				mixin(_tParam&& arg)
				: _tBases(wforward(arg))...
			{}
			template<typename _tParam1, typename _tParam2, typename... _tParams>
			wconstfn
				mixin(_tParam1&& arg1, _tParam2&& arg2, _tParams&&... args)
				: mixin(std::forward_as_tuple(wforward(arg1), wforward(arg2),
					wforward(args)...))
			{}
			template<typename... _tParams>
			wconstfn
				mixin(const std::tuple<_tParams...>& tp,
					wimpl(enable_if_t<(sizeof...(_tBases) == 1)>* = {}))
				: mixin(std::get<0>(tp))
			{}
			template<typename... _tParams>
			wconstfn
				mixin(std::tuple<_tParams...>&& tp,
					wimpl(enable_if_t<(sizeof...(_tBases) == 1)>* = {}))
				: mixin(std::get<0>(std::move(tp)))
			{}

			wconstfn
				mixin(const mixin&) = default;
			wconstfn
				mixin(mixin&&) = default;

			tuple_type
				to_tuple() const
			{
				return this->template to_tuple(index_sequence_for<_tBases...>());
			}
			template<size_t... _vSeq>
			tuple_type
				to_tuple(index_sequence<_vSeq...>) const
			{
				return tuple_type(
					static_cast<const tuple_element_t<_vSeq, tuple_type>&>(*this)...);
			}
		};
	}

	template<typename... _types>
	using mixin = details::mixin<sizeof...(_types), _types...>;

	/*!
	\ingroup metafunctions
	\brief ��װΪ�����ࡣ
	\note ʹ�� classify_value_t �Է������Ͱ�װΪ boxed_value ʵ����
	*/
	template<typename... _types>
	using wrap_mixin_t = mixin<classify_value_t<_types>...>;
	//@}
}

#endif