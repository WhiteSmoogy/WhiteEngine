/*! \file memory.hpp
\ingroup WBase
\brief �洢������ָ�����ԡ�

��չ��׼��ͷ <memory> ���ṩ������ָ�����͵Ĳ������жϵ��������ɽ����õ�ģ�塣
*/

#ifndef WBase_memory_Hpp
#define WBase_memory_Hpp 1

#include "exception.h"
#include "placement.hpp" // for "addressof.hpp", and_,
//	is_copy_constructible, is_class_type, cond_t, vdefer, detected_t,
//	conditional, ::constfn_addressof, indirect_element_t,
//	remove_reference_t, detected_or_t, is_void, remove_pointer_t,
//	is_pointer, enable_if_t, is_array, extent, remove_extent_t;
#include "type_op.hpp" // for is_class_type, is_nonconst_object;
#include "pointer.hpp" // for is_undereferenceable;
#include "exception.h" // for throw_invalid_construction;
#include "ref.hpp" // for totally_ordered, equality_comparable;
#include <new> // for placement ::operator new from standard library;
#include <memory>

#if WB_IMPL_MSCPP >= 1800
/*!
\brief \<memory\> ���Բ��Ժꡣ
\see WG21 P0096R1 3.5 ��
*/
#	ifndef __cpp_lib_make_unique
#		define __cpp_lib_make_unique 201304
#	endif
#endif

using stdex::nullptr_t;

namespace white
{
	//@{
	namespace details
	{

		template<typename _type>
		using is_copy_constructible_class
			= and_<is_copy_constructible<_type>, is_class_type<_type>>;


		//! \since build 1.4
		template<class _tAlloc, typename _type>
		using rebind_t = typename _tAlloc::template rebind<_type>::other;

		// XXX: Details member across headers?
		//! \since build 1.4
		template<typename _type>
		using check_allocator = and_<cond_t<has_mem_value_type<_type>,
			is_detected<rebind_t, _type, vdefer<details::mem_value_type_t, _type>>>,
			is_copy_constructible_class<_type>>;

		//! \since build 1.4
		template<typename _type>
		using nested_allocator_t = typename _type::allocator_type;

		//@{
		template<typename _type, typename... _tParams>
		using mem_new_t
			= decltype(_type::operator new(std::declval<_tParams>()...));

		template<typename _type, typename... _tParams>
		using mem_delete_t
			= decltype(_type::operator delete(std::declval<_tParams>()...));
		//@}

	} // namespace details;
	//@}

	//@{
	template<typename _type, typename... _tParams>
	struct has_mem_new : is_detected<details::mem_new_t, _type, _tParams...>
	{};

	template<typename _type, typename... _tParams>
	struct has_mem_delete : is_detected<details::mem_delete_t, _type, _tParams...>
	{};
	//@}

	/*!
	\ingroup unary_type_traits
	\brief �ж������Ƿ���Ϸ�����Ҫ���Ŀ�����͡�
	\since build 1.4
	*/
	template<typename _type>
	struct is_allocatable : is_nonconst_object<_type>
	{};


	/*!
	\ingroup unary_type_traits
	\brief �ж����;���Ƕ�׵ĳ�Ա allocator_type ָ��һ���ɸ��ƹ���������͡�
	*/
	template<typename _type>
	struct has_nested_allocator
		: details::check_allocator<detected_t<details::nested_allocator_t, _type>>
	{};


	/*!
	\ingroup metafunctions
	\brief ȡǶ�׳�Ա���������ͣ�����������ʹ�õڶ�ģ�����ָ����Ĭ�����͡�
	*/
	template<typename _type, class _tDefault = std::allocator<_type>>
	struct nested_allocator
		: conditional<has_nested_allocator<_type>::value,
		detected_t<details::nested_allocator_t, _type>, _tDefault>
	{};

	/*!
	\brief �ͷŷ�������ɾ������
	\since build 1.4
	*/
	template<class _tAlloc>
	class allocator_delete
	{
	private:
		using traits = std::allocator_traits<_tAlloc>;

	public:
		using pointer = typename traits::pointer;
		using size_type = typename traits::size_type;

	private:
		_tAlloc & alloc_ref;
		size_type size;

	public:
		allocator_delete(_tAlloc& alloc, size_type s)
			: alloc_ref(alloc), size(s)
		{}

		void
			operator()(pointer p) const wnothrowv
		{
			traits::deallocate(alloc_ref, p, size);
		}
	};


	/*!
	\brief ����������ػ���
	*/
	template<typename _tAlloc>
	std::unique_ptr<_tAlloc, allocator_delete<_tAlloc>>
		make_allocator_guard(_tAlloc& a,
			typename std::allocator_traits<_tAlloc>::size_type n = 1)
	{
		using del_t = allocator_delete<_tAlloc>;

		return std::unique_ptr<_tAlloc, del_t>(
			std::allocator_traits<_tAlloc>::allocate(a, n), del_t(a, n));
	}


	//@{
	//! \brief ʹ�÷�������������
	template<typename _type, class _tAlloc, typename... _tParams>
	auto
		create_with_allocator(_tAlloc&& a, _tParams&&... args)
		-> decltype(white::make_allocator_guard(a).release())
	{
		auto gd(white::make_allocator_guard(a));

		white::construct_within<typename std::allocator_traits<decay_t<_tAlloc>>
			::value_type>(*gd.get(), wforward(args)...);
		return gd.release();
	}


	//! \ingroup allocators
	//@{
	/*!
	\brief ���������
	\warning ����������

	�� std::allocator ���Ƶ��Զ������ĳ�Ա���� ::operator new/::operator delete ��
	*/
	template<class _type>
	struct class_allocator : std::allocator<_type>
	{
		class_allocator() = default;
		using std::allocator<_type>::allocator;
		class_allocator(const class_allocator&) = default;

		_type*
			allocate(size_t n)
		{
			return _type::operator new(n * sizeof(_type));
		}

		void
			deallocate(_type* p, size_t)
		{
			// TODO: What if the size hint is supported by %_type?
			return _type::operator delete(p);
		}
	};


	/*!
	\brief �ֲ���������

	��֧��������������������Ϊ class_allocator ������Ϊ std::allocator ��
	*/
	template<typename _type>
	using local_allocator = cond_t<and_<has_mem_new<_type, size_t>,
		has_mem_delete<_type*>>, class_allocator<_type>, std::allocator<_type>>;
	//@}
	//@}

	namespace details
	{

		template<typename _type, typename... _tParams>
		WB_NORETURN _type*
			try_new_impl(void*, _tParams&&...)
		{
			throw_invalid_construction();
		}
		template<typename _type, typename... _tParams>
		auto
			try_new_impl(nullptr_t, _tParams&&... args)
			-> decltype(new _type(wforward(args)...))
		{
			return new _type(wforward(args)...);
		}

		template<typename _type, class _tAlloc, typename... _tParams>
		WB_NORETURN _type*
			try_create_with_allocator_impl(void*, _tAlloc&, _tParams&&...)
		{
			throw_invalid_construction();
		}
		template<typename _type, class _tAlloc, typename... _tParams>
		auto
			try_create_with_allocator_impl(nullptr_t, _tAlloc& a, _tParams&&... args)
			-> decltype(white::create_with_allocator<_type>(a, wforward(args)...))
		{
			return white::create_with_allocator<_type>(a, wforward(args)...);
		}

		//@{
		template<typename _type>
		struct pack_obj_impl
		{
			template<typename... _tParams>
			static _type
				pack(_tParams&&... args)
			{
				return _type(wforward(args)...);
			}
		};

		template<typename _type, class _tDeleter>
		struct pack_obj_impl<std::unique_ptr<_type, _tDeleter>>
		{
			template<typename... _tParams>
			static std::unique_ptr<_type>
				pack(_tParams&&... args)
			{
				return std::unique_ptr<_type>(wforward(args)...);
			}
		};

		template<typename _type>
		struct pack_obj_impl<std::shared_ptr<_type>>
		{
			template<typename... _tParams>
			static std::shared_ptr<_type>
				pack(_tParams&&... args)
			{
				return std::shared_ptr<_type>(wforward(args)...);
			}
		};
		//@}

	} // namespace details;


	  //! \throw invalid_construction ���÷Ǻ�ʽ��
	  //@{
	  /*!
	  \brief ���Ե��� new ���ʽ��������
	  */
	template<typename _type, typename... _tParams>
	_type*
		try_new(_tParams&&... args)
	{
		return details::try_new_impl<_type>(nullptr, wforward(args)...);
	}

	/*!
	\brief ���Ե��� white::create_with_allocator ���ʽ��������
	*/
	template<typename _type, class _tAlloc, typename... _tParams>
	_type*
		try_create_with_allocator(_tAlloc&& a, _tParams&&... args)
	{
		return details::try_create_with_allocator_impl<_type>(nullptr, a,
			wforward(args)...);
	}

	/*!
	\ingroup unary_type_traits
	\brief �ж�ָ�������Ƿ�Ϊ��֪�ĳ��й�������Ȩ�Ķ������͡�
	\tparam _type ��Ҫ�ж����������Ͳ�����
	\note �û��ɼ̳д��������ػ�������������չ�µ����͡�
	*/
	//@{
	template<typename _type>
	struct is_sharing : false_
	{};

	template<typename _type>
	struct is_sharing<std::shared_ptr<_type>> : true_
	{};

	template<typename _type>
	struct is_sharing<std::weak_ptr<_type>> : true_
	{};
	//@}

	/*!
	\brief ʹ����ʽ�����������ú� std::free ��ɾ������
	\note �������͵��ػ��޶��塣
	\since build 1.4

	��ʹ�� std::free ���� \c ::operator delete���� std::default_deleter
	�ķ�����������ͬ��ע���ֱ��ʹ�� std::free ��ͬ����������������Ҳ����������顣
	*/
	//@{
	template<typename _type>
	struct free_delete
	{
		wconstfn free_delete() wnothrow = default;
		template<typename _type2,
			wimpl(typename = enable_if_convertible_t<_type2*, _type*>)>
			free_delete(const free_delete<_type2>&) wnothrow
		{}

		//! \since build 1.4
		void
			operator()(_type* p) const wnothrowv
		{
			p->~_type();
			std::free(p);
		}
	};

	template<typename _type>
	struct free_delete<_type[]>
	{
		//! \since build 1.4
		//@{
		static_assert(is_trivially_destructible<_type>::value, "Invalid type found");

		wconstfn free_delete() wnothrow = default;
		template<typename _type2,
			wimpl(typename = enable_if_convertible_t<_type2(*)[], _type(*)[]>)>
			free_delete(const free_delete<_type2[]>&) wnothrow
		{}

		template<typename _type2,
			wimpl(typename = enable_if_convertible_t<_type2(*)[], _type(*)[]>)>
			void
			operator()(_type2* p) const wnothrowv
		{
			std::free(p);
		}
		//@}
	};
	//@}


	template<class _type> inline
		void return_temporary_buffer(_type * buff)
	{	// delete raw temporary buffer
		::operator delete(buff);
	}

	/*!
	\brief ʹ����ʽ return_temporary_buffer ��ɾ������
	\note ���������͵��ػ��޶��塣
	\since build 1.4
	*/
	//@{
	template<typename>
	struct temporary_buffer_delete;

	template<typename _type>
	struct temporary_buffer_delete<_type[]>
	{
		wconstfn temporary_buffer_delete() wnothrow = default;

		void
			operator()(_type* p) const wnothrowv
		{
			white::return_temporary_buffer(p);
		}
	};
	//@}

	template<class _type>
	std::pair<_type *, ptrdiff_t> get_temporary_buffer(ptrdiff_t count) wnoexcept
	{	// get raw temporary buffer of up to _Count elements
		if (static_cast<size_t>(count) <= static_cast<size_t>(-1) / sizeof(_type))
		{
			for (; 0 < count; count /= 2)
			{
				_type * buff = static_cast<_type *>(::operator new(static_cast<size_t>(count) * sizeof(_type), std::nothrow));
				if (buff)
				{
					return { buff, count };
				}
			}
		}
		return { nullptr, 0 };
	}

	/*!
	\brief ��ʱ��������
	\since build 1.4
	*/
	//@{
	template<typename _type>
	class temporary_buffer
	{
	public:
		using array_type = _type[];
		using element_type = _type;
		using deleter = temporary_buffer_delete<array_type>;
		using pointer = std::unique_ptr<_type, deleter>;

	private:
		std::pair<pointer, size_t> buf;

	public:
		//! \throw std::bad_cast ȡ��ʱ�洢ʧ�ܡ�
		temporary_buffer(size_t n)
			: buf([n] {
			// NOTE: See LWG 2072.
			const auto pr(white::get_temporary_buffer<_type>(ptrdiff_t(n)));

			if (pr.first)
				return std::pair<pointer, size_t>(pointer(pr.first),
					size_t(pr.second));
			throw std::bad_cast();
		}())
		{}
		temporary_buffer(temporary_buffer&&) = default;
		temporary_buffer&
			operator=(temporary_buffer&&) = default;

		//! \since build 1.4
		wconstfn const pointer&
			get() const wnothrow
		{
			return buf.first;
		}

		pointer&
			get_pointer_ref() wnothrow
		{
			return buf.first;
		}

		size_t
			size() const wnothrow
		{
			return buf.second;
		}
	};
	//@}




	/*!
	\ingroup metafunctions
	\since build 1.4
	*/
	//@{
	//! \brief ȡɾ������ \c pointer ��Ա���͡�
	template<class _tDeleter>
	using deleter_member_pointer_t
		= typename remove_reference_t<_tDeleter>::pointer;

	//! \brief ȡ unique_ptr ��װ��ָ�����͡�
	template<typename _type, class _tDeleter>
	using unique_ptr_pointer
		= detected_or_t<_type*, deleter_member_pointer_t, _tDeleter>;

	/*!
	\brief ȡָ���Ӧ��Ԫ�����͡�

	��ָ��Ϊ��ָ��ʱΪ \c void �������Ӳ�����Ԫ�����͡�
	*/
	template<typename _tPointer, typename _tDefault = void>
	using defer_element = cond_t<is_void<remove_pointer_t<_tPointer>>, _tDefault,
		vdefer<indirect_element_t, _tPointer>>;
	//@}

	


	

	/*!
	\ingroup helper_functions
	\tparam _type ��ָ�����͡�
	*/
	//@{
	/*!
		\brief ʹ��ָ�����͵ĳ�ʼ���б���ָ�����͵� std::shared_ptr ����
		\tparam _tValue ��ʼ���б��Ԫ�����͡�
	*/
	template<typename _type, typename _tValue>
	wconstfn std::shared_ptr<_type>
		make_shared(std::initializer_list<_tValue> il)
	{
		return std::make_shared<_type>(il);
	}

	/*!
	\brief ʹ��ָ���� std::shared_ptr ʵ���Ķ��󴴽���Ӧ�� std::weak_ptr ʵ���Ķ���
	*/
	template<typename _type>
	inline std::weak_ptr<_type>
		make_weak(const std::shared_ptr<_type>& p) wnothrow
	{
		return p;
	}
	//@}

	/*!
	\note ʹ�� ADL make_shared ��
	*/
	//@{
	//! \brief ����ֵ������Ӧ�� std::shared_ptr ʵ���Ķ���
	template<typename _tValue, typename _type = _tValue>
	wconstfn std::shared_ptr<decay_t<_type>>
		share_copy(const _tValue& v)
	{
		using std::make_shared;

		return make_shared<decay_t<_type>>(wforward(v));
	}

	//! \brief ����ֵ������Ӧ�� std::shared_ptr ʵ���Ķ���
	template<typename _tValue, typename _type = _tValue>
	wconstfn std::shared_ptr<decay_t<_type>>
		share_forward(_tValue&& v)
	{
		using std::make_shared;

		return make_shared<decay_t<_type>>(wforward(v));
	}

	//! \brief ת��ֵ������Ӧ�� std::shared_ptr ʵ���Ķ���
	template<typename _tValue, typename _type = _tValue>
	wconstfn std::shared_ptr<decay_t<_type>>
		share_move(_tValue&& v)
	{
		using std::make_shared;

		return make_shared<decay_t<_type>>(std::move(v));
	}
	//@}

	/*!
	\brief ����ָ��ת����
	\since build 1.4
	*/
	//@{
	template<typename _tDst, typename _type>
	std::unique_ptr<_tDst>
		static_pointer_cast(std::unique_ptr<_type> p) wnothrow
	{
		return std::unique_ptr<_tDst>(static_cast<_tDst*>(p.release()));
	}
	template<typename _tDst, typename _type, typename _tDeleter>
	std::unique_ptr<_tDst, _tDeleter>
		static_pointer_cast(std::unique_ptr<_type, _tDeleter> p) wnothrow
	{
		return std::unique_ptr<_tDst, _tDeleter>(static_cast<_tDst*>(p.release()),
			std::move(p.get_deleter()));
	}

	template<typename _tDst, typename _type>
	std::unique_ptr<_tDst>
		dynamic_pointer_cast(std::unique_ptr<_type>& p) wnothrow
	{
		if (auto p_res = dynamic_cast<_tDst*>(p.get()))
		{
			p.release();
			return std::unique_ptr<_tDst>(p_res);
		}
		return std::unique_ptr<_tDst>();
	}
	template<typename _tDst, typename _type>
	std::unique_ptr<_tDst>
		dynamic_pointer_cast(std::unique_ptr<_type>&& p) wnothrow
	{
		return white::dynamic_pointer_cast<_tDst, _type>(p);
	}
	template<typename _tDst, typename _type, typename _tDeleter>
	std::unique_ptr<_tDst, _tDeleter>
		dynamic_pointer_cast(std::unique_ptr<_type, _tDeleter>& p) wnothrow
	{
		if (auto p_res = dynamic_cast<_tDst*>(p.get()))
		{
			p.release();
			return std::unique_ptr<_tDst, _tDeleter>(p_res);
		}
		return std::unique_ptr<_tDst, _tDeleter>(nullptr);
	}
	template<typename _tDst, typename _type, typename _tDeleter>
	std::unique_ptr<_tDst, _tDeleter>
		dynamic_pointer_cast(std::unique_ptr<_type, _tDeleter>&& p) wnothrow
	{
		return white::dynamic_pointer_cast<_tDst, _type, _tDeleter>(p);
	}

	template<typename _tDst, typename _type>
	std::unique_ptr<_tDst>
		const_pointer_cast(std::unique_ptr<_type> p) wnothrow
	{
		return std::unique_ptr<_tDst>(const_cast<_tDst*>(p.release()));
	}
	template<typename _tDst, typename _type, typename _tDeleter>
	std::unique_ptr<_tDst, _tDeleter>
		const_pointer_cast(std::unique_ptr<_type, _tDeleter> p) wnothrow
	{
		return std::unique_ptr<_tDst, _tDeleter>(const_cast<_tDst*>(p.release()),
			std::move(p.get_deleter()));
	}
	//@}

	//! \since build 1.4
	//@{
	//! \brief ʹ�� new ���ƶ���
	template<typename _type>
	inline _type*
		clone_monomorphic(const _type& v)
	{
		return new _type(v);
	}
	//! \brief ʹ�÷��������ƶ���
	template<typename _type, class _tAlloc>
	inline auto
		clone_monomorphic(const _type& v, _tAlloc&& a)
		-> decltype(white::create_with_allocator<_type>(wforward(a), v))
	{
		return white::create_with_allocator<_type>(wforward(a), v);
	}

	//! \brief ��ָ��ǿգ�ʹ�� new ����ָ��ָ��Ķ���
	template<typename _tPointer>
	inline auto
		clone_monomorphic_ptr(const _tPointer& p)
		-> decltype(white::clone_monomorphic(*p))
	{
		using ptr_t = decltype(white::clone_monomorphic(*p));

		return p ? white::clone_monomorphic(*p) : ptr_t();
	}
	//! \brief ��ָ��ǿգ�ʹ�÷���������ָ��ָ��Ķ���
	template<typename _tPointer, class _tAlloc>
	inline auto
		clone_monomorphic_ptr(const _tPointer& p, _tAlloc&& a)
		-> decltype(white::clone_monomorphic(*p, wforward(a)))
	{
		using ptr_t = decltype(white::clone_monomorphic(*p, wforward(a)));

		return p ? white::clone_monomorphic(*p, wforward(a)) : ptr_t();
	}

	/*!
	\brief ʹ�� \c clone ��Ա�������ƶ�̬�����Ͷ���
	\pre ��̬���ԣ� <tt>is_polymorphic<decltype(v)>()</tt> ��
	*/
	template<class _type>
	inline auto
		clone_polymorphic(const _type& v) -> decltype(v.clone())
	{
		static_assert(is_polymorphic<_type>(), "Non-polymorphic class type found.");

		return v.clone();
	}

	//! \brief ��ָ��ǿգ�ʹ�� \c clone ��Ա��������ָ��ָ��Ķ�̬�����Ͷ���
	template<class _tPointer>
	inline auto
		clone_polymorphic_ptr(const _tPointer& p) -> decltype(clone_polymorphic(*p))
	{
		return
			p ? white::clone_polymorphic(*p) : decltype(clone_polymorphic(*p))();
	}
	//@}

	//! \since build 1.4
	//@{
	/*!
	\brief �������ͨ��ָ�������������
	\since build

	�� std::unique_ptr �� std::shared_ptr ��ʵ����ʹ�� make_unique
	�� std::make_shared ���죬����ֱ��ʹ�ò������졣
	*/
	template<typename _type, typename... _tParams>
	auto
		pack_object(_tParams&&... args)
		-> decltype(wimpl(details::pack_obj_impl<_type>::pack(wforward(args)...)))
	{
		return details::pack_obj_impl<_type>::pack(wforward(args)...);
	}


	//! \brief ����Ķ���ʹ�� pack_object �õ��Ķ����װ��
	template<typename _type, typename _tPack = std::unique_ptr<_type>>
	struct object_pack
	{
	public:
		using value_type = _type;
		using pack_type = _tPack;

	private:
		pack_type pack;

	public:
		//! \note ʹ�� ADL pack_object ��
		template<typename... _tParams>
		object_pack(_tParams&&... args)
			: pack(pack_object<_tPack>(wforward(args)...))
		{}

		operator pack_type&() wnothrow
		{
			return pack;
		}
		operator const pack_type&() const wnothrow
		{
			return pack;
		}
	};
	//@}
}




#endif