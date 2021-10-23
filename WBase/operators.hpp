/*!	\file operators.hpp
\ingroup WBase
\brief ���ز�������
\see http://www.boost.org/doc/libs/1_60_0/boost/operators.hpp ��
\see https://github.com/taocpp/operators ��

�÷�ͬ Boost.Operators ���������������ֺ�׺�Ľӿڡ�
*/

#ifndef WBase_Operators_hpp
#define WBase_Operators_hpp 1

#include "addressof.hpp"// for "type_traits.hpp", _t, true_type, empty_base,
//	white::constfn_addressof;
#include "integer_sequence.hpp "// for index_sequence, vseq::defer_apply_t,
//	vseq::_a, vseq::fold_t;

#ifdef WB_IMPL_MSCPP
#pragma warning(disable:4503)
#endif

namespace white
{
	namespace deprecated {

		/*!
		\brief �������Ʋ��Ҳ�������Ϊ ADL �ı߽硣
		\note ����ģ�岻ָ��Ϊ��ģ������ģ�塣
		\since build 1.4
		*/
		namespace dep_ops
		{

			//! \since build 1.4
			using no_constfn = wimpl(false_type);
			//! \since build 1.4
			using use_constfn = wimpl(true_type);

		} // namespace dep_ops;

#define WB_Impl_Operators_DeOpt class _tOpt = wimpl(use_constfn)
#define WB_Impl_Operators_H_n(_args) template<class _type, _args>
#define WB_Impl_Operators_H1 \
	WB_Impl_Operators_H_n(WB_Impl_Operators_DeOpt)
#define WB_Impl_Operators_H2 \
	WB_Impl_Operators_H_n(typename _type2 WPP_Comma WB_Impl_Operators_DeOpt)
#define WB_Impl_Operators_H2_de \
	WB_Impl_Operators_H_n(typename _type2 = _type WPP_Comma \
		WB_Impl_Operators_DeOpt)
#define WB_Impl_Operators_H2_Alias(_name, ...) \
	WB_Impl_Operators_H2 \
	wimpl(using) _name = __VA_ARGS__;
#define WB_Impl_Operators_H2_Alias_de(_name, ...) \
	WB_Impl_Operators_H2_de \
	wimpl(using) _name = __VA_ARGS__;
#define WB_Impl_Operators_H3 \
	WB_Impl_Operators_H_n(typename _type2 WPP_Comma typename _type3 \
		WPP_Comma WB_Impl_Operators_DeOpt)

		namespace deprecated_details
		{

			//! \since build 1.4
			//@{
			template<typename _type = void>
			struct op_idt
			{
				using type = _type;
			};

			//! \since build 1.4
			template<>
			struct op_idt<void>;

			template<typename _type = void>
			using op_idt_t = _t<op_idt<_type>>;
			//@}


			//! \since build 1.4
			//@{
			template<class... _types>
			struct ebases : _types...
			{};

			template<size_t _vN, class _type, typename _type2, class _tOpt> \
				struct bin_ops;

			template<class, typename, class, class>
			struct ops_seq;

			template<class _type, typename _type2, class _tOpt, size_t... _vSeq>
			struct ops_seq<_type, _type2, _tOpt, index_sequence<_vSeq...>>
			{
				using type = ebases<bin_ops<_vSeq, _type, _type2, _tOpt>...>;
			};
			//@}


#define WB_Impl_Operators_f(_c, _op, _tRet, _expr, ...) \
	friend _c WB_ATTR(always_inline) _tRet \
	operator _op(__VA_ARGS__) wnoexcept(noexcept(_expr)) \
	{ \
		return (_expr); \
	}
#if WB_IMPL_GNUCPP && WB_IMPL_GNUCPP < 50000
#	define WB_Impl_Operators_bin_ts(_n, _f, _c, _opt, ...) \
	WB_Impl_Operators_H_n(typename _type2) \
	struct bin_ops<_n, _type, _type2, dep_ops::_opt> \
	{ \
		_f(_c, __VA_ARGS__) \
	};
#else
#	define WB_Impl_Operators_bin_ts(_n, _f, _c, _opt, ...) \
	WB_Impl_Operators_H_n(typename _type2) \
	struct bin_ops<_n, _type, _type2, dep_ops::_opt> \
	{ \
		template<wimpl(typename = void)> \
		_f(_c, __VA_ARGS__) \
	};
	//MSCPP has __VA_ARGS__ expack bug
#ifdef WB_IMPL_MSCPP
#	define WB_Impl_Operators_bin_ts_four(_n, _f, _c, _opt, _op, _expr, _ptp, _ptp2) \
	WB_Impl_Operators_H_n(typename _type2) \
	struct bin_ops<_n, _type, _type2, dep_ops::_opt> \
	{ \
		template<wimpl(typename = void)> \
		_f(_c,_op, _expr, _ptp, _ptp2) \
	};
#endif
#endif
#define WB_Impl_Operators_bin_spec(_n, _f, ...) \
	WB_Impl_Operators_bin_ts(_n, _f, inline, no_constfn, __VA_ARGS__) \
	WB_Impl_Operators_bin_ts(_n, _f, wconstfn, use_constfn, __VA_ARGS__)
		// NOTE: The trunk libstdc++ std::experimental::string_view comparison should
		//	depend on the same technique.
		// TODO: See https://gcc.gnu.org/bugzilla/show_bug.cgi?id=52072. It is strange
		//	to still have this bug. Not fully tested for G++ 5. See also https://gcc.gnu.org/bugzilla/show_bug.cgi?id=67426.
#if true // WB_IMPL_GNUCPP
#	define WB_Impl_Operators_cmpf(_c, _op, _expr, _ptp, _ptp2) \
	WB_Impl_Operators_f(_c, _op, bool, _expr, const _ptp& x, const _ptp2& y)
#else
#	define WB_Impl_Operators_cmpf(_c, _op, _expr, _ptp, _ptp2) \
	WB_Impl_Operators_f(_c, _op, bool, _expr, const _ptp& x, const _ptp2& y) \
	WB_Impl_Operators_f(_c, _op, bool, _expr, const op_idt_t<_ptp>& x, \
		const _ptp2& y) \
	WB_Impl_Operators_f(_c, _op, bool, _expr, const _ptp& x, \
		const op_idt_t<_ptp2>& y)
#endif

		//! \since build 1.4
		//@{
		//MSVC Workaround
#ifdef WB_IMPL_MSCPP
#define WB_Impl_Operators_cmp(_n, _op, _expr, _ptp, _ptp2) \
WB_Impl_Operators_bin_ts_four(_n,WB_Impl_Operators_cmpf, inline, no_constfn, _op, _expr, _ptp, _ptp2) \
	WB_Impl_Operators_bin_ts_four(_n,WB_Impl_Operators_cmpf,wconstfn, use_constfn, _op, _expr, _ptp, _ptp2)
#else
#define WB_Impl_Operators_cmp(_n, _op, _expr, _ptp, _ptp2) \
	WB_Impl_Operators_bin_spec(_n, WB_Impl_Operators_cmpf, _op, _expr, \
		_ptp, _ptp2)
#endif

		// TODO: Add strictly partial order comparison support to reduce duplicate code
		//	between 'less_than_comparable' and 'partially_ordered'.

		// NOTE: Range 0-5 is reserved for %less_than_comparable. Range 2-3 is also
		//	used by %partially_ordered.
			WB_Impl_Operators_cmp(0, <= , !bool(x > y), _type, _type2)
				WB_Impl_Operators_cmp(1, >= , !bool(x < y), _type, _type2)
				WB_Impl_Operators_cmp(2, > , y < x, _type2, _type)
				WB_Impl_Operators_cmp(3, <, y > x, _type2, _type)
				WB_Impl_Operators_cmp(4, <= , !bool(y < x), _type2, _type)
				WB_Impl_Operators_cmp(5, >= , !bool(y > x), _type2, _type)

				// NOTE: Range 6-8 is reserved for %equality_comparable.
				WB_Impl_Operators_cmp(6, == , y == x, _type2, _type)
				WB_Impl_Operators_cmp(7, != , !bool(x == y), _type2, _type)
				WB_Impl_Operators_cmp(8, != , !bool(x == y), _type, _type2)

				// NOTE: Range 9-10 is reserved for %equivalent.
				WB_Impl_Operators_cmp(9, != , !bool(x < y) && !bool(x > y), _type, _type2)
				WB_Impl_Operators_cmp(10, != , !bool(x < y) && !bool(y < x), _type, _type2)

				// NOTE: Range 11-14 is reserved for %partially_ordered.
				WB_Impl_Operators_cmp(11, <= , bool(x < y) || bool(x == y), _type, _type2)
				WB_Impl_Operators_cmp(12, >= , bool(x > y) || bool(x == y), _type, _type2)
				WB_Impl_Operators_cmp(13, <= , bool(y > x) || bool(y == x), _type2, _type)
				WB_Impl_Operators_cmp(14, >= , bool(y < x) || bool(y == x), _type2, _type)

#undef WB_Impl_Operators_cmp
#undef WB_Impl_Operators_cmpf
#undef WB_Impl_Operators_bin_ts_four

				// NOTE: Offset range 0-1 is reserved for general binary operations. Range 2-3
				//	is for commutative operations. Range 4-5 is for non commutative "left"
				//	operations.
				wconstfn size_t
				ops_bin_id(size_t id, size_t off) wnothrow
			{
				return (id + 1) * 16 + off;
			}

			template<size_t _vID, size_t... _vSeq>
			using ops_bin_id_seq = index_sequence<ops_bin_id(_vID, _vSeq)...>;

#ifdef WB_IMPL_MSCPP
#	define WB_Impl_Operators_bin_ts_five(_n, _f, _c, _opt, _op, _type, arg1, arg2, arg3) \
	WB_Impl_Operators_H_n(typename _type2) \
	struct bin_ops<_n, _type, _type2, dep_ops::_opt> \
	{ \
		template<wimpl(typename = void)> \
		_f(_c,_op, _type, arg1, arg2, arg3) \
	};
#define WB_Impl_Operators_bin_tmpl(_id, _off, _op,arg1,arg2,arg3) \
	WB_Impl_Operators_bin_ts_five(ops_bin_id(_id WPP_Comma _off), WB_Impl_Operators_f, inline, no_constfn, _op, _type, arg1, arg2, arg3) \
	WB_Impl_Operators_bin_ts_five(ops_bin_id(_id WPP_Comma _off), WB_Impl_Operators_f, wconstfn, use_constfn, _op, _type, arg1, arg2, arg3)
#else
#define WB_Impl_Operators_bin_tmpl(_id, _off, _op, ...) \
	WB_Impl_Operators_bin_spec(ops_bin_id(_id WPP_Comma _off), \
		WB_Impl_Operators_f, _op, _type, __VA_ARGS__)
#endif
#define WB_Impl_Operators_bin(_id, _op) \
	WB_Impl_Operators_bin_tmpl(_id, 0, _op, x _op##= y, _type x, \
		const _type2& y) \
	WB_Impl_Operators_bin_tmpl(_id, 1, _op, x _op##= std::move(y), _type x, \
		_type2&& y)
#define WB_Impl_Operators_con(_id, _op) \
	WB_Impl_Operators_bin(_id, _op) \
	WB_Impl_Operators_bin_tmpl(_id, 2, _op, y _op##= x, const _type2& x, \
		_type y) \
	WB_Impl_Operators_bin_tmpl(_id, 3, _op, y _op##= std::move(x), _type2&& x, \
		_type y)
#define WB_Impl_Operators_left(_id, _op) \
	WB_Impl_Operators_bin(_id, _op) \
	WB_Impl_Operators_bin_tmpl(_id, 4, _op, _type(x) _op##= y, \
		const _type2& x, const _type& y) \
	WB_Impl_Operators_bin_tmpl(_id, 5, _op, \
		_type(x) _op##= std::move(y), const _type2& x, _type&& y)

			WB_Impl_Operators_con(0, +)
				WB_Impl_Operators_left(1, -)
				WB_Impl_Operators_con(2, *)
				WB_Impl_Operators_left(3, / )
				WB_Impl_Operators_left(4, %)
				WB_Impl_Operators_con(5, ^)
				WB_Impl_Operators_con(6, &)
				WB_Impl_Operators_con(7, | )
				WB_Impl_Operators_bin(8, << )
				WB_Impl_Operators_bin(9, >> )

#undef WB_Impl_Operators_left
#undef WB_Impl_Operators_con
#undef WB_Impl_Operators_bin
#undef WB_Impl_Operators_bin_tmpl
#undef WB_Impl_Operators_bin_ts_five

#undef WB_Impl_Operators_bin_spec
#undef WB_Impl_Operators_bin_ts
#undef WB_Impl_Operators_f

				template<class _type, typename _type2, class _tOpt,
				template<typename...> class... _gOps>
			using flat_ops = vseq::defer_t<ebases, vseq::fold_t<vseq::_a<vseq::concat_t>,
				empty_base<>,
				empty_base<vseq::defer_t<empty_base, _gOps<_type, _type2, _tOpt>>... >> >;
			//@}

		} // namespace deprecated_details;


		namespace dep_ops
		{

			//! \since build 1.4
			//@{
#define WB_Impl_Operators_Compare(_name, _bseq, _bseq_s) \
	WB_Impl_Operators_H2_Alias_de(_name, _t<deprecated_details::ops_seq<_type, _type2, \
		_tOpt, cond_t<is_same<_type, _type2>, index_sequence<_bseq_s>, \
		index_sequence<_bseq>>>>)

			WB_Impl_Operators_Compare(less_than_comparable, 0 WPP_Comma 1 WPP_Comma 2
				WPP_Comma 3 WPP_Comma 4 WPP_Comma 5, 2 WPP_Comma 4 WPP_Comma 1)


				WB_Impl_Operators_Compare(equality_comparable, 6 WPP_Comma 7 WPP_Comma 8,
					7)


				WB_Impl_Operators_Compare(equivalent, 9, 10)


				WB_Impl_Operators_Compare(partially_ordered, 11 WPP_Comma 12 WPP_Comma 2
					WPP_Comma 3 WPP_Comma 13 WPP_Comma 14, 2 WPP_Comma 11 WPP_Comma 14)

#undef WB_Impl_Operators_Compare


#define WB_Impl_Operators_Binary_Tmpl(_id, _name, _bseq) \
	WB_Impl_Operators_H2_Alias_de(_name, _t<deprecated_details::ops_seq<_type, _type2, \
		_tOpt, deprecated_details::ops_bin_id_seq<_id, _bseq>>>)
#define WB_Impl_Operators_Binary(_id, _name) \
	WB_Impl_Operators_Binary_Tmpl(_id, _name, 0 WPP_Comma 1)
#define WB_Impl_Operators_Commutative(_id, _name) \
	WB_Impl_Operators_H2_Alias_de(_name, _t< \
		deprecated_details::ops_seq<_type, _type2, _tOpt, cond_t<is_same<_type, _type2>, \
		deprecated_details::ops_bin_id_seq<_id, 0 WPP_Comma 1>, \
		deprecated_details::ops_bin_id_seq<_id, 0 WPP_Comma 1 WPP_Comma 2 WPP_Comma 3>>>>)
#define WB_Impl_Operators_Left(_id, _name) \
	WB_Impl_Operators_H2_Alias_de(_name, _t<deprecated_details::ops_seq<_type, _type2, \
		_tOpt, deprecated_details::ops_bin_id_seq<_id, 0 WPP_Comma 1>>>) \
	WB_Impl_Operators_H2_Alias_de(_name##_left, _t<deprecated_details::ops_seq<_type, \
		_type2, _tOpt, deprecated_details::ops_bin_id_seq<_id, 4 WPP_Comma 5>>>)

				WB_Impl_Operators_Commutative(0, addable)


				WB_Impl_Operators_Left(1, subtractable)


				WB_Impl_Operators_Commutative(2, multipliable)


				WB_Impl_Operators_Left(3, dividable)


				WB_Impl_Operators_Left(4, modable)


				WB_Impl_Operators_Binary(5, left_shiftable)


				WB_Impl_Operators_Binary(6, right_shiftable)


				WB_Impl_Operators_Commutative(7, xorable)


				WB_Impl_Operators_Commutative(8, andable)


				WB_Impl_Operators_Commutative(9, orable)

#undef WB_Impl_Operators_Binary_Tmpl
#undef WB_Impl_Operators_Left
#undef WB_Impl_Operators_Commutative
#undef WB_Impl_Operators_Binary


				WB_Impl_Operators_H1
				wimpl(struct) incrementable
			{
				//! \since build 1.4
				friend _type
					operator++(_type& x, int) wnoexcept(noexcept(_type(x)) && noexcept(++x)
						&& noexcept(_type(std::declval<_type>())))
				{
					_type t(x);

					++x;
					return t;
				}
			};

			WB_Impl_Operators_H1
				wimpl(struct) decrementable
			{
				//! \since build 1.4
				friend _type
					operator--(_type& x, int) wnoexcept(noexcept(_type(x)) && noexcept(--x)
						&& noexcept(_type(std::declval<_type>())))
				{
					_type t(x);

					++x;
					return t;
				}
			};

			WB_Impl_Operators_H2
				wimpl(struct) dereferenceable
			{
				// TODO: Add non-const overloaded version? SFINAE?
				// TODO: Use '_tOpt'.

				wconstfn identity_t<decltype(white::constfn_addressof(std::declval<const _type2&>()))>
					operator->() const
#ifndef WB_IMPL_MSCPP
					wnoexcept_spec(*std::declval<const _type&>())
#endif
				{
					return white::constfn_addressof(*static_cast<const _type&>(*this));
				}
			};

			WB_Impl_Operators_H3
				wimpl(struct) indexable
			{
				// TODO: Add non-const overloaded version? SFINAE?
				// TODO: Use '_tOpt'.
				wconstfn _type3
					operator[](_type2 n) const
					wnoexcept_spec(*(std::declval<const _type&>() + n))
				{
					return *(static_cast<const _type&>(*this) + n);
				}
			};


#define WB_Impl_Operators_FlatAlias2_de(_name, ...) \
	WB_Impl_Operators_H2_Alias_de(_name, deprecated_details::flat_ops<_type, _type2, \
		_tOpt, __VA_ARGS__>)

			WB_Impl_Operators_FlatAlias2_de(totally_ordered, less_than_comparable,
				equality_comparable)


				WB_Impl_Operators_FlatAlias2_de(additive, addable, subtractable)


				WB_Impl_Operators_FlatAlias2_de(multiplicative, multipliable, dividable)


				WB_Impl_Operators_FlatAlias2_de(integer_multiplicative, multiplicative, modable)


				WB_Impl_Operators_FlatAlias2_de(arithmetic, additive, multiplicative)


				WB_Impl_Operators_FlatAlias2_de(integer_arithmetic, additive,
					integer_multiplicative)


				WB_Impl_Operators_FlatAlias2_de(bitwise, xorable, andable, orable)


				WB_Impl_Operators_H1
				wimpl(using) unit_steppable
				= deprecated_details::ebases<incrementable<_type>, decrementable<_type>>;


			WB_Impl_Operators_FlatAlias2_de(shiftable, left_shiftable, right_shiftable)


				WB_Impl_Operators_FlatAlias2_de(ring_operators, additive, subtractable_left,
					multipliable)


				WB_Impl_Operators_FlatAlias2_de(ordered_ring_operators, ring_operators,
					totally_ordered)


				WB_Impl_Operators_FlatAlias2_de(field_operators, ring_operators, dividable,
					dividable_left)


				WB_Impl_Operators_FlatAlias2_de(ordered_field_operators, field_operators,
					totally_ordered)


				WB_Impl_Operators_FlatAlias2_de(euclidean_ring_operators, ring_operators,
					dividable, dividable_left, modable, modable_left)


				WB_Impl_Operators_FlatAlias2_de(ordered_euclidean_ring_operators,
					totally_ordered, euclidean_ring_operators)


				WB_Impl_Operators_H2_Alias(input_iteratable, deprecated_details::ebases<
					equality_comparable<_type, _type, _tOpt>, incrementable<_type>,
					dereferenceable<_type, _type2, _tOpt >>)


				WB_Impl_Operators_H2_Alias(output_iteratable,
					deprecated_details::ebases<incrementable<_type>>)


				WB_Impl_Operators_H2_Alias(forward_iteratable,
					deprecated_details::ebases<input_iteratable<_type, _type2, _tOpt>>)


				WB_Impl_Operators_H2_Alias(bidirectional_iteratable, deprecated_details::ebases<
					forward_iteratable<_type, _type2, _tOpt>, decrementable<_type >>)


				WB_Impl_Operators_H3
				wimpl(using) random_access_iteratable
				= deprecated_details::ebases<bidirectional_iteratable<_type, _type3, _tOpt>,
				less_than_comparable<_type, _type, _tOpt>,
				additive<_type, _type2, _tOpt>, indexable<_type, _type2, _type, _tOpt >>;

			/*!
			\note ����ģ������������ɵĲ�������������
			\note ��ǰ��Ԫ����Ԫ��������ʹ�� wconstfn ��
			\todo ʵ�ֵ���ѡ�
			*/
			WB_Impl_Operators_FlatAlias2_de(operators, totally_ordered, integer_arithmetic,
				bitwise)

#undef WB_Impl_Operators_FlatAlias2_de
				//@}

		} // namespace dep_ops;

		  //! \since build 1.4
		using namespace dep_ops;

#undef WB_Impl_Operators_H3
#undef WB_Impl_Operators_H2_Alias_de
#undef WB_Impl_Operators_H2_Alias
#undef WB_Impl_Operators_H2
#undef WB_Impl_Operators_H1
#undef WB_Impl_Operators_H_n
#undef WB_Impl_Operators_DeOpt
	}

} // namespace white;

namespace white
{
	namespace deprecated_v2 {
		/*!
		\brief �������Ʋ��Ҳ�������Ϊ ADL �ı߽硣
		\note ����ģ�岻ָ��Ϊ��ģ������ģ�塣
		\since build 1.4
		*/
		namespace dep_ops
		{
			//! \since build 1.4
			using no_constfn = false_type;
			//! \since build 1.4
			using use_constfn = true_type;

		} // namespace dep_ops;

		namespace deprecated_v2_details
		{
			//! \since build 1.4
			//@{
			template<typename _type = void>
			struct op_idt
			{
				using type = _type;
			};

			//! \since build 1.4
			template<>
			struct op_idt<void>;

			template<typename _type = void>
			using op_idt_t = _t<op_idt<_type>>;
			//@}


			//! \since build 1.4
			//@{
			template<class... _types>
			struct ebases : _types...
			{};

			template<size_t _vN, class _type, typename _type2, class _tOpt> \
				struct bin_ops;

			template<class, typename, class, class>
			struct ops_seq;

			template<class _type, typename _type2, class _tOpt, size_t... _vSeq>
			struct ops_seq<_type, _type2, _tOpt, index_sequence<_vSeq...>>
			{
				using type = ebases<bin_ops<_vSeq, _type, _type2, _tOpt>...>;
			};
			//@}


			// NOTE: The trunk libstdc++ std::experimental::string_view comparison should
			//	depend on the same technique.
			// TODO: See https://gcc.gnu.org/bugzilla/show_bug.cgi?id=52072. It is strange
			//	to still have this bug. Not fully tested for G++ 5. See also https://gcc.gnu.org/bugzilla/show_bug.cgi?id=67426.

			//! \since build 1.4
			//@{
			// TODO: Add strictly partial order comparison support to reduce duplicate code
			//	between 'less_than_comparable' and 'partially_ordered'.

			// NOTE: Range 0-5 is reserved for %less_than_comparable. Range 2-3 is also
			//	used by %partially_ordered.
			template<class _type, typename _type2> struct bin_ops<0, _type, _type2, dep_ops::no_constfn> { template<typename = void> friend inline  bool operator <=(const _type& x, const _type2& y) wnoexcept(wnoexcept(!bool(x > y))) { return (!bool(x > y)); } }; template<class _type, typename _type2> struct bin_ops<0, _type, _type2, dep_ops::use_constfn> { template<typename = void> friend wconstfn  bool operator <=(const _type& x, const _type2& y) wnoexcept(wnoexcept(!bool(x > y))) { return (!bool(x > y)); } };
			template<class _type, typename _type2> struct bin_ops<1, _type, _type2, dep_ops::no_constfn> { template<typename = void> friend inline  bool operator >=(const _type& x, const _type2& y) wnoexcept(wnoexcept(!bool(x < y))) { return (!bool(x < y)); } }; template<class _type, typename _type2> struct bin_ops<1, _type, _type2, dep_ops::use_constfn> { template<typename = void> friend wconstfn  bool operator >=(const _type& x, const _type2& y) wnoexcept(wnoexcept(!bool(x < y))) { return (!bool(x < y)); } };
			template<class _type, typename _type2> struct bin_ops<2, _type, _type2, dep_ops::no_constfn> { template<typename = void> friend inline  bool operator >(const _type2& x, const _type& y) wnoexcept(wnoexcept(y < x)) { return (y < x); } }; template<class _type, typename _type2> struct bin_ops<2, _type, _type2, dep_ops::use_constfn> { template<typename = void> friend wconstfn  bool operator >(const _type2& x, const _type& y) wnoexcept(wnoexcept(y < x)) { return (y < x); } };
			template<class _type, typename _type2> struct bin_ops<3, _type, _type2, dep_ops::no_constfn> { template<typename = void> friend inline  bool operator <(const _type2& x, const _type& y) wnoexcept(wnoexcept(y > x)) { return (y > x); } }; template<class _type, typename _type2> struct bin_ops<3, _type, _type2, dep_ops::use_constfn> { template<typename = void> friend wconstfn  bool operator <(const _type2& x, const _type& y) wnoexcept(wnoexcept(y > x)) { return (y > x); } };
			template<class _type, typename _type2> struct bin_ops<4, _type, _type2, dep_ops::no_constfn> { template<typename = void> friend inline  bool operator <=(const _type2& x, const _type& y) wnoexcept(wnoexcept(!bool(y < x))) { return (!bool(y < x)); } }; template<class _type, typename _type2> struct bin_ops<4, _type, _type2, dep_ops::use_constfn> { template<typename = void> friend wconstfn  bool operator <=(const _type2& x, const _type& y) wnoexcept(wnoexcept(!bool(y < x))) { return (!bool(y < x)); } };
			template<class _type, typename _type2> struct bin_ops<5, _type, _type2, dep_ops::no_constfn> { template<typename = void> friend inline  bool operator >=(const _type2& x, const _type& y) wnoexcept(wnoexcept(!bool(y > x))) { return (!bool(y > x)); } }; template<class _type, typename _type2> struct bin_ops<5, _type, _type2, dep_ops::use_constfn> { template<typename = void> friend wconstfn  bool operator >=(const _type2& x, const _type& y) wnoexcept(wnoexcept(!bool(y > x))) { return (!bool(y > x)); } };

			// NOTE: Range 6-8 is reserved for %equality_comparable.
			template<class _type, typename _type2> struct bin_ops<6, _type, _type2, dep_ops::no_constfn> { template<typename = void> friend inline  bool operator ==(const _type2& x, const _type& y) wnoexcept(wnoexcept(y == x)) { return (y == x); } }; template<class _type, typename _type2> struct bin_ops<6, _type, _type2, dep_ops::use_constfn> { template<typename = void> friend wconstfn  bool operator ==(const _type2& x, const _type& y) wnoexcept(wnoexcept(y == x)) { return (y == x); } };
			template<class _type, typename _type2> struct bin_ops<7, _type, _type2, dep_ops::no_constfn> { template<typename = void> friend inline  bool operator !=(const _type2& x, const _type& y) wnoexcept(wnoexcept(!bool(x == y))) { return (!bool(x == y)); } }; template<class _type, typename _type2> struct bin_ops<7, _type, _type2, dep_ops::use_constfn> { template<typename = void> friend wconstfn  bool operator !=(const _type2& x, const _type& y) wnoexcept(wnoexcept(!bool(x == y))) { return (!bool(x == y)); } };
			template<class _type, typename _type2> struct bin_ops<8, _type, _type2, dep_ops::no_constfn> { template<typename = void> friend inline  bool operator !=(const _type& x, const _type2& y) wnoexcept(wnoexcept(!bool(x == y))) { return (!bool(x == y)); } }; template<class _type, typename _type2> struct bin_ops<8, _type, _type2, dep_ops::use_constfn> { template<typename = void> friend wconstfn  bool operator !=(const _type& x, const _type2& y) wnoexcept(wnoexcept(!bool(x == y))) { return (!bool(x == y)); } };

			// NOTE: Range 9-10 is reserved for %equivalent.
			template<class _type, typename _type2> struct bin_ops<9, _type, _type2, dep_ops::no_constfn> { template<typename = void> friend inline  bool operator !=(const _type& x, const _type2& y) wnoexcept(wnoexcept(!bool(x < y) && !bool(x > y))) { return (!bool(x < y) && !bool(x > y)); } }; template<class _type, typename _type2> struct bin_ops<9, _type, _type2, dep_ops::use_constfn> { template<typename = void> friend wconstfn  bool operator !=(const _type& x, const _type2& y) wnoexcept(wnoexcept(!bool(x < y) && !bool(x > y))) { return (!bool(x < y) && !bool(x > y)); } };
			template<class _type, typename _type2> struct bin_ops<10, _type, _type2, dep_ops::no_constfn> { template<typename = void> friend inline  bool operator !=(const _type& x, const _type2& y) wnoexcept(wnoexcept(!bool(x < y) && !bool(y < x))) { return (!bool(x < y) && !bool(y < x)); } }; template<class _type, typename _type2> struct bin_ops<10, _type, _type2, dep_ops::use_constfn> { template<typename = void> friend wconstfn  bool operator !=(const _type& x, const _type2& y) wnoexcept(wnoexcept(!bool(x < y) && !bool(y < x))) { return (!bool(x < y) && !bool(y < x)); } };

			// NOTE: Range 11-14 is reserved for %partially_ordered.
			template<class _type, typename _type2> struct bin_ops<11, _type, _type2, dep_ops::no_constfn> { template<typename = void> friend inline  bool operator <=(const _type& x, const _type2& y) wnoexcept(wnoexcept(bool(x < y) || bool(x == y))) { return (bool(x < y) || bool(x == y)); } }; template<class _type, typename _type2> struct bin_ops<11, _type, _type2, dep_ops::use_constfn> { template<typename = void> friend wconstfn  bool operator <=(const _type& x, const _type2& y) wnoexcept(wnoexcept(bool(x < y) || bool(x == y))) { return (bool(x < y) || bool(x == y)); } };
			template<class _type, typename _type2> struct bin_ops<12, _type, _type2, dep_ops::no_constfn> { template<typename = void> friend inline  bool operator >=(const _type& x, const _type2& y) wnoexcept(wnoexcept(bool(x > y) || bool(x == y))) { return (bool(x > y) || bool(x == y)); } }; template<class _type, typename _type2> struct bin_ops<12, _type, _type2, dep_ops::use_constfn> { template<typename = void> friend wconstfn  bool operator >=(const _type& x, const _type2& y) wnoexcept(wnoexcept(bool(x > y) || bool(x == y))) { return (bool(x > y) || bool(x == y)); } };
			template<class _type, typename _type2> struct bin_ops<13, _type, _type2, dep_ops::no_constfn> { template<typename = void> friend inline  bool operator <=(const _type2& x, const _type& y) wnoexcept(wnoexcept(bool(y > x) || bool(y == x))) { return (bool(y > x) || bool(y == x)); } }; template<class _type, typename _type2> struct bin_ops<13, _type, _type2, dep_ops::use_constfn> { template<typename = void> friend wconstfn  bool operator <=(const _type2& x, const _type& y) wnoexcept(wnoexcept(bool(y > x) || bool(y == x))) { return (bool(y > x) || bool(y == x)); } };
			template<class _type, typename _type2> struct bin_ops<14, _type, _type2, dep_ops::no_constfn> { template<typename = void> friend inline  bool operator >=(const _type2& x, const _type& y) wnoexcept(wnoexcept(bool(y < x) || bool(y == x))) { return (bool(y < x) || bool(y == x)); } }; template<class _type, typename _type2> struct bin_ops<14, _type, _type2, dep_ops::use_constfn> { template<typename = void> friend wconstfn  bool operator >=(const _type2& x, const _type& y) wnoexcept(wnoexcept(bool(y < x) || bool(y == x))) { return (bool(y < x) || bool(y == x)); } };

			// NOTE: Offset range 0-1 is reserved for general binary operations. Range 2-3
			//	is for commutative operations. Range 4-5 is for non commutative "left"
			//	operations.
			wconstfn size_t
				ops_bin_id(size_t id, size_t off) wnoexcept
			{
				return (id + 1) * 16 + off;
			}

			template<size_t _vID, size_t... _vSeq>
			using ops_bin_id_seq = index_sequence<ops_bin_id(_vID, _vSeq)...>;


			template<class _type, typename _type2>
			struct bin_ops<ops_bin_id(0, 0), _type, _type2, dep_ops::no_constfn>
			{
				template<typename = void>
				friend inline  _type operator +(_type x, const _type2& y) wnoexcept(wnoexcept(x += y))
				{
					return (x += y);
				}
			};

			template<class _type, typename _type2>
			struct bin_ops<ops_bin_id(0, 0), _type, _type2, dep_ops::use_constfn> {
				template<typename = void> friend wconstfn  _type operator +(_type x, const _type2& y) wnoexcept(wnoexcept(x += y))
				{
					return (x += y);
				}
			};
			template<class _type, typename _type2>
			struct bin_ops<ops_bin_id(0, 1), _type, _type2, dep_ops::no_constfn> {
				template<typename = void> friend inline  _type operator +(_type x, _type2&& y) wnoexcept(wnoexcept(x += std::move(y)))
				{
					return (x += std::move(y));
				}
			};
			template<class _type, typename _type2>
			struct bin_ops<ops_bin_id(0, 1), _type, _type2, dep_ops::use_constfn> { template<typename = void> friend wconstfn  _type operator +(_type x, _type2&& y) wnoexcept(wnoexcept(x += std::move(y))) { return (x += std::move(y)); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(0, 2), _type, _type2, dep_ops::no_constfn> { template<typename = void> friend inline  _type operator +(const _type2& x, _type y) wnoexcept(wnoexcept(y += x)) { return (y += x); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(0, 2), _type, _type2, dep_ops::use_constfn> { template<typename = void> friend wconstfn  _type operator +(const _type2& x, _type y) wnoexcept(wnoexcept(y += x)) { return (y += x); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(0, 3), _type, _type2, dep_ops::no_constfn> { template<typename = void> friend inline  _type operator +(_type2&& x, _type y) wnoexcept(wnoexcept(y += std::move(x))) { return (y += std::move(x)); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(0, 3), _type, _type2, dep_ops::use_constfn> { template<typename = void> friend wconstfn  _type operator +(_type2&& x, _type y) wnoexcept(wnoexcept(y += std::move(x))) { return (y += std::move(x)); } };
			template<class _type, typename _type2>
			struct bin_ops<ops_bin_id(1, 0), _type, _type2, dep_ops::no_constfn> { template<typename = void> friend inline  _type operator -(_type x, const _type2& y) wnoexcept(wnoexcept(x -= y)) { return (x -= y); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(1, 0), _type, _type2, dep_ops::use_constfn> { template<typename = void> friend wconstfn  _type operator -(_type x, const _type2& y) wnoexcept(wnoexcept(x -= y)) { return (x -= y); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(1, 1), _type, _type2, dep_ops::no_constfn> { template<typename = void> friend inline  _type operator -(_type x, _type2&& y) wnoexcept(wnoexcept(x -= std::move(y))) { return (x -= std::move(y)); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(1, 1), _type, _type2, dep_ops::use_constfn> { template<typename = void> friend wconstfn  _type operator -(_type x, _type2&& y) wnoexcept(wnoexcept(x -= std::move(y))) { return (x -= std::move(y)); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(1, 4), _type, _type2, dep_ops::no_constfn> { template<typename = void> friend inline  _type operator -(const _type2& x, const _type& y) wnoexcept(wnoexcept(_type(x) -= y)) { return (_type(x) -= y); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(1, 4), _type, _type2, dep_ops::use_constfn> { template<typename = void> friend wconstfn  _type operator -(const _type2& x, const _type& y) wnoexcept(wnoexcept(_type(x) -= y)) { return (_type(x) -= y); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(1, 5), _type, _type2, dep_ops::no_constfn> { template<typename = void> friend inline  _type operator -(const _type2& x, _type&& y) wnoexcept(wnoexcept(_type(x) -= std::move(y))) { return (_type(x) -= std::move(y)); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(1, 5), _type, _type2, dep_ops::use_constfn> { template<typename = void> friend wconstfn  _type operator -(const _type2& x, _type&& y) wnoexcept(wnoexcept(_type(x) -= std::move(y))) { return (_type(x) -= std::move(y)); } };
			template<class _type, typename _type2>
			struct bin_ops<ops_bin_id(2, 0), _type, _type2, dep_ops::no_constfn> { template<typename = void> friend inline  _type operator *(_type x, const _type2& y) wnoexcept(wnoexcept(x *= y)) { return (x *= y); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(2, 0), _type, _type2, dep_ops::use_constfn> { template<typename = void> friend wconstfn  _type operator *(_type x, const _type2& y) wnoexcept(wnoexcept(x *= y)) { return (x *= y); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(2, 1), _type, _type2, dep_ops::no_constfn> { template<typename = void> friend inline  _type operator *(_type x, _type2&& y) wnoexcept(wnoexcept(x *= std::move(y))) { return (x *= std::move(y)); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(2, 1), _type, _type2, dep_ops::use_constfn> { template<typename = void> friend wconstfn  _type operator *(_type x, _type2&& y) wnoexcept(wnoexcept(x *= std::move(y))) { return (x *= std::move(y)); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(2, 2), _type, _type2, dep_ops::no_constfn> { template<typename = void> friend inline  _type operator *(const _type2& x, _type y) wnoexcept(wnoexcept(y *= x)) { return (y *= x); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(2, 2), _type, _type2, dep_ops::use_constfn> { template<typename = void> friend wconstfn  _type operator *(const _type2& x, _type y) wnoexcept(wnoexcept(y *= x)) { return (y *= x); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(2, 3), _type, _type2, dep_ops::no_constfn> { template<typename = void> friend inline  _type operator *(_type2&& x, _type y) wnoexcept(wnoexcept(y *= std::move(x))) { return (y *= std::move(x)); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(2, 3), _type, _type2, dep_ops::use_constfn> { template<typename = void> friend wconstfn  _type operator *(_type2&& x, _type y) wnoexcept(wnoexcept(y *= std::move(x))) { return (y *= std::move(x)); } };
			template<class _type, typename _type2>
			struct bin_ops<ops_bin_id(3, 0), _type, _type2, dep_ops::no_constfn> { template<typename = void> friend inline  _type operator /(_type x, const _type2& y) wnoexcept(wnoexcept(x /= y)) { return (x /= y); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(3, 0), _type, _type2, dep_ops::use_constfn> { template<typename = void> friend wconstfn  _type operator /(_type x, const _type2& y) wnoexcept(wnoexcept(x /= y)) { return (x /= y); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(3, 1), _type, _type2, dep_ops::no_constfn> { template<typename = void> friend inline  _type operator /(_type x, _type2&& y) wnoexcept(wnoexcept(x /= std::move(y))) { return (x /= std::move(y)); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(3, 1), _type, _type2, dep_ops::use_constfn> { template<typename = void> friend wconstfn  _type operator /(_type x, _type2&& y) wnoexcept(wnoexcept(x /= std::move(y))) { return (x /= std::move(y)); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(3, 4), _type, _type2, dep_ops::no_constfn> { template<typename = void> friend inline  _type operator /(const _type2& x, const _type& y) wnoexcept(wnoexcept(_type(x) /= y)) { return (_type(x) /= y); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(3, 4), _type, _type2, dep_ops::use_constfn> { template<typename = void> friend wconstfn  _type operator /(const _type2& x, const _type& y) wnoexcept(wnoexcept(_type(x) /= y)) { return (_type(x) /= y); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(3, 5), _type, _type2, dep_ops::no_constfn> { template<typename = void> friend inline  _type operator /(const _type2& x, _type&& y) wnoexcept(wnoexcept(_type(x) /= std::move(y))) { return (_type(x) /= std::move(y)); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(3, 5), _type, _type2, dep_ops::use_constfn> { template<typename = void> friend wconstfn  _type operator /(const _type2& x, _type&& y) wnoexcept(wnoexcept(_type(x) /= std::move(y))) { return (_type(x) /= std::move(y)); } };
			template<class _type, typename _type2>
			struct bin_ops<ops_bin_id(4, 0), _type, _type2, dep_ops::no_constfn> { template<typename = void> friend inline  _type operator %(_type x, const _type2& y) wnoexcept(wnoexcept(x %= y)) { return (x %= y); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(4, 0), _type, _type2, dep_ops::use_constfn> { template<typename = void> friend wconstfn  _type operator %(_type x, const _type2& y) wnoexcept(wnoexcept(x %= y)) { return (x %= y); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(4, 1), _type, _type2, dep_ops::no_constfn> { template<typename = void> friend inline  _type operator %(_type x, _type2&& y) wnoexcept(wnoexcept(x %= std::move(y))) { return (x %= std::move(y)); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(4, 1), _type, _type2, dep_ops::use_constfn> { template<typename = void> friend wconstfn  _type operator %(_type x, _type2&& y) wnoexcept(wnoexcept(x %= std::move(y))) { return (x %= std::move(y)); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(4, 4), _type, _type2, dep_ops::no_constfn> { template<typename = void> friend inline  _type operator %(const _type2& x, const _type& y) wnoexcept(wnoexcept(_type(x) %= y)) { return (_type(x) %= y); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(4, 4), _type, _type2, dep_ops::use_constfn> { template<typename = void> friend wconstfn  _type operator %(const _type2& x, const _type& y) wnoexcept(wnoexcept(_type(x) %= y)) { return (_type(x) %= y); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(4, 5), _type, _type2, dep_ops::no_constfn> { template<typename = void> friend inline  _type operator %(const _type2& x, _type&& y) wnoexcept(wnoexcept(_type(x) %= std::move(y))) { return (_type(x) %= std::move(y)); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(4, 5), _type, _type2, dep_ops::use_constfn> { template<typename = void> friend wconstfn  _type operator %(const _type2& x, _type&& y) wnoexcept(wnoexcept(_type(x) %= std::move(y))) { return (_type(x) %= std::move(y)); } };
			template<class _type, typename _type2>
			struct bin_ops<ops_bin_id(5, 0), _type, _type2, dep_ops::no_constfn> { template<typename = void> friend inline  _type operator ^(_type x, const _type2& y) wnoexcept(wnoexcept(x ^= y)) { return (x ^= y); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(5, 0), _type, _type2, dep_ops::use_constfn> { template<typename = void> friend wconstfn  _type operator ^(_type x, const _type2& y) wnoexcept(wnoexcept(x ^= y)) { return (x ^= y); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(5, 1), _type, _type2, dep_ops::no_constfn> { template<typename = void> friend inline  _type operator ^(_type x, _type2&& y) wnoexcept(wnoexcept(x ^= std::move(y))) { return (x ^= std::move(y)); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(5, 1), _type, _type2, dep_ops::use_constfn> { template<typename = void> friend wconstfn  _type operator ^(_type x, _type2&& y) wnoexcept(wnoexcept(x ^= std::move(y))) { return (x ^= std::move(y)); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(5, 2), _type, _type2, dep_ops::no_constfn> { template<typename = void> friend inline  _type operator ^(const _type2& x, _type y) wnoexcept(wnoexcept(y ^= x)) { return (y ^= x); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(5, 2), _type, _type2, dep_ops::use_constfn> { template<typename = void> friend wconstfn  _type operator ^(const _type2& x, _type y) wnoexcept(wnoexcept(y ^= x)) { return (y ^= x); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(5, 3), _type, _type2, dep_ops::no_constfn> { template<typename = void> friend inline  _type operator ^(_type2&& x, _type y) wnoexcept(wnoexcept(y ^= std::move(x))) { return (y ^= std::move(x)); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(5, 3), _type, _type2, dep_ops::use_constfn> { template<typename = void> friend wconstfn  _type operator ^(_type2&& x, _type y) wnoexcept(wnoexcept(y ^= std::move(x))) { return (y ^= std::move(x)); } };
			template<class _type, typename _type2>
			struct bin_ops<ops_bin_id(6, 0), _type, _type2, dep_ops::no_constfn> { template<typename = void> friend inline  _type operator &(_type x, const _type2& y) wnoexcept(wnoexcept(x &= y)) { return (x &= y); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(6, 0), _type, _type2, dep_ops::use_constfn> { template<typename = void> friend wconstfn  _type operator &(_type x, const _type2& y) wnoexcept(wnoexcept(x &= y)) { return (x &= y); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(6, 1), _type, _type2, dep_ops::no_constfn> { template<typename = void> friend inline  _type operator &(_type x, _type2&& y) wnoexcept(wnoexcept(x &= std::move(y))) { return (x &= std::move(y)); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(6, 1), _type, _type2, dep_ops::use_constfn> { template<typename = void> friend wconstfn  _type operator &(_type x, _type2&& y) wnoexcept(wnoexcept(x &= std::move(y))) { return (x &= std::move(y)); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(6, 2), _type, _type2, dep_ops::no_constfn> { template<typename = void> friend inline  _type operator &(const _type2& x, _type y) wnoexcept(wnoexcept(y &= x)) { return (y &= x); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(6, 2), _type, _type2, dep_ops::use_constfn> { template<typename = void> friend wconstfn  _type operator &(const _type2& x, _type y) wnoexcept(wnoexcept(y &= x)) { return (y &= x); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(6, 3), _type, _type2, dep_ops::no_constfn> { template<typename = void> friend inline  _type operator &(_type2&& x, _type y) wnoexcept(wnoexcept(y &= std::move(x))) { return (y &= std::move(x)); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(6, 3), _type, _type2, dep_ops::use_constfn> { template<typename = void> friend wconstfn  _type operator &(_type2&& x, _type y) wnoexcept(wnoexcept(y &= std::move(x))) { return (y &= std::move(x)); } };
			template<class _type, typename _type2>
			struct bin_ops<ops_bin_id(7, 0), _type, _type2, dep_ops::no_constfn> { template<typename = void> friend inline  _type operator |(_type x, const _type2& y) wnoexcept(wnoexcept(x |= y)) { return (x |= y); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(7, 0), _type, _type2, dep_ops::use_constfn> { template<typename = void> friend wconstfn  _type operator |(_type x, const _type2& y) wnoexcept(wnoexcept(x |= y)) { return (x |= y); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(7, 1), _type, _type2, dep_ops::no_constfn> { template<typename = void> friend inline  _type operator |(_type x, _type2&& y) wnoexcept(wnoexcept(x |= std::move(y))) { return (x |= std::move(y)); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(7, 1), _type, _type2, dep_ops::use_constfn> { template<typename = void> friend wconstfn  _type operator |(_type x, _type2&& y) wnoexcept(wnoexcept(x |= std::move(y))) { return (x |= std::move(y)); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(7, 2), _type, _type2, dep_ops::no_constfn> { template<typename = void> friend inline  _type operator |(const _type2& x, _type y) wnoexcept(wnoexcept(y |= x)) { return (y |= x); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(7, 2), _type, _type2, dep_ops::use_constfn> { template<typename = void> friend wconstfn  _type operator |(const _type2& x, _type y) wnoexcept(wnoexcept(y |= x)) { return (y |= x); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(7, 3), _type, _type2, dep_ops::no_constfn> { template<typename = void> friend inline  _type operator |(_type2&& x, _type y) wnoexcept(wnoexcept(y |= std::move(x))) { return (y |= std::move(x)); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(7, 3), _type, _type2, dep_ops::use_constfn> { template<typename = void> friend wconstfn  _type operator |(_type2&& x, _type y) wnoexcept(wnoexcept(y |= std::move(x))) { return (y |= std::move(x)); } };
			template<class _type, typename _type2>
			struct bin_ops<ops_bin_id(8, 0), _type, _type2, dep_ops::no_constfn> { template<typename = void> friend inline  _type operator <<(_type x, const _type2& y) wnoexcept(wnoexcept(x <<= y)) { return (x <<= y); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(8, 0), _type, _type2, dep_ops::use_constfn> { template<typename = void> friend wconstfn  _type operator <<(_type x, const _type2& y) wnoexcept(wnoexcept(x <<= y)) { return (x <<= y); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(8, 1), _type, _type2, dep_ops::no_constfn> { template<typename = void> friend inline  _type operator <<(_type x, _type2&& y) wnoexcept(wnoexcept(x <<= std::move(y))) { return (x <<= std::move(y)); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(8, 1), _type, _type2, dep_ops::use_constfn> { template<typename = void> friend wconstfn  _type operator <<(_type x, _type2&& y) wnoexcept(wnoexcept(x <<= std::move(y))) { return (x <<= std::move(y)); } };
			template<class _type, typename _type2>
			struct bin_ops<ops_bin_id(9, 0), _type, _type2, dep_ops::no_constfn> { template<typename = void> friend inline  _type operator >> (_type x, const _type2& y) wnoexcept(wnoexcept(x >>= y)) { return (x >>= y); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(9, 0), _type, _type2, dep_ops::use_constfn> { template<typename = void> friend wconstfn  _type operator >> (_type x, const _type2& y) wnoexcept(wnoexcept(x >>= y)) { return (x >>= y); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(9, 1), _type, _type2, dep_ops::no_constfn> { template<typename = void> friend inline  _type operator >> (_type x, _type2&& y) wnoexcept(wnoexcept(x >>= std::move(y))) { return (x >>= std::move(y)); } }; template<class _type, typename _type2> struct bin_ops<ops_bin_id(9, 1), _type, _type2, dep_ops::use_constfn> { template<typename = void> friend wconstfn  _type operator >> (_type x, _type2&& y) wnoexcept(wnoexcept(x >>= std::move(y))) { return (x >>= std::move(y)); } };

			template<class _type, typename _type2, class _tOpt,
				template<typename...> class... _gOps>
			using flat_ops = vseq::defer_t<ebases, vseq::fold_t<vseq::_a<vseq::concat_t>,
				empty_base<>,
				empty_base<vseq::defer_t<empty_base, _gOps<_type, _type2, _tOpt>>... >> >;
			//@}

		} // namespace deprecated_v2_details;


		namespace dep_ops
		{

			//! \since build 1.4
			//@{



#ifndef WB_IMPL_MSCPP
			template<class _type, typename _type2 = _type, class _tOpt = use_constfn>
			using less_than_comparable = _t<deprecated_v2_details::ops_seq<_type, _type2, _tOpt, cond_t<is_same<_type, _type2>, index_sequence<2, 4, 1>, index_sequence<0, 1, 2, 3, 4, 5>>>>;

			template<class _type, typename _type2 = _type, class _tOpt = use_constfn>
			using equality_comparable = _t<deprecated_v2_details::ops_seq<_type, _type2, _tOpt, cond_t<is_same<_type, _type2>, index_sequence<7>, index_sequence<6, 7, 8>>>>;

			/*template<class _type, typename _type2 = _type, class _tOpt = use_constfn>
			using equivalent = _t<deprecated_v2_details::ops_seq<_type, _type2, _tOpt, cond_t<is_same<_type, _type2>, index_sequence<10>, index_sequence<9>>>>;

			template<class _type, typename _type2 = _type, class _tOpt = use_constfn>
			using partially_ordered = _t<deprecated_v2_details::ops_seq<_type, _type2, _tOpt, cond_t<is_same<_type, _type2>, index_sequence<2, 11, 14>, index_sequence<11, 12, 2, 3, 13, 14>>>>;
			*/

			template<class _type, typename _type2 = _type, class _tOpt = use_constfn>
			using addable = _t< deprecated_v2_details::ops_seq<_type, _type2, _tOpt, cond_t<is_same<_type, _type2>, deprecated_v2_details::ops_bin_id_seq<0, 0, 1>, deprecated_v2_details::ops_bin_id_seq<0, 0, 1, 2, 3>>>>;

			template<class _type, typename _type2 = _type, class _tOpt = use_constfn>
			using subtractable = _t<deprecated_v2_details::ops_seq<_type, _type2, _tOpt, deprecated_v2_details::ops_bin_id_seq<1, 0, 1>>>;


#else
			namespace deprecated_v2_details_vc {
				template<bool _test, class _type, typename _type2, class _tOpt>
				struct less_than_comparable_base :
					deprecated_v2_details::bin_ops<2, _type, _type2, _tOpt>,
					deprecated_v2_details::bin_ops<4, _type, _type2, _tOpt>,
					deprecated_v2_details::bin_ops<1, _type, _type2, _tOpt>
				{};

				template< class _type, typename _type2, class _tOpt>
				struct less_than_comparable_base<false, _type, _type2, _tOpt> :
					deprecated_v2_details::bin_ops<0, _type, _type2, _tOpt>,
					deprecated_v2_details::bin_ops<1, _type, _type2, _tOpt>,
					deprecated_v2_details::bin_ops<2, _type, _type2, _tOpt>,
					deprecated_v2_details::bin_ops<3, _type, _type2, _tOpt>,
					deprecated_v2_details::bin_ops<4, _type, _type2, _tOpt>,
					deprecated_v2_details::bin_ops<5, _type, _type2, _tOpt>
				{};

				template<bool _test, class _type, typename _type2, class _tOpt>
				struct equality_comparable_base :
					deprecated_v2_details::bin_ops<7, _type, _type2, _tOpt>
				{};

				template< class _type, typename _type2, class _tOpt>
				struct equality_comparable_base<false, _type, _type2, _tOpt> :
					deprecated_v2_details::bin_ops<6, _type, _type2, _tOpt>,
					deprecated_v2_details::bin_ops<7, _type, _type2, _tOpt>,
					deprecated_v2_details::bin_ops<8, _type, _type2, _tOpt>
				{};


				template<bool _test, class _type, typename _type2, class _tOpt>
				struct addable_base :
					deprecated_v2_details::bin_ops<16, _type, _type2, _tOpt>,
					deprecated_v2_details::bin_ops<17, _type, _type2, _tOpt>
				{};

				template< class _type, typename _type2, class _tOpt>
				struct addable_base<false, _type, _type2, _tOpt> :
					deprecated_v2_details::bin_ops<16, _type, _type2, _tOpt>,
					deprecated_v2_details::bin_ops<17, _type, _type2, _tOpt>,
					deprecated_v2_details::bin_ops<18, _type, _type2, _tOpt>,
					deprecated_v2_details::bin_ops<19, _type, _type2, _tOpt>
				{};
			}

			template<class _type, typename _type2 = _type, class _tOpt = use_constfn>
			struct less_than_comparable : deprecated_v2_details_vc::less_than_comparable_base<is_same<_type, _type2>::value, _type, _type2, _tOpt>
			{};

			template<class _type, typename _type2 = _type, class _tOpt = use_constfn>
			struct equality_comparable : deprecated_v2_details_vc::equality_comparable_base<is_same<_type, _type2>::value, _type, _type2, _tOpt>
			{};

			template<class _type, typename _type2 = _type, class _tOpt = use_constfn>
			struct addable : deprecated_v2_details_vc::addable_base<is_same<_type, _type2>::value, _type, _type2, _tOpt>
			{};

			template<class _type, typename _type2 = _type, class _tOpt = use_constfn>
			struct subtractable :
				deprecated_v2_details::bin_ops<32, _type, _type2, _tOpt>,
				deprecated_v2_details::bin_ops<33, _type, _type2, _tOpt>
			{};
#endif
			template<class _type, typename _type2 = _type, class _tOpt = use_constfn>
			using equivalent = _t<deprecated_v2_details::ops_seq<_type, _type2, _tOpt, cond_t<is_same<_type, _type2>, index_sequence<10>, index_sequence<9>>>>;

			template<class _type, typename _type2 = _type, class _tOpt = use_constfn>
			using partially_ordered = _t<deprecated_v2_details::ops_seq<_type, _type2, _tOpt, cond_t<is_same<_type, _type2>, index_sequence<2, 11, 14>, index_sequence<11, 12, 2, 3, 13, 14>>>>;

			template<class _type, typename _type2 = _type, class _tOpt = use_constfn>
			using subtractable_left = _t<deprecated_v2_details::ops_seq<_type, _type2, _tOpt, deprecated_v2_details::ops_bin_id_seq<1, 4, 5>>>;

			template<class _type, typename _type2 = _type, class _tOpt = use_constfn>
			using multipliable = _t< deprecated_v2_details::ops_seq<_type, _type2, _tOpt, cond_t<is_same<_type, _type2>, deprecated_v2_details::ops_bin_id_seq<2, 0, 1>, deprecated_v2_details::ops_bin_id_seq<2, 0, 1, 2, 3>>>>;

			template<class _type, typename _type2 = _type, class _tOpt = use_constfn>
			using dividable = _t<deprecated_v2_details::ops_seq<_type, _type2, _tOpt, deprecated_v2_details::ops_bin_id_seq<3, 0, 1>>>; template<class _type, typename _type2 = _type, class _tOpt = use_constfn> using dividable_left = _t<deprecated_v2_details::ops_seq<_type, _type2, _tOpt, deprecated_v2_details::ops_bin_id_seq<3, 4, 5>>>;

			template<class _type, typename _type2 = _type, class _tOpt = use_constfn>
			using modable = _t<deprecated_v2_details::ops_seq<_type, _type2, _tOpt, deprecated_v2_details::ops_bin_id_seq<4, 0, 1>>>; template<class _type, typename _type2 = _type, class _tOpt = use_constfn> using modable_left = _t<deprecated_v2_details::ops_seq<_type, _type2, _tOpt, deprecated_v2_details::ops_bin_id_seq<4, 4, 5>>>;

			template<class _type, typename _type2 = _type, class _tOpt = use_constfn>
			using left_shiftable = _t<deprecated_v2_details::ops_seq<_type, _type2, _tOpt, deprecated_v2_details::ops_bin_id_seq<5, 0, 1>>>;

			template<class _type, typename _type2 = _type, class _tOpt = use_constfn>
			using right_shiftable = _t<deprecated_v2_details::ops_seq<_type, _type2, _tOpt, deprecated_v2_details::ops_bin_id_seq<6, 0, 1>>>;

			template<class _type, typename _type2 = _type, class _tOpt = use_constfn>
			using xorable = _t< deprecated_v2_details::ops_seq<_type, _type2, _tOpt, cond_t<is_same<_type, _type2>, deprecated_v2_details::ops_bin_id_seq<7, 0, 1>, deprecated_v2_details::ops_bin_id_seq<7, 0, 1, 2, 3>>>>;

			template<class _type, typename _type2 = _type, class _tOpt = use_constfn>
			using andable = _t< deprecated_v2_details::ops_seq<_type, _type2, _tOpt, cond_t<is_same<_type, _type2>, deprecated_v2_details::ops_bin_id_seq<8, 0, 1>, deprecated_v2_details::ops_bin_id_seq<8, 0, 1, 2, 3>>>>;

			template<class _type, typename _type2 = _type, class _tOpt = use_constfn>
			using orable = _t< deprecated_v2_details::ops_seq<_type, _type2, _tOpt, cond_t<is_same<_type, _type2>, deprecated_v2_details::ops_bin_id_seq<9, 0, 1>, deprecated_v2_details::ops_bin_id_seq<9, 0, 1, 2, 3>>>>;

			template<class _type, class _tOpt = use_constfn>
			struct incrementable
			{
				//! \since build 1.4
				friend _type
					operator++(_type& x, int) wnoexcept(noexcept(_type(x)) && noexcept(++x)
						&& noexcept(_type(std::declval<_type>())))
				{
					_type t(x);

					++x;
					return t;
				}
			};

			template<class _type, class _tOpt = use_constfn>
			struct decrementable
			{
				//! \since build 1.4
				friend _type
					operator--(_type& x, int) wnoexcept(noexcept(_type(x)) && noexcept(--x)
						&& noexcept(_type(std::declval<_type>())))
				{
					_type t(x);

					++x;
					return t;
				}
			};

			template<class _type, typename _type2, class _tOpt = use_constfn>
			struct dereferenceable
			{
				// TODO: Add non-const overloaded version? SFINAE?
				// TODO: Use '_tOpt'.

				wconstfn identity_t<decltype(white::constfn_addressof(std::declval<const _type2&>()))>
					operator->() const
#ifndef WB_IMPL_MSCPP
					wnoexcept_spec(*std::declval<const _type&>())
#endif
				{
					return white::constfn_addressof(*static_cast<const _type&>(*this));
				}
			};

			template<class _type, typename _type2, typename _type3, class _tOpt = use_constfn>
			struct indexable
			{
				// TODO: Add non-const overloaded version? SFINAE?
				// TODO: Use '_tOpt'.
				wconstfn _type3
					operator[](_type2 n) const
					wnoexcept_spec(*(std::declval<const _type&>() + n))
				{
					return *(static_cast<const _type&>(*this) + n);
				}
			};

			template<class _type, typename _type2 = _type, class _tOpt = use_constfn> using totally_ordered = deprecated_v2_details::flat_ops<_type, _type2, _tOpt, less_than_comparable, equality_comparable>;
			template<class _type, typename _type2 = _type, class _tOpt = use_constfn> using additive = deprecated_v2_details::flat_ops<_type, _type2, _tOpt, addable, subtractable>;
			template<class _type, typename _type2 = _type, class _tOpt = use_constfn> using multiplicative = deprecated_v2_details::flat_ops<_type, _type2, _tOpt, multipliable, dividable>;
			template<class _type, typename _type2 = _type, class _tOpt = use_constfn> using integer_multiplicative = deprecated_v2_details::flat_ops<_type, _type2, _tOpt, multiplicative, modable>;
			template<class _type, typename _type2 = _type, class _tOpt = use_constfn> using arithmetic = deprecated_v2_details::flat_ops<_type, _type2, _tOpt, additive, multiplicative>;
			template<class _type, typename _type2 = _type, class _tOpt = use_constfn> using integer_arithmetic = deprecated_v2_details::flat_ops<_type, _type2, _tOpt, additive, integer_multiplicative>;
			template<class _type, typename _type2 = _type, class _tOpt = use_constfn> using bitwise = deprecated_v2_details::flat_ops<_type, _type2, _tOpt, xorable, andable, orable>;
			template<class _type, class _tOpt = use_constfn>
			using unit_steppable
				= deprecated_v2_details::ebases<incrementable<_type>, decrementable<_type>>;
			template<class _type, typename _type2 = _type, class _tOpt = use_constfn> using shiftable = deprecated_v2_details::flat_ops<_type, _type2, _tOpt, left_shiftable, right_shiftable>;
			template<class _type, typename _type2 = _type, class _tOpt = use_constfn> using ring_operators = deprecated_v2_details::flat_ops<_type, _type2, _tOpt, additive, subtractable_left, multipliable>;
			template<class _type, typename _type2 = _type, class _tOpt = use_constfn> using ordered_ring_operators = deprecated_v2_details::flat_ops<_type, _type2, _tOpt, ring_operators, totally_ordered>;
			template<class _type, typename _type2 = _type, class _tOpt = use_constfn> using field_operators = deprecated_v2_details::flat_ops<_type, _type2, _tOpt, ring_operators, dividable, dividable_left>;
			template<class _type, typename _type2 = _type, class _tOpt = use_constfn> using ordered_field_operators = deprecated_v2_details::flat_ops<_type, _type2, _tOpt, field_operators, totally_ordered>;
			template<class _type, typename _type2 = _type, class _tOpt = use_constfn> using euclidean_ring_operators = deprecated_v2_details::flat_ops<_type, _type2, _tOpt, ring_operators, dividable, dividable_left, modable, modable_left>;
			template<class _type, typename _type2 = _type, class _tOpt = use_constfn> using ordered_euclidean_ring_operators = deprecated_v2_details::flat_ops<_type, _type2, _tOpt, totally_ordered, euclidean_ring_operators>;

#ifndef WB_IMPL_MSCPP
			template<class _type, typename _type2, class _tOpt = use_constfn>
			using input_iteratable = deprecated_v2_details::ebases< equality_comparable<_type, _type, _tOpt>, incrementable<_type>, dereferenceable<_type, _type2, _tOpt >>;


			template<class _type, typename _type2, class _tOpt = use_constfn>
			using output_iteratable = deprecated_v2_details::ebases<incrementable<_type>>;

			template<class _type, typename _type2, class _tOpt = use_constfn>
			using forward_iteratable = deprecated_v2_details::ebases<input_iteratable<_type, _type2, _tOpt>>;

			template<class _type, typename _type2, class _tOpt = use_constfn>
			using bidirectional_iteratable = deprecated_v2_details::ebases< forward_iteratable<_type, _type2, _tOpt>, decrementable<_type >>;

			template<class _type, typename _type2, typename _type3, class _tOpt = use_constfn>
			using random_access_iteratable
				= deprecated_v2_details::ebases<bidirectional_iteratable<_type, _type3, _tOpt>,
				less_than_comparable<_type, _type, _tOpt>,
				additive<_type, _type2, _tOpt>, indexable<_type, _type2, _type, _tOpt >>;
#else
			template<class _type, typename _type2, class _tOpt = use_constfn>
			struct input_iteratable : equality_comparable<_type, _type, _tOpt>, incrementable<_type>, dereferenceable<_type, _type2, _tOpt >
			{};

			template<class _type, typename _type2, class _tOpt = use_constfn>
			struct output_iteratable : incrementable<_type>
			{};

			template<class _type, typename _type2, class _tOpt = use_constfn>
			struct forward_iteratable : input_iteratable<_type, _type2, _tOpt>
			{};

			template<class _type, typename _type2, class _tOpt = use_constfn>
			struct bidirectional_iteratable : forward_iteratable<_type, _type2, _tOpt>, decrementable<_type >
			{};

			template<class _type, typename _type2, typename _type3, class _tOpt = use_constfn>
			struct random_access_iteratable :
				bidirectional_iteratable<_type, _type3, _tOpt>,
				less_than_comparable<_type, _type, _tOpt>,
				additive<_type, _type2, _tOpt>, indexable<_type, _type2, _type, _tOpt >
			{};
#endif

			/*!
			\note ����ģ������������ɵĲ�������������
			\note ��ǰ��Ԫ����Ԫ��������ʹ�� wconstfn ��
			\todo ʵ�ֵ���ѡ�
			*/
			template<class _type, typename _type2 = _type, class _tOpt = use_constfn> using operators = deprecated_v2_details::flat_ops<_type, _type2, _tOpt, totally_ordered, integer_arithmetic, bitwise>;
			//@}

		} // namespace dep_ops;

		  //! \since build 1.4
		using namespace dep_ops;
	}
}

namespace white
{
	using namespace deprecated;
}

#endif