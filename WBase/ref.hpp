/*!	\file ref.hpp
\ingroup WBase
\brief ���ð�װ��
��չ��׼��ͷ <functional> ���ṩ��� std::reference_wrapper �Ľӿڡ�
*/

#ifndef WBase_ref_hpp
#define WBase_ref_hpp 1

#include "addressof.hpp"// for "addressof.hpp", white::constfn_addressof,
//	exclude_self_t, cond_t, not_, is_object;
#include <functional> // for std::reference_wrapper;

namespace white
{
	//!
	//@{
	/*!
	\brief ��ֵ���ð�װ��
	\tparam _type ����װ�����͡�

	���� \c std::reference_wrapper �� \c boost::reference_wrapper �����ӿڼ��ݵ�
	���ð�װ��ʵ�֡�
	�� \c std::reference_wrapper ��ͬ���� \c boost::reference_wrapper ���ƣ�
	��Ҫ��ģ�����Ϊ�������͡�
	*/
	//@{
	template<typename _type>
	class lref
	{
	public:
		using type = _type;

	private:
		_type* ptr;

	public:
		wconstfn
			lref(_type& t) wnothrow
			: ptr(white::addressof(t))
		{}
		wconstfn
			lref(std::reference_wrapper<_type> t) wnothrow
			: lref(t.get())
		{}

		//! \since build 556
		lref(const lref&) = default;

		//! \since build 556
		lref&
			operator=(const lref&) = default;

		operator _type&() const wnothrow
		{
			return *ptr;
		}

		_type&
			get() const wnothrow
		{
			return *ptr;
		}
	};

	/*!
	\brief �������ð�װ��
	\relates lref
	*/
	//@{
	template<typename _type>
	wconstfn lref<_type>
		ref(_type& t)
	{
		return lref<_type>(t);
	}
	template <class _type>
	void
		ref(const _type&&) = delete;

	template<typename _type>
	wconstfn lref<const _type>
		cref(const _type& t)
	{
		return lref<const _type>(t);
	}
	template<class _type>
	void
		cref(const _type&&) = delete;
	//@}
	//@}

	//! \since build 1.4
	//@{
	/*!
	\ingroup unary_type_traits
	\brief �ж�ģ�����ָ���������Ƿ�
	\note �ӿں������� boost::is_reference_wrapper ��
	*/
	//@{
	template<typename _type>
	struct is_reference_wrapper : false_type
	{};

	template<typename _type>
	struct is_reference_wrapper<std::reference_wrapper<_type>> : true_type
	{};

	template<typename _type>
	struct is_reference_wrapper<lref<_type>> : true_type
	{};
	//@}

	/*!
	\ingroup metafunctions
	\brief ȡ���ð�װ�����ͻ�δ����װ��ģ��������͡�
	\note �ӿں������� boost::unwrap_reference ��
	\since build 1.4
	*/
	//@{
	template<typename _type>
	struct unwrap_reference
	{
		using type = _type;
	};

	template<typename _type>
	using unwrap_reference_t = _t<unwrap_reference<_type>>;

	template<typename _type>
	struct unwrap_reference<std::reference_wrapper<_type>>
	{
		using type = _type;
	};

	template<typename _type>
	struct unwrap_reference<lref<_type>>
	{
		using type = _type;
	};
	//@}
	//@}


	//@{
	template<typename _type>
	struct decay_unwrap : unwrap_reference<decay_t<_type>>
	{};

	template<typename _type>
	using decay_unwrap_t = _t<decay_unwrap<_type>>;
	//@}
	//@}


	/*!
	\brief ��װ��Ӳ�����������������
	*/
	template<typename _type, typename _tReference = lref<_type>>
	class indirect_ref_adaptor
	{
	public:
		using value_type = _type;
		using reference = _tReference;

	private:
		reference ref;

	public:
		indirect_ref_adaptor(value_type& r)
			: ref(r)
		{}

		auto
			get() wnothrow -> decltype(ref.get().get())
		{
			return ref.get().get();
		}
	};


	/*!
	\brief ������ð�װ��
	\note Ĭ�Ͻ��ṩ�� \c std::reference_wrapper �� lref ��ʵ�����͵����ء�
	\note ʹ�� ADL ��
	*/
	//@{
	template<typename _type>
	wconstfn _type&
		unref(_type&& x) wnothrow
	{
		return x;
	}
	//! \since build 554
	template<typename _type>
	wconstfn _type&
		unref(const lref<_type>& x) wnothrow
	{
		return x.get();
	}
	//@}


	/*!
	\brief ��������������͡�
	\warning ����� cv-qualifier ��
	\todo ��ֵ���ð汾��
	*/
	class void_ref
	{
	private:
		const volatile void* ptr;

	public:
		template<typename _type>
		wconstfn
			void_ref(_type& obj)
			: ptr(&obj)
		{}

		template<typename _type>
		wconstfn WB_PURE
			operator _type&() const
		{
			return *static_cast<_type*>(&*this);
		}

		WB_PURE void*
			operator&() const volatile
		{
			return const_cast<void*>(ptr);
		}
	};

	/*!
	\brief α�������
	\note �������и�ֵ������
	\since build 1.4
	*/
	struct pseudo_output
	{
		//! \since build 690
		//@{
		template<typename... _tParams>
		wconstfn
			pseudo_output(_tParams&&...) wnothrow
		{}

		template<typename _tParam,
			wimpl(exclude_self_t<pseudo_output, _tParam>)>
			wconstfn const pseudo_output&
			operator=(_tParam&&) const wnothrow
		{
			return *this;
		}
		template<typename... _tParams>
		wconstfn const pseudo_output&
			operator()(_tParams&&...) const wnothrow
		{
			return *this;
		}
		//@}
	};

	/*!
	\ingroup metafunctions
	\since build 1.4
	\see ������صĺ����������ԣ� WG21 P0146R0 ��
	*/
	//@{
	//! \brief �����Ͳ��ǿ�������ȡ�󱸽�����ͣ�Ĭ��Ϊ pseudo_output ����
	template<typename _type, typename _tRes = pseudo_output>
	using nonvoid_result_t = cond_t<not_<is_void<_type>>, _type, pseudo_output>;

	//! \brief �����Ͳ��Ƕ���������ȡ�󱸽�����ͣ�Ĭ��Ϊ pseudo_output ����
	template<typename _type, typename _tRes = pseudo_output>
	using object_result_t = cond_t<is_object<_type>, _type, pseudo_output>;
	//@}

}

#endif
