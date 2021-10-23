/*! \file placement.hpp
\ingroup WBase
\brief ���ö�����������
\par �޸�ʱ��:
2016-09-20 09:58 +0800
*/

#ifndef WBase_placement_hpp
#define WBase_placement_hpp 1

#include "addressof.hpp"
#include "deref_op.hpp"
#include "cassert.h"

#include <new>
#include <iterator>
#include <memory>

namespace white {

	/*!
	\brief Ĭ�ϳ�ʼ����ǡ�
	*/
	wconstexpr const struct default_init_t {} default_init{};

	/*!
	\brief ֵ��ʼ����ǡ�
	*/
	wconstexpr const struct value_init_t {} value_init{};


	/*!
	\see WG21 P0032R3 ��
	\see WG21 N4606 20.2.7[utility.inplace] ��
	*/
	//@{
	//! \brief ԭ�ر�����͡�
	struct in_place_tag
	{
		in_place_tag() = delete;
	};

	//! \brief ԭ�ؿձ�����͡�
	using in_place_t = in_place_tag(&)(wimpl(empty_base<>));

	//! \brief ԭ�����ͱ��ģ�塣
	template<typename _type>
	using in_place_type_t = in_place_tag(&)(wimpl(empty_base<_type>));

	//! \brief ԭ���������ģ�塣
	template<size_t _vIdx>
	using in_place_index_t = in_place_tag(&)(wimpl(size_t_<_vIdx>));

#ifdef WB_IMPL_MSCPP
#pragma warning(disable:4646)
#endif

	/*!
	\ingroup helper_functions
	\brief ԭ�ر�Ǻ�����
	\warning ��������δ������Ϊ��
	*/
	wimpl(WB_NORETURN) inline in_place_tag
		in_place(wimpl(empty_base<>))
	{
		WB_ASSUME(false);
	}
	template<typename _type>
	wimpl(WB_NORETURN) in_place_tag
		in_place(wimpl(empty_base<_type>))
	{
		WB_ASSUME(false);
	}
	template<size_t _vIdx>
	wimpl(WB_NORETURN) in_place_tag
		in_place(wimpl(size_t_<_vIdx>))
	{
		WB_ASSUME(false);
	}
	//@}


	/*!
	\tparam _type ����Ķ������͡�
	\param obj ����Ĵ洢����
	*/
	//@{
	//! \brief ��Ĭ�ϳ�ʼ���ڶ����й��졣
	template<typename _type, typename _tObj>
	inline _type*
		construct_default_within(_tObj& obj)
	{
		return ::new(static_cast<void*>(static_cast<_tObj*>(
			constfn_addressof(obj)))) _type;
	}

	/*!
	\brief ��ֵ��ʼ���ڶ����й��졣
	\tparam _tParams ���ڹ������Ĳ��������͡�
	\param args ���ڹ������Ĳ�������
	*/
	template<typename _type, typename _tObj, typename... _tParams>
	inline _type*
		construct_within(_tObj& obj, _tParams&&... args)
	{
		return ::new(static_cast<void*>(static_cast<_tObj*>(
			constfn_addressof(obj)))) _type(wforward(args)...);
	}
	//@}

	/*!
	\brief ��Ĭ�ϳ�ʼ��ԭ�ع��졣
	\tparam _tParams ���ڹ������Ĳ��������͡�
	\param args ���ڹ������Ĳ�������
	*/
	template<typename _type>
	inline void
		construct_default_in(_type& obj)
	{
		construct_default_within<_type>(obj);
	}

	/*!
	\brief ��ֵ��ʼ��ԭ�ع��졣
	\tparam _tParams ���ڹ������Ĳ��������͡�
	\param args ���ڹ������Ĳ�������
	*/
	template<typename _type, typename... _tParams>
	inline void
		construct_in(_type& obj, _tParams&&... args)
	{
		construct_within<_type>(obj, wforward(args)...);
	}

	//@{
	//! \tparam _tIter ���������͡�
	//@{
	/*!
	\tparam _tParams ���ڹ������Ĳ��������͡�
	\param args ���ڹ������Ĳ�������
	\pre ���ԣ�ָ����Χĩβ����ĵ��������� <tt>!is_undereferenceable</tt> ��
	*/
	//@{
	/*!
	\brief ʹ��ָ�������ڵ�����ָ����λ����ָ��������ʼ���������
	\param i ��������
	\note ��ʽת��Ϊ void* ָ����ʵ�ֱ�׼���㷨 uninitialized_* ʵ�����Ƶ����塣
	\see libstdc++ 5 �� Microsoft VC++ 2013 ��׼���������ռ� std �ڶ�ָ�����͵�ʵ�֣�
	_Construct ģ�塣
	*/
	template<typename _tIter, typename... _tParams>
	void
		construct(_tIter i, _tParams&&... args)
	{
		using value_type = typename std::iterator_traits<_tIter>::value_type;

		wconstraint(!is_undereferenceable(i));
		construct_within<value_type>(*i, wforward(args)...);
	}

	/*!
	\brief ʹ��ָ�������ڵ�����ָ����λ����Ĭ�ϳ�ʼ���������
	\param i ��������
	*/
	template<typename _tIter>
	void
		construct_default(_tIter i)
	{
		using value_type = typename std::iterator_traits<_tIter>::value_type;

		wconstraint(!is_undereferenceable(i));
		construct_default_within<value_type>(*i);
	}

	/*!
	\brief ʹ��ָ���Ĳ����ظ������������Χ�ڵĶ������С�
	\note ���������ݵĴ����͹���Ķ�������ͬ��
	*/
	template<typename _tIter, typename... _tParams>
	void
		construct_range(_tIter first, _tIter last, _tParams&&... args)
	{
		for (; first != last; ++first)
			construct(first, wforward(args)...);
	}
	//@}


	/*!
	\brief ԭ�����١�
	\see WG21 N4606 20.10.10.7[specialized.destroy] ��
	*/
	//@{
	using std::destroy_at;

	using std::destroy;

	using std::destroy_n;
	//@}
	//@}


	/*!
	\brief ԭ��������
	\tparam _type ����������������͡�
	\param obj �����Ķ���
	\sa destroy_at
	*/
	template<typename _type>
	inline void
		destruct_in(_type& obj)
	{
		obj.~_type();
	}

	/*!
	\brief ����������ָ��Ķ���
	\param i ��������
	\pre ���ԣ�<tt>!is_undereferenceable(i)</tt> ��
	\sa destroy
	*/
	template<typename _tIter>
	void
		destruct(_tIter i)
	{
		using value_type = typename std::iterator_traits<_tIter>::value_type;

		wconstraint(!is_undereferenceable(i));
		destruct_in<value_type>(*i);
	}

	/*!
	\brief ����d��������Χ�ڵĶ������С�
	\note ��֤˳��������
	\sa destroy
	*/
	template<typename _tIter>
	void
		destruct_range(_tIter first, _tIter last)
	{
		for (; first != last; ++first)
			destruct(first);
	}
	//@}


#define WB_Impl_UninitGuard_Begin \
	auto i = first; \
	\
	try \
	{

	// NOTE: The order of destruction is unspecified.
#define WB_Impl_UninitGuard_End \
	} \
	catch(...) \
	{ \
		white::destruct_range(first, i); \
		throw; \
	}

	//@{
	//! \brief �ڷ�Χ��δ��ʼ�����ù��졣
	//@{
	//! \see WG21 N4606 20.10.2[uninitialized.construct.default] ��
	//@{
	template<typename _tFwd>
	inline void
		uninitialized_default_construct(_tFwd first, _tFwd last)
	{
		WB_Impl_UninitGuard_Begin
			for (; first != last; ++first)
				white::construct_default(first);
		WB_Impl_UninitGuard_End
	}

	template<typename _tFwd, typename _tSize>
	_tFwd
		uninitialized_default_construct_n(_tFwd first, _tSize n)
	{
		WB_Impl_UninitGuard_Begin
			// XXX: Excessive order refinment by ','?
			for (; n > 0; static_cast<void>(++first), --n)
				white::construct_default(first);
		WB_Impl_UninitGuard_End
	}
	//@}

	//! \see WG21 N4606 20.10.3[uninitialized.construct.value] ��
	template<typename _tFwd>
	inline void
		uninitialized_value_construct(_tFwd first, _tFwd last)
	{
		WB_Impl_UninitGuard_Begin
			white::construct_range(first, last);
		WB_Impl_UninitGuard_End
	}

	template<typename _tFwd, typename _tSize>
	_tFwd
		uninitialized_value_construct_n(_tFwd first, _tSize n)
	{
		WB_Impl_UninitGuard_Begin
			// XXX: Excessive order refinment by ','?
			for (; n > 0; static_cast<void>(++first), --n)
				white::construct(first);
		WB_Impl_UninitGuard_End
			return first;
	}
	//@}


	/*!
	\brief ת�Ƴ�ʼ����Χ��
	\see WG21 N4606 20.10.10.5[uninitialized.move] ��
	*/
	//@{
	template<typename _tIn, class _tFwd>
	_tFwd
		uninitialized_move(_tIn first, _tIn last, _tFwd result)
	{
		WB_Impl_UninitGuard_Begin
			for (; first != last; static_cast<void>(++result), ++first)
				white::construct(result, std::move(*first));
		WB_Impl_UninitGuard_End
			return result;
	}

	template<typename _tIn, typename _tSize, class _tFwd>
	std::pair<_tIn, _tFwd>
		uninitialized_move_n(_tIn first, _tSize n, _tFwd result)
	{
		WB_Impl_UninitGuard_Begin
			// XXX: Excessive order refinment by ','?
			for (; n > 0; ++result, static_cast<void>(++first), --n)
				white::construct(result, std::move(*first));
		WB_Impl_UninitGuard_End
			return{ first, result };
	}
	//@}


	/*!
	\brief �ڵ�����ָ����δ��ʼ���ķ�Χ�Ϲ������
	\tparam _tFwd �����Χǰ����������͡�
	\tparam _tParams ���ڹ������Ĳ��������͡�
	\param first �����Χ��ʼ��������
	\param args ���ڹ������Ĳ�������
	\note ���������ݵĴ����͹���Ķ�������ͬ��
	\note �ӿڲ���֤ʧ��ʱ������˳��
	*/
	//@{
	/*!
	\param last �����Χ��ֹ��������
	\note �� std::unitialized_fill ���ƣ�������ָ�������ʼ��������
	\see WG21 N4431 20.7.12.3[uninitialized.fill] ��
	*/
	template<typename _tFwd, typename... _tParams>
	void
		uninitialized_construct(_tFwd first, _tFwd last, _tParams&&... args)
	{
		WB_Impl_UninitGuard_Begin
			for (; i != last; ++i)
				white::construct(i, wforward(args)...);
		WB_Impl_UninitGuard_End
	}

	/*!
	\tparam _tSize ��Χ��С���͡�
	\param n ��Χ��С��
	\note �� std::unitialized_fill_n ���ƣ�������ָ�������ʼ��������
	\see WG21 N4431 20.7.12.4[uninitialized.fill.n] ��
	*/
	template<typename _tFwd, typename _tSize, typename... _tParams>
	void
		uninitialized_construct_n(_tFwd first, _tSize n, _tParams&&... args)
	{
		WB_Impl_UninitGuard_Begin
			// NOTE: This form is by specification (WG21 N4431) of
			//	'std::unitialized_fill' literally.
			for (; n--; ++i)
				white::construct(i, wforward(args)...);
		WB_Impl_UninitGuard_End
	}
	//@}
	//@}

#undef WB_Impl_UninitGuard_End
#undef WB_Impl_UninitGuard_Begin

	//@{
	//! \brief Ĭ�ϳ�ʼ�������������
	template<typename _type, class _tAlloc = std::allocator<_type>>
	class default_init_allocator : public _tAlloc
	{
	public:
		using allocator_type = _tAlloc;
		using traits_type = std::allocator_traits<allocator_type>;
		template<typename _tOther>
		struct rebind
		{
			using other = default_init_allocator<_tOther,
				typename traits_type::template rebind_alloc<_tOther>>;
		};

		using allocator_type::allocator_type;

		template<typename _tOther>
		void
			construct(_tOther* p)
			wnoexcept(std::is_nothrow_default_constructible<_tOther>::value)
		{
			::new(static_cast<void*>(p)) _tOther;
		}
		template<typename _tOther, typename... _tParams>
		void
			construct(_type* p, _tParams&&... args)
		{
			traits_type::construct(static_cast<allocator_type&>(*this), p,
				wforward(args)...);
		}
	};


	/*!
	\brief ���ô洢�Ķ���ɾ�������ͷ�ʱ����α����������
	\tparam _type ��ɾ���Ķ������͡�
	\pre _type ���� Destructible ��
	*/
	template<typename _type, typename _tPointer = _type*>
	struct placement_delete
	{
		using pointer = _tPointer;

		wconstfn placement_delete() wnothrow = default;
		template<typename _type2,
			wimpl(typename = enable_if_convertible_t<_type2*, _type*>)>
			placement_delete(const placement_delete<_type2>&) wnothrow
		{}

		void
			operator()(pointer p) const wnothrowv
		{
			destroy_at(p);
		}
	};


	//! \brief ��ռ���ô洢�Ķ�������Ȩ��ָ�롣
	template<typename _type, typename _tPointer = _type*>
	using placement_ptr
		= std::unique_ptr<_type, placement_delete<_type, _tPointer>>;
	//@}


	/*!
	\brief ���Ǻű�ǵĿ�ѡֵ��
	\warning ����������
	*/
	template<typename _tToken, typename _type>
	struct tagged_value
	{
		using token_type = _tToken;
		using value_type = _type;

		token_type token;
		union
		{
			empty_base<> empty;
			mutable _type value;
		};

		wconstfn
			tagged_value()
			wnoexcept_spec(is_nothrow_default_constructible<token_type>())
			: token(), empty()
		{}
		tagged_value(default_init_t)
		{}
		tagged_value(token_type t)
			: token(t), empty()
		{}
		template<typename... _tParams>
		explicit wconstfn
			tagged_value(token_type t, in_place_t, _tParams&&... args)
			: token(t), value(wforward(args)...)
		{}
		tagged_value(const tagged_value& v)
			: token(v.token)
		{}
		/*!
		\brief ��������ʵ�֡�
		\note ��ʹ��Ĭ�Ϻ����Ա�����������ķ�ƽ�����������Ǻ�ʽ����ɾ����
		*/
		~tagged_value()
		{}

		template<typename... _tParams>
		void
			construct(_tParams&&... args)
		{
			white::construct_in(value, wforward(args)...);
		}

		void
			destroy() wnoexcept(is_nothrow_destructible<value_type>())
		{
			white::destruct_in(value);
		}

		void
			destroy_nothrow() wnothrow
		{
			wnoexcept_assert("Invalid type found.", value.~value_type());

			destroy();
		}
	};
}

#endif
