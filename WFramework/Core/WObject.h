/*! \file WObject.h
\ingroup WFrameWork/Core
\brief ��������

*/
#ifndef WFrameWork_Core_WObject_H
#define WFrameWork_Core_WObject_H 1

#include <WBase/any.h>
#include <WBase/examiner.hpp>
#include <WBase/operators.hpp>
#include <WBase/observer_ptr.hpp>
#include <WFramework/Core/WCoreUtilities.h>

#ifdef WB_IMPL_MSCPP
#include <fstream>
#include <sstream>
#endif

namespace white
{
	/*!
	\brief ָ���Բ���ָ�����͵ĳ�Ա��������Ȩ�ı�ǩ��
	*/
	template<typename = void>
	struct OwnershipTag
	{};

	struct MoveTag
	{};

	struct PointerTag
	{};

	template<class _tOwner, typename _type>
	struct HasOwnershipOf : std::is_base_of<OwnershipTag<_type>, _tOwner>
	{};

	/*!
	\brief ��һ������ָ����ѡ����������͵ĳ����߻��׳��쳣��
	*/
	//@{
	template<class _tHolder, typename... _tParams>
	white::any
		CreateHolderInPlace(white::true_, _tParams&&... args)
	{
		return white::any(white::any_ops::use_holder,
			white::in_place<_tHolder>, wforward(args)...);
	}
	//! \exception white::invalid_construction ���������޷����ڳ�ʼ�������ߡ�
	template<class _tHolder, typename... _tParams>
	WB_NORETURN white::any
		CreateHolderInPlace(white::false_, _tParams&&...)
	{
		white::throw_invalid_construction();
	}
	//@}

	DeclDerivedI(WF_API, IValueHolder, white::any_ops::holder)

		/*!
		\brief ����ѡ�
		\sa Create
		*/
		enum Creation
		{
		/*!
		\brief �������õļ�ӳ����ߡ�

		����ʵ��Ӧ��֤���ж�Ӧ�� lref<T> ���͵�ֵ���õ�ǰ���е� T ���͵�ֵ��
		*/
		Indirect,
		/*!
		\brief �������õ�ֵ�ĸ�����

		����ʵ��Ӧ��֤���ж�Ӧ��ֵ�ӵ�ǰ���е�ֵ���ƣ�
		���ҽ�����ǰ�����߲������ã����򣬴����õ�ֵ���ơ�
		*/
		Copy,
		/*!
		\brief �������õ�ֵת�Ƶĸ�����

		����ʵ��Ӧ��֤���ж�Ӧ��ֵ�ӵ�ǰ���е�ֵת�ƣ�
		���ҽ�����ǰ�����߲������ã����򣬴����õ�ֵת�ơ�
		*/
		Move
		};

		/*!
		\brief �ж���ȡ�
		\pre ����Ϊ��ָ��ֵ����еĶ���ȥ�� const ����кͲ�����ͬ��̬���͵Ķ���
		\return ���еĶ�����ȣ�����пն����Ҳ���Ϊ��ָ��ֵ��

		�Ƚϲ����ͳ��еĶ���
		����ʵ��Ӧ��֤���ص�ֵ���� EqualityComparable �ж� == ������Ҫ��
		*/
		DeclIEntry(bool Equals(const void*) const)
		/*!
		\brief ȡ�����ж���Ĺ���������������
		*/
		DeclIEntry(size_t OwnsCount() const wnothrow)
		/*!
		\sa Creation
		*/
		//@{
		/*!
		\brief �����µĳ����ߡ�
		\return �����´����ĳ����ߵĶ�̬���Ͷ���
		\exception white::invalid_construction ���������޷����ڳ�ʼ�������ߡ�
		\sa Creation
		\sa CreateHolder

		������ָ����ѡ�����ָ��ѡ����еĶ���
		����ʵ��Ӧ��֤���ص�ֵ����ѡ��ָ�����������ұ任���ı䵱ǰ�߼�״̬��
		�� mutable �����ݳ�Ա�ɱ�ת�ƣ�����Ӧ�׳��쳣��
		*/
		DeclIEntry(white::any Create(Creation) const)

		/*!
		\brief �ṩ���������ߵ�Ĭ��ʵ�֡�
		\sa Create
		*/
		template<typename _type>
		static white::any
		CreateHolder(Creation, _type&);
		//@}
	EndDecl


	template<typename _type1, typename _type2>
	struct HeldEqual : private white::examiners::equal_examiner

	{
		using examiners::equal_examiner::are_equal;
	};

#if  defined(WB_IMPL_MSCPP) && 0
	template<>
	struct HeldEqual<std::ifstream, std::ifstream>
	{
		static bool are_equal(const std::ifstream&, const std::ifstream&)
		{
			return true;
		}
	};

	template<>
	struct HeldEqual<std::istringstream, std::istringstream>
	{
		static bool are_equal(const std::istringstream&, const std::istringstream&)
		{
			return true;
		}
	};
#endif

	template<typename _type1, typename _type2>
	struct HeldEqual<weak_ptr<_type1>, weak_ptr<_type2>>
	{
		static bool
			are_equal(const weak_ptr<_type1>& x, const weak_ptr<_type2>& y)
		{
			return x == y;
		}
	};


	template<typename _type1, typename _type2, typename _type3, typename _type4>
	struct HeldEqual<pair<_type1, _type2>, pair<_type3, _type4>>

	{
		static wconstfn bool
			are_equal(const pair<_type1, _type2>& x, const pair<_type3, _type4>& y)
		{
			return x.first == y.first && x.second == y.second;
		}
	};

	template<typename _type1, typename _type2>
	wconstfn bool
		AreEqualHeld(const _type1& x, const _type2& y)
	{
		return HeldEqual<_type1, _type2>::are_equal(x, y);
	}

	template<typename _type>
	class ValueHolder
		: protected boxed_value<_type>, implements IValueHolder
	{
		static_assert(std::is_object<_type>(), "Non-object type found.");
		static_assert(!is_cv<_type>(), "Cv-qualified type found.");

	public:
		using value_type = _type;

		//@{
		DefDeCtor(ValueHolder)
			template<typename _tParam,
			wimpl(typename = exclude_self_t<ValueHolder, _tParam>)>
			ValueHolder(_tParam&& arg)
			wnoexcept(std::is_nothrow_constructible<_type, _tParam&&>::value)
			: boxed_value<_type>(wforward(arg))
		{}
		using boxed_value<_type>::boxed_value;
		//@}
		DefDeCopyMoveCtorAssignment(ValueHolder)

		PDefH(white::any, Create, Creation c) const ImplI(IValueHolder)
		ImplRet(CreateHolder(c, this->value))

		PDefH(bool, Equals, const void* p) const ImplI(IValueHolder)
		ImplRet(bool(p) && AreEqualHeld(this->value,
			Deref(static_cast<const value_type*>(p))))

		PDefH(size_t, OwnsCount, ) const wnothrow ImplI(IValueHolder)
		ImplRet(1)

		PDefH(void*, get, ) const ImplI(IValueHolder)
		ImplRet(white::addressof(this->value))

		PDefH(const type_info&, type, ) const wnothrow ImplI(IValueHolder)
		ImplRet(type_id<_type>())
	};

	/*!
	\ingroup type_traits_operations
	\brief ָ�������������
	*/
	template<typename _tPointer>
	struct PointerHolderTraits : std::pointer_traits<_tPointer>
	{
		using holder_pointer = _tPointer;
		using shared = white::is_sharing<holder_pointer>;

		//@{
		static PDefH(size_t, count_owner, const holder_pointer& p_held) wnothrow
			ImplRet(count_owner(shared(), p_held))

	private:
		static PDefH(size_t, count_owner, white::false_,
			const holder_pointer& p_held) wnothrow
			ImplRet(is_owner(p_held) ? 1 : 0)
			static PDefH(size_t, count_owner, white::true_,
				const holder_pointer& p_held) wnothrow
			ImplRet(size_t(p_held.use_count()))
			//@}

	public:
		//! \note ʹ�� ADL get_raw ��
		static PDefH(auto, get, const holder_pointer& p_held)
			wnoexcept_spec(get_raw(p_held)) -> decltype(get_raw(p_held))
			ImplRet(get_raw(p_held))

			//! \note ʹ�� ADL owns_unique ��
			static PDefH(bool, is_unique_owner, const holder_pointer& p_held) wnothrow
			ImplRet(owns_unique(p_held))

			//! \note �Է��ڽ�ָ��ʹ�� ADL owns_nonnull ��
			static PDefH(bool, is_owner, const holder_pointer& p_held) wnothrow
			ImplRet(is_owner(std::is_pointer<holder_pointer>(), p_held))

	private:
		static PDefH(bool, is_owner, white::false_, const holder_pointer& p_held)
			wnothrow
			ImplRet(owns_nonnull(p_held))
			static PDefH(bool, is_owner, white::true_, const holder_pointer& p_held)
			wnothrow
			ImplRet(bool(p_held))
	};

	template<typename _type, class _tTraits = PointerHolderTraits<unique_ptr<_type>>>
	class PointerHolder : implements IValueHolder
	{
		static_assert(white::is_decayed<_type>(), "Invalid type found.");

	public:
		using value_type = _type;
		using traits = _tTraits;
		using holder_pointer = typename traits::holder_pointer;
		using pointer
			= decltype(traits::get(std::declval<const holder_pointer&>()));
		using shared = typename traits::shared;

	protected:
		holder_pointer p_held;

	public:
		//! \brief ȡ������Ȩ��
		explicit
		PointerHolder(pointer value)
			: p_held(value)
		{}
		//@{
		PointerHolder(const holder_pointer& p)
			: p_held(p)
		{}
		PointerHolder(holder_pointer&& p)
			: p_held(std::move(p))
		{}

		PointerHolder(const PointerHolder& h)
			: PointerHolder(h,shared())
		{}
	private:
		PointerHolder(const PointerHolder& h, white::false_)
			: PointerHolder(white::clone_monomorphic_ptr(h.p_held))
		{}
		PointerHolder(const PointerHolder& h, white::true_)
			: p_held(h.p_held)
		{}
	public:
		DefDeMoveCtor(PointerHolder)
			//@}

			DefDeCopyAssignment(PointerHolder)
			DefDeMoveAssignment(PointerHolder)
			
			DefGetter(wnothrow, const holder_pointer&, Held, p_held)

			white::any
			Create(Creation c) const ImplI(IValueHolder)
		{
			if (shared() && c == IValueHolder::Copy)
				return CreateHolderInPlace<PointerHolder>(shared(), p_held);
			if (const auto& p = traits::get(p_held))
				return CreateHolder(c, *p);
			white::throw_invalid_construction();
		}

		PDefH(bool, Equals, const void* p) const ImplI(IValueHolder)
			ImplRet(traits::is_owner(p_held) && p
				? AreEqualHeld(Deref(traits::get(p_held)),
					Deref(static_cast<const value_type*>(p))) : !get())

			PDefH(size_t, OwnsCount, ) const wnothrow ImplI(IValueHolder)
			ImplRet(traits::count_owner(p_held))


			PDefH(void*, get, ) const ImplI(IValueHolder)
			ImplRet(traits::get(p_held))

			PDefH(const type_info&, type, ) const wnothrow ImplI(IValueHolder)
			ImplRet(traits::is_owner(p_held)
				? white::type_id<_type>() : white::type_id<void>())
	};

	/*!
	\ingroup metafunctions
	\relates PointerHolder
	*/
	template<typename _tPointer>
	using HolderFromPointer = PointerHolder<typename PointerHolderTraits<
		_tPointer>::element_type, PointerHolderTraits<_tPointer>>;

	/*!
	\brief �����ڽӿڵ����ö�̬���ͳ����ߡ�
	\tparam _type ���еı����õ�ֵ���͡�
	\note ���Գ���ֵ��������Ȩ��
	\sa ValueHolder
	*/
	template<typename _type>
	class RefHolder : implements IValueHolder
	{
		static_assert(std::is_object<_type>(), "Invalid type found.");

	public:
		using value_type
			= white::remove_reference_t<white::unwrap_reference_t<_type>>;

	private:
		ValueHolder<lref<value_type>> base;

	public:
		//! \brief ��ȡ������Ȩ��
		RefHolder(_type& r)
			: base(r)
		{}
		DefDeCopyMoveCtorAssignment(RefHolder)

		PDefH(white::any, Create, Creation c) const ImplI(IValueHolder)
		ImplRet(CreateHolder(c, Ref()))

		PDefH(bool, Equals, const void* p) const ImplI(IValueHolder)
		ImplRet(bool(p) && AreEqualHeld(Deref(static_cast<const value_type*>(
			get())), Deref(static_cast<const value_type*>(p))))

		PDefH(size_t, OwnsCount, ) const wnothrow ImplI(IValueHolder)
		ImplRet(0)

	private:
		PDefH(value_type&, Ref, ) const
			ImplRet(Deref(static_cast<lref<value_type>*>(base.get())).get())
	public:
			PDefH(void*, get, ) const ImplI(IValueHolder)
			ImplRet(white::pvoid(std::addressof(Ref())))

			PDefH(const type_info&, type, ) const wnothrow ImplI(IValueHolder)
			ImplRet(white::type_id<value_type>())
	};


	template<typename _type>
	white::any
		IValueHolder::CreateHolder(Creation c, _type& obj)
	{
		switch (c)
		{
		case Indirect:
			return CreateHolderInPlace<RefHolder<_type>>(white::true_(),
				white::ref(obj));
		case Copy:
			return CreateHolderInPlace<ValueHolder<_type>>(
				std::is_copy_constructible<_type>(), obj);
		case Move:
			return CreateHolderInPlace<ValueHolder<_type>>(
				std::is_move_constructible<_type>(), std::move(obj));
		default:
			white::throw_invalid_construction();
		}
	}

	class WF_API ValueObject : private equality_comparable<ValueObject>
	{
	public:
		/*!
		\brief ��������ݡ�
		*/
		using Content = white::any;

	private:

		Content content;

	public:
		/*!
		\brief �޲������졣
		\note �õ���ʵ����
		*/
		DefDeCtor(ValueObject)
			/*!
			\brief ���죺ʹ�ö������á�
			\pre obj ����Ϊת�ƹ��������
			*/
			template<typename _type,
			wimpl(typename = white::exclude_self_t<ValueObject, _type>)>
			ValueObject(_type&& obj)
			: content(white::any_ops::use_holder,
				white::in_place<ValueHolder<white::decay_t<_type>>>, wforward(obj))
		{}
		/*!
		\brief ���죺ʹ�ö����ʼ��������
		\tparam _type Ŀ�����͡�
		\tparam _tParams Ŀ�����ͳ�ʼ���������͡�
		\pre _type �ɱ� _tParams ������ʼ����
		*/
		template<typename _type, typename... _tParams>
		ValueObject(white::in_place_type_t<_type>, _tParams&&... args)
			: content(white::any_ops::use_holder,
				white::in_place<ValueHolder<_type>>, wforward(args)...)
		{}

		template<typename _tHolder, typename... _tParams>
		ValueObject(white::any_ops::use_holder_t,
			white::in_place_type_t<_tHolder>, _tParams&&... args)
			: content(white::any_ops::use_holder,
				white::in_place<_tHolder>, wforward(args)...)
		{}
	private:
		/*!
		\brief ���죺ʹ�ó����ߡ�
		*/
		ValueObject(const IValueHolder& holder, IValueHolder::Creation c)
			: content(holder.Create(c))
		{}
	public:
		template<typename _type>
		ValueObject(_type& obj, OwnershipTag<>)
			: content(white::any_ops::use_holder,
				white::in_place<RefHolder<_type>>, white::ref(obj))
		{}

		/*!
		\brief ���죺ʹ�ö���ָ�롣
		\note �õ�����ָ��ָ���ָ�������ʵ�������������Ȩ��
		\note ʹ�� PointerHolder ������Դ��Ĭ��ʹ�� delete �ͷ���Դ����
		*/
		template<typename _type>
		ValueObject(_type* p, PointerTag)
			: content(any_ops::use_holder,
				in_place<PointerHolder<_type>>, p)
		{}
		/*!
		\brief ���죺ʹ�ö��� unique_ptr ָ�롣
		\note �õ�����ָ��ָ���ָ�������ʵ�������������Ȩ��
		\note ʹ�� PointerHolder ������Դ��Ĭ��ʹ�� delete �ͷ���Դ����
		*/
		template<typename _type>
		ValueObject(unique_ptr<_type>&& p, PointerTag)
			: ValueObject(p.get(), PointerTag())
		{
			p.release();
		}
		/*!
		\brief ���ƹ��죺Ĭ��ʵ�֡�
		*/
		DefDeCopyCtor(ValueObject)
			/*!
			\brief ת�ƹ��죺Ĭ��ʵ�֡�
			*/
			DefDeMoveCtor(ValueObject)
			/*!
			\brief ������Ĭ��ʵ�֡�
			*/
			DefDeDtor(ValueObject)

			//@{
			DefDeCopyAssignment(ValueObject)
			DefDeMoveAssignment(ValueObject)
			//@}

			/*!
			\brief �ж��Ƿ�Ϊ�ջ�ǿա�
			*/
			DefBoolNeg(explicit, content.has_value())

			//@{
			//! \brief �Ƚ���ȣ�������Ϊ�ջ򶼷ǿ��Ҵ洢�Ķ�����ȡ�
			friend PDefHOp(bool, == , const ValueObject& x, const ValueObject& y)
			ImplRet(x.Equals(y))

		//@{
		//! \brief �Ƚ���ȣ��洢�Ķ���ֵ��ȡ�
		//@}
		//@}
		

		/*!
		\brief ȡ��������ݡ�
		*/
		DefGetter(const wnothrow, const Content&, Content, content)

		IValueHolder*
			GetHolderPtr() const;
		/*!
		\build ȡ���������á�
		\pre ������ָ��ǿա�
		*/
		IValueHolder&
			GetHolderRef() const;

		/*!
		\brief ȡָ�����͵Ķ���
		\pre ��Ӷ��ԣ��洢�������ͺͷ��ʵ�����һ�¡�
		*/
		//@{
		template<typename _type>
		inline _type&
			GetObject() wnothrowv
		{
			return Deref(unchecked_any_cast<_type>(&content));
		}
		template<typename _type>
		inline const _type&
			GetObject() const wnothrowv
		{
			return Deref(unchecked_any_cast<const _type>(&content));
		}
		//@}
		//@}

		/*!
		\brief ����ָ�����Ͷ���
		\exception std::bad_cast ��ʵ�������ͼ��ʧ�� ��
		*/
		//@{
		template<typename _type>
		inline _type&
			Access()
		{
			return any_cast<_type&>(content);
		}
		template<typename _type>
		inline const _type&
			Access() const
		{
			return any_cast<const _type&>(content);
		}
		//@}

		/*!
		\brief ����ָ�����Ͷ���ָ�롣
		*/
		//@{
		template<typename _type>
		inline observer_ptr<_type>
			AccessPtr() wnothrow
		{
			return make_observer(any_cast<_type>(&content));
		}
		template<typename _type>
		inline observer_ptr<const _type>
			AccessPtr() const wnothrow
		{
			return make_observer(any_cast<const _type>(&content));
		}
		//@}

		/*!
		\brief �����
		\post <tt>*this == ValueObject()</tt> ��
		*/
		PDefH(void, Clear, ) wnothrow
			ImplExpr(content.reset())

		/*!
		\brief ȡ����ĸ��Ƴ�ʼ��ת�ƽ�������Ƿ����Ψһ����Ȩѡ��ת�ƻ���ֵ����
		*/
		PDefH(ValueObject, CopyMove, )
			ImplRet(white::copy_or_move(!OwnsUnique(), *this))

			/*!
			\brief ȡ��ָ��������ѡ����ĸ�����
			*/
			ValueObject
			Create(IValueHolder::Creation) const;

		/*!
		\brief �ж���ȡ�
		\sa IValueHolder::Equals

		�Ƚϲ����ͳ��еĶ���
		*/
		//@{
		bool
			Equals(const ValueObject&) const;
		template<typename _type>
		bool
			Equals(const _type& x) const
		{
			if (const auto p_holder = content.get_holder())
				return p_holder->type() == white::type_id<_type>()
				&& EqualsRaw(std::addressof(x));
			return {};
		}

		//! \pre ����Ϊ��ָ��ֵ����еĶ���ȥ�� const ����кͲ�����ͬ��̬���͵Ķ���
		bool
			EqualsRaw(const void*) const;

		/*!
		\pre ��Ӷ��ԣ����еĶ���ǿա�
		\pre ���еĶ���ȥ�� const ����кͲ�����ͬ��̬���͵Ķ���
		*/
		bool
			EqualsUnchecked(const void*) const;
		//@}

		/*!
		\brief ȡ���õ�ֵ����ĸ�����
		*/
		PDefH(ValueObject, MakeCopy, ) const
			ImplRet(Create(IValueHolder::Copy))

			/*!
			\brief ȡ���õ�ֵ�����ת�Ƹ�����
			*/
			PDefH(ValueObject, MakeMove, ) const
			ImplRet(Create(IValueHolder::Move))

			/*!
			\brief ȡ���õ�ֵ����ĳ�ʼ�����������Ƿ��������Ȩѡ��ת�ƻ��ƶ��󸱱���
			*/
			PDefH(ValueObject, MakeMoveCopy, ) const
			ImplRet(Create(OwnsUnique() ? IValueHolder::Move : IValueHolder::Copy))

			/*!
			\brief ȡ������õ�ֵ����
			*/
			PDefH(ValueObject, MakeIndirect, ) const
			ImplRet(Create(IValueHolder::Indirect))

			/*!
			\brief ȡ�����߳��еĶ���Ĺ��������ߵ�������
			*/
			size_t
			OwnsCount() const wnothrow;

		/*!
		\brief �ж��Ƿ��ǳ��еĶ����Ψһ�����ߡ�
		*/
		PDefH(bool, OwnsUnique, ) const wnothrow
			ImplRet(OwnsCount() == 1)

		//@{
		template<typename _type, typename... _tParams>
		void
			emplace(_tParams&&... args)
		{
			using Holder = ValueHolder<white::decay_t<_type>>;

			content.emplace<Holder>(white::any_ops::use_holder,
				Holder(wforward(args)...));
		}
		template<typename _type>
		void
			emplace(_type* p, PointerTag)
		{
			using Holder = PointerHolder<white::decay_t<_type>>;

			content.emplace<Holder>(white::any_ops::use_holder, Holder(p));
		}
		//@}

		/*!
		\brief ������
		*/
		friend PDefH(void, swap, ValueObject& x, ValueObject& y) wnothrow
			ImplExpr(x.content.swap(y.content))

		/*!
		\brief ȡ���еĶ�������͡�
		\sa white::any::type
		*/
		PDefH(const type_info&, type, ) const wnothrow
			ImplRet(content.type())
	};

	template<typename _type>
	inline observer_ptr<_type>
		AccessPtr(ValueObject& vo) wnothrow
	{
		return vo.AccessPtr<_type>();
	}
	template<typename _type>
	inline observer_ptr<const _type>
		AccessPtr(const ValueObject& vo) wnothrow
	{
		return vo.AccessPtr<_type>();
	}

	/*!
	\brief ��ָ���������蹹���滻ֵ��

	Ĭ�϶� ValueObject ������ֵ�ᱻֱ�Ӹ��ƻ�ת�Ƹ�ֵ��
	�������ε��� ValueObject::emplace ��
	ʹ�õ����͵��ĸ������ֱ�ָ����Ĭ�������²�����ֵ��ʹ�ø�ֵ��
	*/
	//@{
	template<typename _type, typename... _tParams>
	void
		EmplaceCallResult(ValueObject&, _type&&, white::false_) wnothrow
	{}
	template<typename _type, typename... _tParams>
	inline void
		EmplaceCallResult(ValueObject& vo, _type&& res, white::true_, white::true_)
		wnoexcept_spec(vo = wforward(res))
	{
		vo = wforward(res);
	}
	template<typename _type, typename... _tParams>
	inline void
		EmplaceCallResult(ValueObject& vo, _type&& res, white::true_, white::false_)
	{
		vo.emplace<white::decay_t<_type>>(wforward(res));
	}
	template<typename _type, typename... _tParams>
	inline void
		EmplaceCallResult(ValueObject& vo, _type&& res, white::true_)
	{
		white::EmplaceCallResult(vo, wforward(res), white::true_(),
			std::is_same<white::decay_t<_type>, ValueObject>());
	}
	template<typename _type, typename... _tParams>
	inline void
		EmplaceCallResult(ValueObject& vo, _type&& res)
	{
		white::EmplaceCallResult(vo, wforward(res), white::not_<
			std::is_same<white::decay_t<_type>, white::pseudo_output>>());
	}
	//@}

	template<typename _type, typename... _tParams>
	_type&
		EmplaceIfEmpty(ValueObject& vo, _tParams&&... args)
	{
		if (!vo)
		{
			vo.emplace<_type>(wforward(args)...);
			return vo.GetObject<_type>();
		}
		return vo.Access<_type>();
	}

	//! \brief �ж��Ƿ������ͬ����
	inline PDefH(bool, HoldSame, const ValueObject& x, const ValueObject& y)
		ImplRet(white::hold_same(x.GetContent(), y.GetContent()))
	//@}

	/*!
	\brief ������ģ�塣
	\tparam _type �������Ķ������ͣ����ܱ��޲������졣
	\tparam _tOwnerPointer ����������ָ�����͡�
	\warning ����������ָ����Ҫʵ������Ȩ���壬
	��������޷��ͷ���Դ�����ڴ�й©��������Ԥ����Ϊ��
	\todo �߳�ģ�ͼ���ȫ�ԡ�

	���ڱ�������Ĭ�϶��󣬿�ͨ��дʱ���Ʋ��Դ����¶��󣻿���Ϊ�ա�
	*/
	template<typename _type, class _tOwnerPointer = shared_ptr<_type>>
	class GDependency
	{
	public:
		using DependentType = _type;
		using PointerType = _tOwnerPointer;
		using ConstReferenceType = identity_t<decltype(*(PointerType()))>;
		using ReferentType = remove_const_t<remove_reference_t<
			ConstReferenceType>>;
		using ReferenceType = ReferentType&;

	private:
		PointerType ptr;

	public:
		inline
			GDependency(PointerType p = PointerType())
			: ptr(p)
		{
			GetCopyOnWritePtr();
		}

		DefDeCopyAssignment(GDependency)
			DefDeMoveAssignment(GDependency)

			DefCvt(const wnothrow, ConstReferenceType, *ptr)
			DefCvt(wnothrow, ReferenceType, *ptr)
			DefCvt(const wnothrow, bool, bool(ptr))

			DefGetter(const wnothrow, ConstReferenceType, Ref,
				operator ConstReferenceType())
			DefGetter(wnothrow, ReferenceType, Ref, operator ReferenceType())
			DefGetter(wnothrow, ReferenceType, NewRef, *GetCopyOnWritePtr())

			//! \post ����ֵ�ǿա�
			PointerType
			GetCopyOnWritePtr()
		{
			if (!ptr)
				ptr = PointerType(new DependentType());
			else if (!ptr.unique())
				ptr = PointerType(white::clone_monomorphic(Deref(ptr)));
			return Nonnull(ptr);
		}

		inline
			void Reset()
		{
			reset(ptr);
		}
	};

	template<typename _type>
	class GMRange
	{
	public:
		using ValueType = _type;

	protected:
		ValueType max_value; //!< ���ȡֵ��
		ValueType value; //!< ֵ��

		/*!
		\brief ���죺ʹ��ָ�����ȡֵ��ֵ��
		*/
		GMRange(ValueType m, ValueType v)
			: max_value(m), value(v)
		{}

	public:
		DefGetter(const wnothrow, ValueType, MaxValue, max_value)
			DefGetter(const wnothrow, ValueType, Value, value)
	};

}

#endif
