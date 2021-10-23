/*! \file any.h
\ingroup WBase
\brief ��̬�������͡�
\par �޸�ʱ��:
2017-01-11 14:00 +0800

\see WG21 N4582 20.6[any] ��
\see http://www.boost.org/doc/libs/1_60_0/doc/html/any/reference.html ��
*/

#ifndef WBase_any_h
#define WBase_any_h 1

#include "typeinfo.h" //for "typeinfo.h����cloneable,type_id_info,
// white::type_id,std::bad_cast
#include "memory.hpp" //for white::addressof
#include "utility.hpp" // "utility.hpp", for boxed_value,
//	standard_layout_storage, aligned_storage_t, is_aligned_storable,
//	exclude_self_t, enable_if_t, decay_t, wconstraint;
#include "exception.h"
#include "ref.hpp" // for is_reference_wrapper, unwrap_reference_t;
#include "placement.hpp"
#include <initializer_list>
#include <memory> //std::unique_ptr

namespace white
{
	/*!
	\brief any �����������ռ䡣
	\sa any
	\since change 1.4
	*/
	namespace any_ops
	{
		//! \since build 1.4
		template<typename>
		struct with_handler_t
		{};


		/*!
		\brief ����̬���ͳ����߽ӿڡ�
		\since build 1.4
		*/
		class WB_API holder 
		{
		public:
			//! \since build 1.4
			//@{
			holder() = default;
			holder(const holder&) = default;
			//! \brief ���������ඨ����Ĭ��ʵ�֡�
			virtual ~holder();
			//@}

			virtual void*
				get() const = 0;

			//! \since build 1.4
			virtual const type_info&
				type() const wnothrow = 0;
		};

		/*!
		\brief �ж��Ƿ������ͬ����
		\relates holder
		*/
		inline bool
			hold_same(const holder& x, const holder& y)
		{
			return x.get() == y.get();
		}

		/*!
		\brief ֵ���Ͷ�̬���ͳ����ߡ�
		\pre ֵ���Ͳ��� cv-qualifier ���Ρ�
		*/
		template<typename _type>
		class value_holder : protected boxed_value<_type>, public holder
		{
			static_assert(is_object<_type>::value, "Non-object type found.");
			static_assert(!is_cv<_type>::value, "Cv-qualified type found.");

		public:
			using value_type = _type;

			//! \since build 1.4
			//@{
			value_holder() = default;
			template<typename _tParam,
				wimpl(typename = exclude_self_t<value_holder, _tParam>)>
				value_holder(_tParam&& arg)
				wnoexcept(is_nothrow_constructible<_type, _tParam&&>())
				: boxed_value<_type>(wforward(arg))
			{}
			using boxed_value<_type>::boxed_value;
			//@}
			//! \since build 1.4
			//@{
			value_holder(const value_holder&) = default;
			value_holder(value_holder&&) = default;

			value_holder&
				operator=(const value_holder&) = default;
			value_holder&
				operator=(value_holder&&) = default;
			//@}

			value_holder*
				clone() const override
			{
				return white::clone_monomorphic(*this);
			}

			//! \since build 1.3
			void*
				get() const override
			{
				return addressof(this->value);
			}

			//! \since build 1.4
			const type_info&
				type() const wnothrow override
			{
				return white::type_id<_type>();
			}
		};


		/*!
		\brief ָ�����Ͷ�̬���ͳ����ߡ�
		\tparam _type �������͡�
		\tparam _tPointer ����ָ�����͡�
		\pre _tPointer ���� _type ��������Ȩ��
		\pre ��̬���ԣ� <tt>is_object<_type>()</tt> ��
		\since build 1.4
		*/
		template<typename _type, class _tPointer = std::unique_ptr<_type>>
		class pointer_holder : public holder
		{
			static_assert(is_object<_type>::value, "Invalid type found.");

		public:
			using value_type = _type;
			using holder_pointer = _tPointer;
			using pointer = typename holder_pointer::pointer;

		protected:
			holder_pointer p_held;

		public:
			//! \brief ȡ������Ȩ��
			pointer_holder(pointer value)
				: p_held(value)
			{}
			//! \since build 1.3
			//@{
			pointer_holder(const pointer_holder& h)
				: pointer_holder(white::clone_monomorphic(h.p_held))
			{}
			pointer_holder(pointer_holder&&) = default;
			//@}

			pointer_holder&
				operator=(const pointer_holder&) = default;
			pointer_holder&
				operator=(pointer_holder&&) = default;

			pointer_holder*
				clone() const override
			{
				return white::clone_monomorphic(*this);
			}

			void*
				get() const override
			{
				return p_held.get();
			}

			const type_info&
				type() const wnothrow override
			{
				return p_held ? white::type_id<_type>() : white::type_id<void>();
			}
		};


		//! \since build 1.3
		using op_code = std::uint32_t;

		//! \since build 1.3
		enum base_op : op_code
		{
			//! \since build 1.3
			no_op,
			get_type,
			get_ptr,
			clone,
			destroy,
			get_holder_type,
			get_holder_ptr,
			//! \since build 1,3
			end_base_op
		};


		//! \since build 1.3
		using any_storage
			= standard_layout_storage<aligned_storage_t<sizeof(void*), walignof(void*)>>;
		//! \since build 1.3
		using any_manager = void(*)(any_storage&, any_storage&, op_code);

		/*!
		\brief ʹ��ָ����������ʼ���洢��
		\since build 1.4
		*/
		template<class _tHandler, typename... _tParams>
		any_manager
			construct(any_storage& storage, _tParams&&... args)
		{
			_tHandler::init(storage, wforward(args)...);
			return _tHandler::manage;
		}


		/*!
		\brief ʹ�ó����߱�ǡ�
		\since build 1.4
		*/
		wconstexpr const struct use_holder_t {} use_holder{};


		/*!
		\brief ��̬���Ͷ���������
		\since build 1.4
		*/
		template<typename _type,
			bool _bStoredLocally = and_<is_nothrow_move_constructible<_type>,
			is_aligned_storable<any_storage, _type>>::value>
			class value_handler
		{
		public:
			//! \since build 1.3
			//@{
			using value_type = _type;
			using local_storage = bool_<_bStoredLocally>;
			//@}

			//! \since build 1.4
			//@{
			static void
				copy(any_storage& d, const any_storage& s)
			{
				try_init(is_copy_constructible<value_type>(), local_storage(), d,
					get_reference(s));
			}

			static void
				dispose(any_storage& d) wnothrowv
			{
				dispose_impl(local_storage(), d);
			}

		private:
			static void
				dispose_impl(false_, any_storage& d) wnothrowv
			{
				delete d.access<value_type*>();
			}
			static void
				dispose_impl(true_, any_storage& d) wnothrowv
			{
				d.access<value_type>().~value_type();
			}
			//@}

		public:
			//! \since build 1.4
			//@{
			static value_type*
				get_pointer(any_storage& s)
			{
				return get_pointer_impl(local_storage(), s);
			}
			static const value_type*
				get_pointer(const any_storage& s)
			{
				return get_pointer_impl(local_storage(), s);
			}

		private:
			static value_type*
				get_pointer_impl(false_, any_storage& s)
			{
				return s.access<value_type*>();
			}
			static const value_type*
				get_pointer_impl(false_, const any_storage& s)
			{
				return s.access<const value_type*>();
			}
			static value_type*
				get_pointer_impl(true_, any_storage& s)
			{
				return std::addressof(get_reference_impl(true_(), s));
			}
			static const value_type*
				get_pointer_impl(true_, const any_storage& s)
			{
				return std::addressof(get_reference_impl(true_(), s));
			}

		public:
			static value_type&
				get_reference(any_storage& s)
			{
				return get_reference_impl(local_storage(), s);
			}
			static const value_type&
				get_reference(const any_storage& s)
			{
				return get_reference_impl(local_storage(), s);
			}

		private:
			static value_type&
				get_reference_impl(false_, any_storage& s)
			{
				const auto p(get_pointer_impl(false_(), s));

				wassume(p);
				return *p;
			}
			static const value_type&
				get_reference_impl(false_, const any_storage& s)
			{
				const auto p(get_pointer_impl(false_(), s));

				wassume(p);
				return *p;
			}
			static value_type&
				get_reference_impl(true_, any_storage& s)
			{
				return s.access<value_type>();
			}
			static const value_type&
				get_reference_impl(true_, const any_storage& s)
			{
				return s.access<const value_type>();
			}
			//@}

		public:
			//! \since build 1.4
			//@{
			template<typename... _tParams>
			static void
				init(any_storage& d, _tParams&&... args)
			{
				init_impl(local_storage(), d,wforward(args)...);
			}

		private:
			template<typename... _tParams>
			static WB_ATTR(always_inline) void
				init_impl(false_, any_storage& d, _tParams&&... args)
			{
				d.construct<value_type*>(new value_type(wforward(args)...));
			}
			template<typename... _tParams>
			static WB_ATTR(always_inline) void
				init_impl(true_, any_storage& d, _tParams&&... args)
			{
				d.construct<value_type>(wforward(args)...);
			}
			//@}

		public:
			//! \since build 1.4
			static void
				manage(any_storage& d, any_storage& s, op_code op)
			{
				switch (op)
				{
				case get_type:
					d = &white::type_id<value_type>();
					break;
				case get_ptr:
					d = white::pvoid(get_pointer(s));
					break;
				case clone:
					copy(d, s);
					break;
				case destroy:
					dispose(d);
					break;
				case get_holder_type:
					d = &white::type_id<void>();
					break;
				case get_holder_ptr:
					d = static_cast<holder*>(nullptr);
				}
			}

		private:
			//! \since build 1.4
			//@{
			template<typename... _tParams>
			WB_NORETURN static WB_ATTR(always_inline) void
				try_init(false_, _tParams&&...)
			{
				throw_invalid_construction();
			}
			template<class _bInPlace, typename... _tParams>
			static WB_ATTR(always_inline) void
				try_init(true_, _bInPlace b, any_storage& d, _tParams&&... args)
			{
				init_impl(b, d, wforward(args)...);
			}
			//@}
		};


		/*!
		\brief ��̬�������ô�������
		\since build 1.3
		*/
		template<typename _type>
		class ref_handler : public value_handler<_type*>
		{
		public:
			using value_type = _type;
			using base = value_handler<value_type*>;

			//! \since build 1.4
			static value_type*
				get_pointer(any_storage& s)
			{
				return base::get_reference(s);
			}

			//! \since build 1.4
			static value_type&
				get_reference(any_storage& s)
			{
				wassume(get_pointer(s));
				return *get_pointer(s);
			}

			//! \since build 678
			template<typename _tWrapper,
				wimpl(typename = enable_if_t<is_reference_wrapper<_tWrapper>::value>)>
				static auto
				init(any_storage& d, _tWrapper x)
				-> decltype(base::init(d, addressof(x.get())))
			{
				base::init(d, addressof(x.get()));
			}

			//! \since build 1.4
			static void
				manage(any_storage& d, any_storage& s, op_code op)
			{
				switch (op)
				{
				case get_type:
					d = &white::type_id<value_type>();
					break;
				case get_ptr:
					d =white::pvoid(get_pointer(s));
					break;
				default:
					base::manage(d, s, op);
				}
			}
		};


		/*!
		\brief ��̬���ͳ����ߴ�������
		\since build 1.3
		*/
		template<typename _tHolder>
		class holder_handler : public value_handler<_tHolder>
		{
			static_assert(is_convertible<_tHolder&, holder&>::value,
				"Invalid holder type found.");

		public:
			using value_type = typename _tHolder::value_type;
			using base = value_handler<_tHolder>;

			//! \since build 1.4
			static _tHolder*
				get_holder_pointer(any_storage& s)
			{
				return base::get_pointer(s);
			}

			static value_type*
				get_pointer(any_storage& s)
			{
				const auto p(get_holder_pointer(s));

				wassume(p);
				return static_cast<value_type*>(p->_tHolder::get());
			}

		private:
			//! \since build 1.4
			static void
				init(false_, any_storage& d, std::unique_ptr<_tHolder> p)
			{
				d.construct<value_type*>(p.release());
			}
			//! \since build 1.4
			static void
				init(true_, any_storage& d, std::unique_ptr<_tHolder> p)
			{
				d.construct<_tHolder>(std::move(*p));
			}

		public:
			//! \since build 1.3
			static void
				init(any_storage& d, std::unique_ptr<_tHolder> p)
			{
				init(typename base::local_storage(), d, std::move(p));
			}
			//! \since build 1.4
			using base::init;

			//! \since build 1.4
			static void
				manage(any_storage& d, any_storage& s, op_code op)
			{
				switch (op)
				{
				case get_type:
					d = &type_id<value_type>();
					break;
				case get_ptr:
					d =white::pvoid(get_pointer(s));
					break;
				case get_holder_type:
					d = &type_id<_tHolder>();
					break;
				case get_holder_ptr:
					d = static_cast<holder*>(get_holder_pointer(s));
					break;
				default:
					base::manage(d, s, op);
				}
			}
		};

		template<typename _type>
		struct wrap_handler
		{
			using value_type = remove_reference_t<unwrap_reference_t<_type>>;
			using type = cond_t<is_reference_wrapper<_type>, ref_handler<value_type>,
				value_handler<value_type>>;
		};
	} // namespace any_ops;


	/*!
	\ingroup unary_type_traits
	\brief �ж������Ƿ������Ϊ any ����ת����Ŀ�ꡣ
	\sa any_cast
	\since build 1.4
	*/
	template<typename _type>
	using is_any_cast_dest = or_<is_reference<_type>, is_copy_constructible<_type>>;

	/*!
	\ingroup exceptions
	\brief ��̬����ת��ʧ���쳣��
	\note �����ӿں�����ͬ boost::bad_any_cast ��
	\note �Ǳ�׼���᰸��չ���ṩ��ʶת��ʧ�ܵ�Դ��Ŀ�����͡�
	\sa any_cast
	\see WG21 N4582 20.6.2[any.bad_any_cast] ��
	\since build 1.4
	*/
	class WB_API bad_any_cast : public std::bad_cast
	{
	private:
		lref<const type_info> from_ti, to_ti;

	public:
		//! \since build 1.3
		//@{
		bad_any_cast()
			: std::bad_cast(),
			from_ti(type_id<void>()), to_ti(type_id<void>())
		{}
		//! \since build 1.4
		bad_any_cast(const type_info& from_, const type_info& to_)
			: std::bad_cast(),
			from_ti(from_), to_ti(to_)
		{}
		//! \since build 1.4
		bad_any_cast(const bad_any_cast&) = default;
		/*!
		\brief ���������ඨ����Ĭ��ʵ�֡�
		\since build 1.4
		*/
		~bad_any_cast() override;

		//! \note WBase ��չ��
		//@{
		WB_ATTR_returns_nonnull const char*
			from() const wnothrow;

		//! \since build 1.4
		const type_info&
			from_type() const wnothrow
		{
			return from_ti.get();
		}

		WB_ATTR_returns_nonnull const char*
			to() const wnothrow;

		//! \since build 1.4
		const type_info&
			to_type() const wnothrow
		{
			return to_ti.get();
		}
		//@}
		//@}

		virtual WB_ATTR_returns_nonnull const char*
			what() const wnothrow override;
	};


	//! \since build 1.4
	//@{
	namespace details
	{
		struct any_base
		{
			mutable any_ops::any_storage storage{};
			any_ops::any_manager manager{};

			any_base() = default;
			template<class _tHandler, typename... _tParams>
			inline
				any_base(any_ops::with_handler_t<_tHandler>, _tParams&&... args)
				: manager(any_ops::construct<_tHandler>(storage, wforward(args)...))
			{}


		protected:
			any_base(const any_base& a)
				: manager(a.manager)
			{}
			~any_base() = default;
		public:
			WB_API any_ops::any_storage&
				call(any_ops::any_storage&, any_ops::op_code) const;

			WB_API void
				clear() wnothrowv;

			void
				copy(const any_base&);

			void
				destroy() wnothrowv;

			bool
				has_value() const wnothrow
			{
				return manager;
			}

			//! \pre ���ԣ�\c manager ��
			//@{
			WB_API void*
				get() const wnothrowv;

			WB_API any_ops::holder*
				get_holder() const;

			//! \since build 1.4
			any_ops::any_storage&
				get_storage() const
			{
				return storage;
			}

			WB_API void
				swap(any_base&) wnothrow;

			template<typename _type>
			_type*
				target() wnothrowv
			{
				return type() == type_id<_type>() ? static_cast<_type*>(get())
					: nullptr;
			}
			template<typename _type>
			const _type*
				target() const wnothrowv
			{
				return type() == type_id<_type>()
					? static_cast<const _type*>(get()) : nullptr;
			}

			WB_API const type_info&
				type() const wnothrowv
			{
				return *unchecked_access<const type_info*>(any_ops::get_type);
			}

			//! \since build 1.4
			template<typename _type, typename... _tParams>
			inline _type
				unchecked_access(any_ops::op_code op, _tParams&&... args) const
			{
				any_ops::any_storage t;

				const auto gd(t.pun<_type>(wforward(args)...));

				return unchecked_access<_type>(t, op);
			}
			template<typename _type>
			inline _type
				unchecked_access(default_init_t, any_ops::op_code op) const
			{
				any_ops::any_storage t;
				const auto gd(t.pun_default<_type>());

				return unchecked_access<_type>(t, op);
			}
			//! \since build 1.4
			template<typename _type>
			inline _type
				unchecked_access(any_ops::any_storage& t, any_ops::op_code op) const
			{
				return call(t, op).access<_type>();
			}
			//@}
		};


		template<class _tAny>
		struct any_emplace
		{
			template<typename _type, typename... _tParams>
			void
				emplace(_tParams&&... args)
			{
				emplace_with_handler<any_ops::value_handler<decay_t<_type>>>(
					wforward(args)...);
			}
			template<typename _type, typename _tOther, typename... _tParams>
			void
				emplace(std::initializer_list<_tOther> il, _tParams&&... args)
			{
				emplace_with_handler<any_ops::value_handler<decay_t<_type>>>(
					il, wforward(args)...);
			}
			template<typename _tHolder, typename... _tParams>
			void
				emplace(any_ops::use_holder_t, _tParams&&... args)
			{
				emplace_with_handler<any_ops::holder_handler<decay_t<_tHolder>>>(
					wforward(args)...);
			}

			template<typename _tHandler, typename... _tParams>
			void
				emplace_with_handler(_tParams&&... args)
			{
				auto& a(static_cast<_tAny&>(*this));

				a.reset();
				a.manager = any_ops::construct<decay_t<_tHandler>>(a.storage,
					wforward(args)...);
			}
		};

	} // namespace details;
	//@}

	/*
	\brief �������Ͳ����Ķ�̬���Ͷ���
	\note  ֵ����,���ڽӿں�����ͬ std::experimental::any ����
	\warning ��������
	\see WG21 N4582 20.6.3[any.class] ��
	\see http://www.boost.org/doc/libs/1_53_0/doc/html/any/reference.html#any.ValueType
	*/
	class WB_API any : private details::any_base, private details::any_emplace<any>
	{
		friend details::any_emplace<any>;
	public:
		//! \post \c this->empty() ��
		any() wnothrow = default;
		//! \since build 1.4
		template<typename _type, wimpl(typename = exclude_self_t<any, _type>)>
			inline
			any(_type&& x)
			: any(any_ops::with_handler_t<
				_t<any_ops::wrap_handler<decay_t<_type>>>>(), wforward(x))
		{}
		//! \note WBase ��չ��
		//@{
		//! \since build 1.4
		template<typename _type, typename... _tParams>
		inline
			any(in_place_type_t<_type>, _tParams&&... args)
			: any(any_ops::with_handler_t<
				any_ops::value_handler<_type>>(), wforward(args)...)
		{}
		/*!
		\brief ���죺ʹ��ָ�������ߡ�
		\since build 1.4
		*/
		//@{
		template<typename _tHolder>
		inline
			any(any_ops::use_holder_t, std::unique_ptr<_tHolder> p)
			: any(any_ops::with_handler_t<
				any_ops::holder_handler<_tHolder>>(), std::move(p))
		{}
		template<typename _tHolder>
		inline
			any(any_ops::use_holder_t, _tHolder&& h)
			: any(any_ops::with_handler_t<
				any_ops::holder_handler<decay_t<_tHolder>>>(), wforward(h))
		{}
		template<typename _tHolder, typename... _tParams>
		inline
			any(any_ops::use_holder_t, in_place_type_t<_tHolder>,
				_tParams&&... args)
			: any(any_ops::with_handler_t<any_ops::holder_handler<_tHolder>>(),
				wforward(args)...)
		{}
		//@}
		template<typename _type>
		inline
			any(_type&& x, any_ops::use_holder_t)
			: any(any_ops::with_handler_t<any_ops::holder_handler<
				any_ops::value_holder<decay_t<_type>>>>(), wforward(x))
		{}
		//! \since build 1.4
		template<class _tHandler, typename... _tParams>
		inline
			any(any_ops::with_handler_t<_tHandler> t, _tParams&&... args)
			: any_base(t, wforward(args)...)
		{}
		//@}
		any(const any&);
		any(any&& a) wnothrow
			: any()
		{
			a.swap(*this);
		}
		//! \since build 1.3
		~any();

		//! \since build 1.4
		template<typename _type, wimpl(typename = exclude_self_t<any, _type>)>
		any&
			operator=(_type&& x)
		{
			any(wforward(x)).swap(*this);
			return *this;
		}
		/*!
		\brief ���Ƹ�ֵ��ʹ�ø��ƺͽ�����
		\since build 1.3
		*/
		any&
			operator=(const any& a)
		{
			any(a).swap(*this);
			return *this;
		}
		/*!
		\brief ת�Ƹ�ֵ��ʹ�ø��ƺͽ�����
		\since build 1.3
		*/
		any&
			operator=(any&& a) wnothrow
		{
			any(std::move(a)).swap(*this);
			return *this;
		}

		//! \since build 1.4
		using any_base::has_value;

		//! \note WBase ��չ��
		//@{
		//! \since build 1.4
		void*
			get() const wnothrow
		{
			return manager ? unchecked_get() : nullptr;
		}

		any_ops::holder*
			get_holder() const
		{
			return manager ? unchecked_get_holder() : nullptr;
		}
		//@}

	protected:
		/*!
		\note WBase ��չ��
		\since build 687
		*/
		//@{
		using any_base::get_storage;

		using any_base::call;
		//@}

	public:
		void
			reset() wnothrow;

		/*!
		\note WBase ��չ��
		\since build 1.4
		*/
		//@{
		using any_emplace<any>::emplace;

		using any_emplace<any>::emplace_with_handler;
		//@}

		void
			swap(any& a) wnothrow
		{
			any_base::swap(a);
		}

		/*!
		\brief ȡĿ��ָ�롣
		\return ���洢Ŀ�����ͺ�ģ�������ͬ��Ϊָ��洢�����ָ��ֵ������Ϊ��ָ��ֵ��
		\note WBase ��չ��
		\since build 1.4
		*/
		//@{
		template<typename _type>
		_type*
			target() wnothrow
		{
			return manager ? any_base::template target<_type>() : nullptr;
		}
		template<typename _type>
		const _type*
			target() const wnothrow
		{
			return manager ? any_base::template target<_type>() : nullptr;
		}
		//@}

		//! \since build 1.4
		const type_info&
			type() const wnothrow
		{
			return manager ? unchecked_type() : type_id<void>();
		}

		/*!
		\note WBase ��չ��
		\pre ���ԣ�\c !empty() ��
		\since build 1.4
		*/
		//@{
	protected:
		//! \since build 1.4
		using any_base::unchecked_access;

	public:
		//! \brief ȡ���������ָ�롣
		void*
			unchecked_get() const wnothrowv
		{
			return any_base::get();
		}

		//! \brief ȡ������ָ�롣
		any_ops::holder*
			unchecked_get_holder() const
		{
			return any_base::get_holder();
		}

		/*!
		\brief ȡ������������͡�
		\since build 1.4
		*/
		const type_info&
			unchecked_type() const wnothrowv
		{
			return any_base::type();
		}
		//@}
	};

	inline void
		swap(any& x, any& y) wnothrow
	{
		x.swap(y);
	}


	/*!
	\ingroup helper_functions
	\brief ���� any ����
	\see WG21 P0032R3 ��
	*/
	//@{
	template<typename _type, typename... _tParams>
	any
		make_any(_tParams&&... args)
	{
		return any(in_place<_type>, wforward(args)...);
	}
	template<typename _type, typename _tOther, typename... _tParams>
	any
		make_any(std::initializer_list<_tOther> il, _tParams&&... args)
	{
		return any(in_place<_type>, il, wforward(args)...);
	}
	//@}

	/*!
	\brief ��̬����ת����
	\return �� <tt>p
	&& p->type() == type_id<remove_pointer_t<_tPointer>>()</tt> ʱ
	Ϊָ������ָ�룬����Ϊ��ָ�롣
	\note ����ͬ \c boost::any_cast ��
	\relates any
	\since build 1.4
	\todo �����ض���������ʹ�ö�̬��ʱ���Ƚ� std::type_info::name() �ı�Ҫ�ԡ�
	*/
	//@{
	//@{
	template<typename _type>
	inline _type*
		any_cast(any* p) wnothrow
	{
		return p ? p->target<_type>() : nullptr;
	}
	template<typename _type>
	inline const _type*
		any_cast(const any* p) wnothrow
	{
		return p ? p->target<_type>() : nullptr;
	}
	//@}
	/*!
	\throw bad_any_cast �� <tt>x.type()
	!= type_id<remove_reference_t<_tValue>>()</tt> ��
	*/
	//@{
	template<typename _tValue>
	_tValue
		any_cast(any& x)
	{
		static_assert(is_any_cast_dest<_tValue>(),
			"Invalid cast destination type found.");

		if (const auto p = x.template target<remove_reference_t<_tValue>>())
			return static_cast<_tValue>(*p);
		throw bad_any_cast(x.type(), white::type_id<_tValue>());
	}
	template<typename _tValue>
	_tValue
		any_cast(const any& x)
	{
		static_assert(is_any_cast_dest<_tValue>(),
			"Invalid cast destination type found.");

		if (const auto p = x.template target<const remove_reference_t<_tValue>>())
			return static_cast<_tValue>(*p);
		throw bad_any_cast(x.type(), white::type_id<_tValue>());
	}
	//! \since build 1.4
	template<typename _tValue>
	_tValue
		any_cast(any&& x)
	{
		static_assert(is_any_cast_dest<_tValue>(),
			"Invalid cast destination type found.");

		if (const auto p = x.template target<remove_reference_t<_tValue>>())
			return static_cast<_tValue>(*p);
		throw bad_any_cast(x.type(), white::type_id<_tValue>());
	}
	//@}
	//@}

	//! \note WBase ��չ��
	//@{
	/*!
	\brief �ж��Ƿ������ͬ����
	*/
	inline bool
		hold_same(const any& x, const any& y)
	{
		return x.get() == y.get();
	}

	/*!
	\brief δ���Ķ�̬����ת����
	\note �Էǿն�������ͬ�ǹ����ӿ� \c boost::unsafe_any_cast ��
	\since build 1.4
	*/
	//@{
	/*!
	\pre ���ԣ� <tt>p && !p->empty()
	&& p->unchecked_type() == type_id<_type>()</tt> ��
	*/
	template<typename _type>
	inline _type*
		unchecked_any_cast(any* p) wnothrowv
	{
		wconstraint(p && p->has_value()
			&& p->unchecked_type() == white::type_id<_type>());
		return static_cast<_type*>(p->unchecked_get());
	}

	/*!
	\pre ���ԣ� <tt>p && !p->empty()
	&& p->unchecked_type() == type_id<const _type>()</tt> ��
	*/
	template<typename _type>
	inline const _type*
		unchecked_any_cast(const any* p) wnothrowv
	{
		wconstraint(p && p->has_value()
			&& p->unchecked_type() == white::type_id<const _type>());
		return static_cast<const _type*>(p->unchecked_get());
	}
	//@}

	/*!
	\brief �ǰ�ȫ��̬����ת����
	\note ����ͬ�ǹ����ӿ� \c boost::unsafe_any_cast ��
	\since build 1.4
	*/
	//@{
	//! \pre ���ԣ� <tt>p && p->type() == type_id<_type>()</tt> ��
	template<typename _type>
	inline _type*
		unsafe_any_cast(any* p) wnothrowv
	{
		wconstraint(p && p->type() == white::type_id<_type>());
		return static_cast<_type*>(p->get());
	}

	//! \pre ���ԣ� <tt>p && p->type() == type_id<const _type>()</tt> ��
	template<typename _type>
	inline const _type*
		unsafe_any_cast(const any* p) wnothrowv
	{
		wconstraint(p && p->type() == white::type_id<const _type>());
		return static_cast<const _type*>(p->get());
	}
	//@}
	//@}


	/*!
	\note WBase ��չ��
	\sa void_ref
	*/
	class WB_API void_ref_any
	{
	private:
		mutable any data;

	public:
		void_ref_any() = default;
		template<typename _type,
			wimpl(typename = exclude_self_t<void_ref_any, _type>)>
			void_ref_any(_type&& x)
			: void_ref_any(wforward(x), is_convertible<_type&&, void_ref>())
		{}

	private:
		template<typename _type>
		void_ref_any(_type&& x, true_)
			: data(void_ref(wforward(x)))
		{}
		template<typename _type>
		void_ref_any(_type&& x, false_)
			: data(wforward(x))
		{}

	public:
		void_ref_any(const void_ref_any&) = default;
		void_ref_any(void_ref_any&&) = default;

		void_ref_any&
			operator=(const void_ref_any&) = default;
		void_ref_any&
			operator=(void_ref_any&&) = default;

		//! \pre ��Ӷ��ԣ�����ָ����ʼ����Ӧ�����͡�
		template<typename _type,
			wimpl(typename = exclude_self_t<void_ref_any, _type>)>
			WB_PURE
			operator _type() const
		{
			if (data.type() == white::type_id<void_ref>())
				return *white::unchecked_any_cast<void_ref>(&data);
			return std::move(*white::unchecked_any_cast<_type>(&data));
		}
	};
}
#endif