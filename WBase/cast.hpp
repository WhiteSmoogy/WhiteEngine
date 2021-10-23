#ifndef WBase_cast_hpp
#define WBase_cast_hpp 1

#include "type_op.hpp"
#include "exception.h"
#include "cassert.h"
#include <limits>
#include <typeinfo>

namespace white {

	/*!	\defgroup cast Cast
	\brief ��ʽ����ת����
	*/
	//@{
	/*!
	\pre Դ���ͺ�Ŀ�������˻�����ͬ��
	\note ָ��Դ�������ȣ����Զ��Ƶ�Ŀ�����͡�
	\sa as_const
	*/
	//! \brief ��������޶���ת����
	template<typename _tSrc, typename _tDst = const decay_t<_tSrc>&&>
	wconstfn _tDst
		qualify(_tSrc&& arg) wnothrow
	{
		static_assert(is_same<decay_t<_tSrc>, decay_t<_tDst>>(),
			"Non-qualification conversion found.");

		return static_cast<_tDst>(arg);
	}


	//@{
	//! \ingroup binary_type_traits
	//@{
	template<typename _tDst, typename _tSrc>
	using is_narrowing_from_floating_to_integer
		= and_<is_integral<_tDst>, is_floating_point<_tSrc>>;

	template<typename _tDst, typename _tSrc>
	using is_narrowing_from_floating_to_floating
		= and_<is_floating_point<_tDst>, is_floating_point<_tSrc>,
		is_same<common_type_t<_tDst, _tSrc>, _tSrc>>;

	template<typename _tDst, typename _tSrc>
	using is_narrowing_from_floating
		= or_<is_narrowing_from_floating_to_integer<_tDst, _tSrc>,
		is_narrowing_from_floating_to_floating<_tDst, _tSrc>>;
	//@}

	namespace details
	{

		template<typename _tDst>
		struct narrow_test
		{
			_tDst i;

			wconstfn false_
				get() const wnothrow
			{
				return{};
			}
		};

		template<typename _tDst, typename _tSrc>
		wconstfn auto
			test_narrow(_tSrc v) wnothrow -> decltype(narrow_test<_tDst>{v}.get())
		{
			return narrow_test<_tDst>{v}.get();
		}
		template<typename>
		wconstfn true_
			test_narrow(...) wnothrow
		{
			return{};
		}

	} // namespace details;
	  //@}

	  //@{
	  //! \brief ������С��ֵ��Χ����ʽת����
	template<typename _tDst, typename _tSrc>
	wconstfn _tDst
		narrow_cast(_tSrc v) wnothrow
	{
		static_assert(is_arithmetic<_tDst>(), "Invalid destination type found.");
		static_assert(is_arithmetic<_tSrc>(), "Invalid source type found.");
		static_assert(is_narrowing_from_floating<_tDst, _tSrc>()
			|| details::test_narrow<_tDst>(std::numeric_limits<_tSrc>::max())
			|| details::test_narrow<_tDst>(std::numeric_limits<_tSrc>::min()),
			"Invalid types found.");

		return static_cast<_tDst>(v);
	}

	//! \note ʹ�� ADL narrow_cast ��
	template<typename _tDst, typename _tSrc>
	inline _tDst
		narrow(_tSrc v)
	{
		const auto d(narrow_cast<_tDst>(v));

		if (static_cast<_tSrc>(d) == v && ((is_signed<_tDst>()
			== is_signed<_tSrc>()) || (d < _tDst()) == (v < _tSrc())))
			return d;
		throw narrowing_error();
	}

	namespace details
	{
		template<typename _tDst, typename _tSrc>
		wconstfn _tDst
			not_widen_cast(_tSrc v, true_) wnothrow
		{
			return narrow_cast<_tDst>(v);
		}
		template<typename _tDst, typename _tSrc>
		wconstfn _tDst
			not_widen_cast(_tSrc v, false_) wnothrow
		{
			static_assert(is_arithmetic<_tDst>(), "Invalid destination type found.");
			static_assert(is_arithmetic<_tSrc>(), "Invalid source type found.");
			static_assert(std::numeric_limits<_tSrc>::max() == std::numeric_limits<
				_tDst>::max() || std::numeric_limits<_tSrc>::min()
				== std::numeric_limits<_tDst>::min(), "Invalid types found.");
			return v;
		}

	} // namespace details;


	/*!
	\brief ������С��ֵ��Χ����ֵ��Χ������Сֵ���ֲ����ת����
	\note ʹ�� ADL narrow_cast ��
	*/
	template<typename _tDst, typename _tSrc>
	wconstfn _tDst
		not_widen_cast(_tSrc v) wnothrow
	{
		return details::not_widen_cast<_tDst>(v,
			bool_<is_narrowing_from_floating<_tDst, _tSrc>()
			|| details::test_narrow<_tDst>(std::numeric_limits<_tSrc>::max())
			|| details::test_narrow<_tDst>(std::numeric_limits<_tSrc>::min())>());
	}

	//! \note ʹ�� ADL not_widen_cast ��
	template<typename _tDst, typename _tSrc>
	inline _tDst
		not_widen(_tSrc v)
	{
		const auto d(not_widen_cast<_tDst>(v));

		if (static_cast<_tSrc>(d) == v && ((is_signed<_tDst>()
			== is_signed<_tSrc>()) || (d < _tDst()) == (v < _tSrc())))
			return d;
		throw narrowing_error();
	}
	//@}


	/*!
	\brief ��̬��ָ������ת����
	\tparam _tSrc Դ���͡�
	\tparam _pDst Ŀ�����͡�
	\pre ��̬���ԣ� _tSrc �Ƕ�̬�ࡣ
	\pre ��̬���ԣ� _pDst ���ڽ�ָ�롣
	\throw std::bad_cast dynamic_cast ʧ�ܡ�
	*/
	template<typename _pDst, class _tSrc>
	inline _pDst
		polymorphic_cast(_tSrc* v)
	{
		static_assert(is_polymorphic<_tSrc>(), "Non-polymorphic class found.");
		static_assert(is_pointer<_pDst>(), "Non-pointer destination found.");

		if (const auto p = dynamic_cast<_pDst>(v))
			return p;
		throw std::bad_cast();
	}

	/*!
	\brief ��̬��ָ����������ָ��ת����
	*/
	//@{
	/*!
	\tparam _tSrc Դ���͡�
	\tparam _pDst Ŀ�����͡�
	\pre ��̬���ԣ� _tSrc �Ƕ�̬�ࡣ
	\pre ��̬���ԣ� _pDst ���ڽ�ָ�롣
	\pre ��̬���ԣ� _tSrc �� _pDst ָ�����ȥ�����η���Ļ��ࡣ
	\pre ���ԣ� dynamic_cast �ɹ���
	*/
	template<typename _pDst, class _tSrc>
	inline _pDst
		polymorphic_downcast(_tSrc* v) wnothrowv
	{
		static_assert(is_polymorphic<_tSrc>(), "Non-polymorphic class found.");
		static_assert(is_pointer<_pDst>(), "Non-pointer destination found.");
		static_assert(is_base_of<_tSrc, remove_cv_t<
			remove_pointer_t<_pDst>>>(), "Wrong destination type found.");

		wassume(dynamic_cast<_pDst>(v) == v);
		return static_cast<_pDst>(v);
	}
	/*!
	\tparam _tSrc Դ���͡�
	\tparam _rDst Ŀ�����͡�
	\pre ��̬���ԣ� _rDst ����ֵ���á�
	*/
	template<typename _rDst, class _tSrc>
	wconstfn wimpl(enable_if_t)<is_lvalue_reference<_rDst>::value, _rDst>
		polymorphic_downcast(_tSrc& v) wnothrowv
	{
		return *polymorphic_downcast<remove_reference_t<_rDst>*>(
			std::addressof(v));
	}
	/*!
	\tparam _tSrc Դ���͡�
	\tparam _rDst Ŀ�����͡�
	\pre ��̬���ԣ� _rDst ����ֵ���á�
	*/
	template<typename _rDst, class _tSrc>
	wconstfn wimpl(enable_if_t)<is_rvalue_reference<_rDst>::value
		&& !is_reference<_tSrc>::value, _rDst>
		polymorphic_downcast(_tSrc&& v) wnothrowv
	{
		return std::move(polymorphic_downcast<_rDst&>(v));
	}
	/*!
	\tparam _tSrc Դ��Ԫ�����͡�
	\tparam _tDst Ŀ���Ԫ�����͡�
	\tparam _tDeleter ɾ�������͡�
	\pre _tDst ��ͨ�� \c release() ȡ�õ�ָ���ɾ������ֵ���졣
	\pre ���ԣ����� polymorphic_downcast ת�� \c release() ȡ�õ�ָ�뱣֤���쳣�׳���
	\note ʹ�� ADL ���� polymorphic_downcast ת�� \c release() ȡ�õ�ָ�롣
	*/
	template<class _tDst, typename _tSrc, typename _tDeleter>
	inline wimpl(enable_if_t)<!is_reference<_tDst>::value
		&& !is_array<_tDst>::value, std::unique_ptr<_tDst, _tDeleter>>
		polymorphic_downcast(std::unique_ptr<_tSrc, _tDeleter>&& v) wnothrowv
	{
		using dst_type = std::unique_ptr<_tDst, _tDeleter>;
		using pointer = typename dst_type::pointer;
		auto ptr(v.release());
		wnoexcept_assert("Invalid cast found.", polymorphic_downcast<pointer>(ptr));

		wassume(bool(ptr));
		return dst_type(polymorphic_downcast<pointer>(ptr),
			std::move(v.get_deleter()));
	}
	/*!
	\tparam _tSrc Դ��Ԫ�����͡�
	\tparam _tDst Ŀ���Ԫ�����͡�
	\pre _tDst ��ͨ�� \c get() ȡ�õ�ָ���ɾ������ֵ���졣
	*/
	template<class _tDst, typename _tSrc>
	inline wimpl(enable_if_t)<!is_reference<_tDst>::value
		&& !is_array<_tDst>::value, std::shared_ptr<_tDst>>
		polymorphic_downcast(const std::unique_ptr<_tSrc>& v) wnothrowv
	{
		wassume(dynamic_cast<_tDst*>(v.get()) == v.get());

		return std::static_pointer_cast<_tDst>(v);
	}
	//@}

	//! \brief ��̬��ָ�뽻��ת����
	//@{
	/*!
	\tparam _tSrc Դ���͡�
	\tparam _pDst Ŀ�����͡�
	\pre ��̬���ԣ� _tSrc �Ƕ�̬�ࡣ
	\pre ��̬���ԣ� _pDst ���ڽ�ָ�롣
	\pre ���ԣ� dynamic_cast �ɹ���
	\return �ǿս����
	\note ��ָ����Ϊ����һ��ʧ�ܡ�
	*/
	template<typename _pDst, class _tSrc>
	inline _pDst
		polymorphic_crosscast(_tSrc* v)
	{
		static_assert(is_polymorphic<_tSrc>(), "Non-polymorphic class found.");
		static_assert(is_pointer<_pDst>(), "Non-pointer destination found.");

		auto p(dynamic_cast<_pDst>(v));

		wassume(p);
		return p;
	}
	/*!
	\tparam _tSrc Դ���͡�
	\tparam _rDst Ŀ�����͡�
	\pre ��̬���ԣ� _rDst ����ֵ���á�
	*/
	template<typename _rDst, class _tSrc>
	wconstfn _rDst
		polymorphic_crosscast(_tSrc& v)
	{
		static_assert(is_lvalue_reference<_rDst>(),
			"Invalid destination type found.");

		return *polymorphic_crosscast<remove_reference_t<_rDst>*>(
			std::addressof(v));
	}
	/*!
	\tparam _tSrc Դ���͡�
	\tparam _rDst Ŀ�����͡�
	\pre ��̬���ԣ� _rDst ����ֵ���á�
	*/
	template<typename _rDst, class _tSrc>
	wconstfn enable_if_t<!is_reference<_tSrc>::value, _rDst>
		polymorphic_crosscast(_tSrc&& v)
	{
		static_assert(is_rvalue_reference<_rDst>(),
			"Invalid destination type found.");

		return std::move(polymorphic_crosscast<_rDst&>(v));
	}
	//@}


	namespace details
	{

		//@{
		template<typename _tFrom, typename _tTo, bool _bNonVirtualDownCast>
		struct general_polymorphic_cast_helper
		{
			static wconstfn _tTo
				cast(_tFrom v)
			{
				return polymorphic_downcast<_tTo>(v);
			}
		};

		template<typename _tFrom, typename _tTo>
		struct general_polymorphic_cast_helper<_tFrom, _tTo, false>
		{
			static wconstfn _tTo
				cast(_tFrom v)
			{
				return dynamic_cast<_tTo>(v);
			}
		};


		template<typename _tFrom, typename _tTo, bool _bUseStaticCast>
		struct general_cast_helper
		{
			static wconstfn _tTo
				cast(_tFrom v)
			{
				return _tTo(v);
			}
		};

		template<typename _tFrom, typename _tTo>
		struct general_cast_helper<_tFrom, _tTo, false>
		{
			static wconstfn _tTo
				cast(_tFrom v)
			{
				return general_polymorphic_cast_helper<_tFrom, _tTo, and_<is_base_of<
					_tFrom, _tTo>, not_<has_common_nonempty_virtual_base<
					_t<remove_rp<_tFrom>>, _t<remove_rp<_tTo>>>>>::value>::cast(v);
			}
		};

		template<typename _type>
		struct general_cast_helper<_type, _type, true>
		{
			static _type
				cast(_type v)
			{
				return v;
			}
		};

		template<typename _type>
		struct general_cast_helper<_type, _type, false>
		{
			static wconstfn _type
				cast(_type v)
			{
				return v;
			}
		};

		template<typename _tFrom, typename _tTo>
		struct general_cast_type_helper : is_convertible<_tFrom, _tTo>
		{};
		//@}

	} // namespace details;

	  /*!
	  \brief һ������ת����
	  \tparam _tSrc Դ���͡�
	  \tparam _tDst Ŀ�����͡�
	  \todo ��չ������ֵ���ò�����

	  ��ȷ����ȫ��ʽת��ʱʹ�� static_cast ��
	  ����֮����������������ת��ʹ�� polymophic_downcast��
	  ����ʹ�� dynamic_cast��
	  */
	  //@{
	template<typename _tDst, typename _tSrc>
	wconstfn _tDst
		general_cast(_tSrc* v)
	{
		return details::general_cast_helper<_tSrc*, _tDst,
			details::general_cast_type_helper<_tSrc*, _tDst>::value>::cast(v);
	}
	template<typename _tDst, typename _tSrc>
	wconstfn _tDst
		general_cast(_tSrc& v)
	{
		return details::general_cast_helper<_tSrc&, _tDst,
			details::general_cast_type_helper<_tSrc&, _tDst>::value>::cast(v);
	}
	template<typename _tDst, typename _tSrc>
	wconstfn const _tDst
		general_cast(const _tSrc& v)
	{
		return details::general_cast_helper<const _tSrc&, _tDst, details
			::general_cast_type_helper<const _tSrc&, const _tDst>::value>::cast(v);
	}
	//@}
	//@}
}

#endif
