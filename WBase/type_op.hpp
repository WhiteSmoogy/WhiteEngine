/*!	\file type_traits.hpp
\ingroup WBase
\brief ISO C++ ���Ͳ�����

�����չ��׼��ͷ <type_traits> ������һЩ���������������ͻ���ϵ�����������
�Լ�����Ԫ�����ʩ��
*/
#ifndef Lbase_type_op_hpp
#define Lbase_type_op_hpp 1

#include "tuple.hpp"// for is_class, std::declval, is_detected, vseq::apply,
//	_t, bool_constant, is_void, is_same, remove_reference_t, and_, cond_t,
//	is_enum, vdefer, underlying_type_t, common_type_t;

namespace white {
	namespace details {
		//! \since build 1.4
		template<typename _type, typename _type2>
		using subscription_t
			= decltype(std::declval<_type>()[std::declval<_type2>()]);

		//! \since build 1.4
		template<typename _type, typename _type2>
		using equality_operator_t
			= decltype(std::declval<_type>() == std::declval<_type2>());

		//! \ingroup binary_type_traits
		//@{
		template<class _type>
		struct has_nonempty_virtual_base
		{
			static_assert(std::is_class<_type>::value,
				"Non-class type found @ white::has_nonempty_virtual_base;");

		private:
			struct A : _type
			{};
			struct B : _type
			{};
			struct C : A, B
			{};

		public:
			static wconstexpr bool value = sizeof(C) < sizeof(A) + sizeof(B);
		};

		template<class _type1, class _type2>
		struct has_common_nonempty_virtual_base
		{
			static_assert(std::is_class<_type1>::value
				&& std::is_class<_type2>::value,
				"Non-class type found @ white::has_common_nonempty_virtual_base;");

		private:
			struct A : virtual _type1
			{};

			struct B : virtual _type2
			{};

#ifdef WB_IMPL_GNUCPP
#	if WB_IMPL_GNUCPP >= 40600
#		pragma GCC diagnostic push
#		pragma GCC diagnostic ignored "-Wextra"
#	else
#		pragma GCC system_header
#	endif
#endif

			struct C : A, B
			{};

#if WB_IMPL_GNUCPP && YB_IMPL_GNUCPP >= 40600
			//#	pragma GCC diagnostic warning "-Wextra"
#	pragma GCC diagnostic pop
#endif
		public:
			static wconstexpr bool value = sizeof(C) < sizeof(A) + sizeof(B);
		};
		//@}

		//! \since build 1.4
		template<typename _type>
		using mem_value_t = decltype(std::declval<_type>().value);

		//! \since build 1.4
		template<typename _type>
		using mem_value_type_t = typename _type::value_type;


		//! \since build 1.4
		//@{
		template<typename _type>
		struct member_target_type_impl
		{
			using type = void;
		};

		template<class _tClass, typename _type>
		struct member_target_type_impl<_type _tClass::*>
		{
			using type = _tClass;
		};
		//@}
	}

	//! \ingroup unary_type_traits
	//@{
	/*!
	\brief �ж� _type �Ƿ��������ָ������Ӧ�õõ����͵ĳ�Ա apply ģ�塣
	\since build 1.4
	*/
	template<class _type, typename... _tParams>
	struct has_mem_apply : is_detected<vseq::apply, _type, _tParams...>
	{};


	/*!
	\brief �ж� _type �Ƿ���� type ���ͳ�Ա��
	\since build 1.4
	*/
	template<class _type>
	struct has_mem_type : is_detected<_t, _type>
	{};


	/*!
	\brief �ж� _type �Ƿ���� value ��Ա��
	\since build 1.4
	*/
	template<class _type>
	struct has_mem_value : is_detected<details::mem_value_t, _type>
	{};


	/*!
	\brief �ж� _type �Ƿ���� value_type ���ͳ�Ա��
	\since build 1.4
	*/
	template<class _type>
	struct has_mem_value_type : is_detected<details::mem_value_type_t, _type>
	{};
	//@}

	//! \ingroup binary_type_traits
	//@{
	/*!
	\brief �ж��Ƿ���ں�ʽ�Ľ��Ϊ�� void ���͵� [] ����������ָ�����͵ı��ʽ��
	\since build 1.4
	*/
	template<typename _type1, typename _type2>
	struct has_subscription
		: bool_constant<is_detected<details::subscription_t, _type1, _type2>::value
		&& !is_void<detected_t<details::subscription_t, _type1, _type2>>::value>
	{};


	/*!
	\brief �ж�ָ�������Ƿ��зǿ�����ࡣ
	\since build 1.4
	*/
	template<class _type>
	struct has_nonempty_virtual_base
		: bool_constant<details::has_nonempty_virtual_base<_type>::value>
	{};
	//@}


	/*!
	\ingroup unary_type_traits
	\brief �ж�ָ���������������Ƿ�ͬ���й����ǿ�����ࡣ
	\since build 1.4
	*/
	template<class _type1, class _type2>
	struct has_common_nonempty_virtual_base
		: bool_constant<!is_same<_type1, _type2>::value
		&& details::has_common_nonempty_virtual_base<_type1, _type2>::value>
	{};

	//! \ingroup transformation_traits
	//@{
	/*!
	\brief �Ƴ����ܱ� cv-qualifier ���ε��������͡�
	\note remove_pointer ���� cv-qualifier ���Ƴ�������Ҫ��Ӧ�汾��
	\since build 1.4
	*/
	//@{
	template<typename _type>
	struct remove_rcv : remove_cv<remove_reference_t<_type>>
	{};

	//! \since build 1.4
	template<typename _type>
	using remove_rcv_t = _t<remove_rcv<_type>>;
	//@}


	/*!
	\brief �Ƴ�ָ����������͡�
	\note ָ��������ܵ� cv-qualifier ���Ρ�
	\since build 1.4
	*/
	template<typename _type>
	struct remove_rp : remove_pointer<remove_reference_t<_type>>
	{};


	/*!
	\brief �Ƴ����ܱ� cv-qualifier ���ε����ú�ָ�����͡�
	\since build 1.4
	*/
	template<typename _type>
	struct remove_rpcv : remove_cv<_t<remove_rp<_type>>>
	{};


	/*!
	\brief ȡ��Աָ������ָ��������͡�
	\since build 1.4
	*/
	template<typename _type>
	using member_target_type_t = _t<details::member_target_type_impl<_type>>;
	//@}


	//! \ingroup metafunctions
	//@{
	/*!
	\brief ȡ�ײ����͡�
	\note ͬ underlying_type_t �����Է�ö�����ͽ��Ϊ�������͡�
	\since build 1.4
	*/
	template<typename _type>
	using underlying_cond_type_t
		= cond_t<is_enum<_type>, vdefer<underlying_type_t, _type>, _type>;

	/*!
	\brief �����жϣ���ʧ��ʹ��Ĭ�����͡�
	\sa detected_or
	\sa detected_or_t
	*/
	//@{
	template<class _tCond, typename _tDefault, template<typename...> class _gOp,
		typename... _tParams>
		using cond_or = cond_t<_tCond, vdefer<_gOp, _tParams...>, _tDefault>;

	template<class _tCond, typename _tDefault, template<typename...> class _gOp,
		typename... _tParams>
		using cond_or_t = _t<cond_or<_tCond, identity<_tDefault>, _gOp, _tParams...>>;
	//@}

	/*!
	\brief ȡ�����ǿ����ͣ�����һ����Ϊ�ǿ�������Ϊ��һ�������������������ƶϡ�
	\since build 1.4
	*/
	template<typename _type, typename... _types>
	using common_nonvoid_t = _t<cond_t<is_void<_type>,
		vdefer<common_type_t, _types...>, identity<_type >> >;

	/*!
	\brief ȡ�����ײ����͡�
	\since build 1.4
	*/
	template<typename... _types>
	using common_underlying_t = common_type_t<underlying_cond_type_t<_types>...>;


	/*!	\defgroup tags Tags
	\ingroup meta
	\brief ��ǡ�
	\note ���������ͻ�Ԫ���͡�
	\since build 1.4
	*/

	/*!
	\ingroup meta_types
	\brief ��Ȼ����ǡ�
	*/
	//@{
	template<size_t _vN>
	struct n_tag
	{
		using type = n_tag<_vN - 1>;
	};

	template<>
	struct n_tag<0>
	{
		using type = void;
	};
	//@}

	/*!
	\ingroup tags
	\brief ��һ������ǡ�
	*/
	using first_tag = n_tag<0>;

	/*!
	\ingroup tags
	\brief �ڶ�������ǡ�
	*/
	using second_tag = n_tag<1>;
}

#endif