/*! \file WEvent.hpp
\ingroup WFrameWork/Core
\brief �¼��ص���

*/
#ifndef WFrameWork_Core_LEvent_Hpp
#define WFrameWork_Core_LEvent_Hpp 1

#include <WBase/sutility.h>
#include "WFramework/Core/WFunc.hpp"
#include "WFramework/Core/WObject.h"
#include <WBase/iterator.hpp>
#include <WBase/container.hpp>
#include <WBase/operators.hpp>
#include <WBase/functional.hpp>

#include <WBase/optional.hpp>

namespace white
{

	/*!
	\brief �¼��������ӿ�ģ�塣
	*/
	template<typename... _tParams>
	DeclDerivedI(, GIHEvent, cloneable)
		DeclIEntry(size_t operator()(_tParams...) const)
		DeclIEntry(GIHEvent* clone() const ImplI(cloneable))
		EndDecl

		template<typename... _tParams>
	GIHEvent<_tParams...>::DefDeDtor(GIHEvent)


		/*!
		\brief ��׼�¼�������ģ�塣
		\note ��ʹ�÷º��������Բ����� \c EqualityComparable �Ľӿڣ���
		��ʹ�÷��� \c bool �� \c operator== ������ģ�����޷������������ȷ�ԡ�
		*/
		//@{
		template<typename>
	class GHEvent;

	//! \warning ����������
	template<typename _tRet, typename... _tParams>
	class GHEvent<_tRet(_tParams...)>
		: protected std::function<_tRet(_tParams...)>,
		private equality_comparable<GHEvent<_tRet(_tParams...)>>,
		private equality_comparable<GHEvent<_tRet(_tParams...)>, nullptr_t>
	{
	public:
		using TupleType = tuple<_tParams...>;
		using FuncType = _tRet(_tParams...);
		using BaseType = std::function<FuncType>;

	private:
		//! \brief �ȽϺ������͡�
		using Comparer = bool(*)(const GHEvent&, const GHEvent&);
		template<class _tFunctor>
		struct GEquality
		{
			//@{
			using Decayed = white::decay_t<_tFunctor>;

			static bool
				AreEqual(const GHEvent& x, const GHEvent& y)
				//	https://stackoverflow.com/questions/35790350/noexcept-inheriting-constructors-and-the-invalid-use-of-an-incomplete-type-that.
#if !(WB_IMPL_GNUCPP >= 70000)
				wnoexcept_spec(white::examiners::equal_examiner::are_equal(Deref(
					x.template target<Decayed>()), Deref(y.template target<Decayed>())))
#endif
			{
				return white::examiners::equal_examiner::are_equal(
					Deref(x.template target<Decayed>()),
					Deref(y.template target<Decayed>()));
			}
			//@}
		};

		Comparer comp_eq; //!< �ȽϺ�������ȹ�ϵ��

	public:
		/*!
		\brief ���죺ʹ�ú���ָ�롣
		*/
		wconstfn
			GHEvent(FuncType* f = {})
			: BaseType(f), comp_eq(GEquality<FuncType>::AreEqual)
		{}
		/*!
		\brief ʹ�ú�������
		*/
		template<class _fCallable>
		wconstfn
			GHEvent(_fCallable f, white::enable_if_t<
				std::is_constructible<BaseType, _fCallable>::value, int> = 0)
			: BaseType(f), 
			comp_eq(GEquality<white::decay_t<_fCallable>>::AreEqual)
		{}
		/*!
		\brief ʹ����չ��������
		\todo �ƶϱȽ���Ȳ�����
		*/
		template<class _fCallable>
		wconstfn
			GHEvent(_fCallable&& f,white::enable_if_t<
				!std::is_constructible<BaseType, _fCallable>::value, int> = 0)
			: BaseType(white::make_expanded<_tRet(_tParams...)>(wforward(f))),
			comp_eq([](const GHEvent&, const GHEvent&) wnothrow{
			return true;
		})
		{}
		/*!
		\brief ���죺ʹ�ö������úͳ�Ա����ָ�롣
		\warning ʹ�ÿճ�Աָ�빹��ĺ��������������δ������Ϊ��
		*/
		template<class _type>
		wconstfn
			GHEvent(_type& obj, _tRet(_type::*pm)(_tParams...))
			: GHEvent([&, pm](_tParams... args) wnoexcept(
				noexcept((obj.*pm)(wforward(args)...))
				&& std::is_nothrow_copy_constructible<_tRet>::value) {
			return (obj.*pm)(wforward(args)...);
		})
		{}
		DefDeCopyMoveCtorAssignment(GHEvent)

			friend 
#ifndef WB_IMPL_MSCPP
			wconstfn 
#endif
			bool
			operator==(const GHEvent& x, const GHEvent& y)
		{
			return
#if defined(WS_DLL) || defined(WS_BUILD_DLL)
				x.target_type() == y.target_type()
#else
				x.comp_eq == y.comp_eq
#endif
				&& (!bool(x) || x.comp_eq(x, y));
		}

		//! \brief ���á�
		using BaseType::operator();

		using BaseType::operator bool;

		using BaseType::target;

		using BaseType::target_type;
	};
	//@}

	/*!
	\relates GHEvent
	*/
	template<typename _tRet, typename... _tParams>
	wconstfn bool
		operator==(const GHEvent<_tRet(_tParams...)>& x, nullptr_t)
	{
		return !x;
	}


	/*!
	\brief �¼����ȼ���
	*/
	using EventPriority = std::uint8_t;


	/*!
	\brief Ĭ���¼����ȼ���
	*/
	wconstexpr const EventPriority DefaultEventPriority(0x80);


	//@{
	//! \brief �����������������¼���������������
	struct CountedHandlerInvoker
	{
		/*!
		\exception std::bad_function_call �����쳣������
		\return �ɹ����õ��¼�������������
		*/
		template<typename _tIn, typename... _tParams>
		size_t
			operator()(_tIn first, _tIn last, _tParams&&... args) const
		{
			size_t n(0);

			while (first != last)
			{
				TryExpr((*first)(wforward(args)...))
					CatchIgnore(std::bad_function_call&)
					wunseq(++n, ++first);
			}
			return n;
		}
	};


	//! \brief �����ϵ�������
	template<typename _type, typename _tCombiner>
	struct GCombinerInvoker
	{
		static_assert(is_decayed<_tCombiner>(), "Invalid type found.");

	public:
		using result_type = _type;

	private:
		_tCombiner combiner;

	public:
		template<typename _tIn, typename... _tParams>
		result_type
			operator()(_tIn first, _tIn last, _tParams&&... args) const
		{
			// XXX: Blocked. 'wforward' cause G++ 5.2 crash: internal
			//	compiler error: Aborted (program cc1plus).
#if false
			const auto tr([&](_tIn iter) {
				return make_transform(iter, [&](_tIn i) {
					// XXX: Blocked. 'std::forward' still cause G++ 5.2 crash:
					//	internal compiler error: in execute, at cfgexpand.c:6044.
					// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=65992.
					return (*i)(std::forward<_tParams>(args)...);
				});
			});
#else
			const auto f([&](_tIn i) {
				return (*i)(std::forward<_tParams>(args)...);
			});
			const auto tr([f](_tIn iter) {
				return make_transform(iter, f);
			});
#endif
			return combiner(tr(first), tr(last));
		}
	};


	//! \brief Ĭ�Ͻ����ϵ�������
	template<typename _type>
	using GDefaultLastValueInvoker
		= GCombinerInvoker<_type, default_last_value<_type>>;

	//! \brief ��ѡ�����ϵ�������
	template<typename _type>
	using GOptionalLastValueInvoker = GCombinerInvoker<cond_t<std::is_void<
		_type>, void, optional<_type>>,optional_last_value<_type>>;



	/*!
	\brief �¼�ģ�塣
	\note ֧��˳��ಥ��
	*/
	//@{
	template<typename, typename = CountedHandlerInvoker>
	class GEvent;

	/*!
	\note ��ơ�
	\warning ����������
	*/
	template<typename _tRet, typename... _tParams, typename _tInvoker>
	class GEvent<_tRet(_tParams...), _tInvoker>
	{
	public:
		//@{
		using HandlerType = GHEvent<_tRet(_tParams...)>;
		using TupleType = typename HandlerType::TupleType;
		using FuncType = typename HandlerType::FuncType;
		//@}
		/*!
		\brief �������͡�
		*/
		using ContainerType
			= multimap<EventPriority, HandlerType, std::greater<EventPriority>>;
		using InvokerType = _tInvoker;
		//@{
		using const_iterator = typename ContainerType::const_iterator;
		using const_reference = typename ContainerType::const_reference;
		using const_reverse_iterator
			= typename ContainerType::const_reverse_iterator;
		using iterator = typename ContainerType::iterator;
		using reference = typename ContainerType::reference;
		using reverse_iterator = typename ContainerType::reverse_iterator;
		using size_type = typename ContainerType::size_type;
		using value_type = typename ContainerType::value_type;
		//@}
		using result_type = decltype(std::declval<_tInvoker&>()(
			std::declval<const_iterator>(), std::declval<const_iterator>(),
			std::declval<_tParams>()...));

		/*!
		\brief ��������
		*/
		InvokerType Invoker{};

	private:
		/*!
		\brief ��Ӧ�б�
		*/
		ContainerType handlers{};

	public:
		/*!
		\brief �޲������죺Ĭ��ʵ�֡�
		\note �õ���ʵ����
		*/
		wconstfn DefDeCtor(GEvent)
			/*!
			\brief ���죺ʹ��ָ����������
			*/
			GEvent(InvokerType ivk)
			: Invoker(std::move(ivk))
		{}
		/*!
		\brief ���죺ʹ��ָ��������������¼���������
		*/
		template<typename _tHandler,
			wimpl(typename = exclude_self_t<GEvent, _tHandler>)>
			GEvent(_tHandler&& h, InvokerType ivk = {})
			: Invoker(std::move(ivk))
		{
			Add(wforward(h));
		}
		DefDeCopyMoveCtorAssignment(GEvent)

			/*!
			\brief ��ֵ�������¼���Ӧ��ʹ�õ�һ�������ָ����ָ���¼���������
			*/
			template<typename _tHandler,
			wimpl(typename = exclude_self_t<GEvent, _tHandler>)>
			inline GEvent&
			operator=(_tHandler&& _arg)
		{
			return *this = GEvent(wforward(_arg));
		}

		//@{
		/*!
		\brief ����¼���Ӧ��ʹ�� const �¼������������ȼ���
		\note ������Ƿ��Ѿ����б��С�
		*/
		inline PDefHOp(GEvent&, +=, const HandlerType& h)
			ImplRet(Add(h))
			/*!
			\brief ����¼���Ӧ��ʹ���¼���������
			\note ������Ƿ��Ѿ����б��С�
			*/
			inline PDefHOp(GEvent&, +=, HandlerType&& h)
			ImplRet(Add(std::move(h)))
			/*!
			\brief ����¼���Ӧ��Ŀ��Ϊ��һ�������ָ����ָ���¼���������
			\note ������Ƿ��Ѿ����б��С�
			*/
			template<typename _type>
		inline GEvent&
			operator+=(_type&& _arg)
		{
			return Add(HandlerType(wforward(_arg)));
		}

		//! \brief �Ƴ��¼���Ӧ��ָ�� const �¼���������
		GEvent&
			operator-=(const HandlerType& h)
		{
			erase_all_if<ContainerType>(handlers, handlers.cbegin(),
				handlers.cend(), [&](decltype(*handlers.cbegin()) pr) {
				return pr.second == h;
			});
			return *this;
		}
		/*!
		\brief �Ƴ��¼���Ӧ��ָ���� const �¼���������
		\note ��ֹģ�� <tt>operator-=</tt> �ݹ顣
		*/
		inline PDefHOp(GEvent&, -=, HandlerType&& h)
			ImplRet(*this -= static_cast<const HandlerType&>(h))
			/*!
			\brief �Ƴ��¼���Ӧ��Ŀ��Ϊ��һ�������ָ����ָ���¼���������
			*/
			template<typename _type>
		inline GEvent&
			operator-=(_type&& _arg)
		{
			return *this -= HandlerType(wforward(_arg));
		}
		//@}

		/*!
		\brief �����¼���Ӧ��
		\note ������Ƿ��Ѿ����б��С�
		\sa Insert
		*/
		//@{
		//! \note ʹ�� const �¼������������ȼ���
		inline PDefH(GEvent&, Add, const HandlerType& h,
			EventPriority prior = DefaultEventPriority)
			ImplRet(Insert(h, prior), *this)
			//! \note ʹ�÷� const �¼������������ȼ���
			inline PDefH(GEvent&, Add, HandlerType&& h,
				EventPriority prior = DefaultEventPriority)
			ImplRet(Insert(std::move(h), prior), *this)
			//! \note ʹ�õ�һ�������ָ�����¼������������ȼ���
			template<typename _type>
		inline GEvent&
			Add(_type&& _arg, EventPriority prior = DefaultEventPriority)
		{
			return Add(HandlerType(wforward(_arg)), prior);
		}
		/*!
		\note ʹ�ö������á���Ա����ָ������ȼ���
		*/
		template<class _tObj, class _type>
		inline GEvent&
			Add(_tObj& obj, _tRet(_type::*pm)(_tParams...),
				EventPriority prior = DefaultEventPriority)
		{
			return Add(HandlerType(static_cast<_type&>(obj), std::move(pm)), prior);
		}
		//@}

		/*!
		\brief �����¼���Ӧ��
		\note ������Ƿ��Ѿ����б��С�
		*/
		//@{
		//! \note ʹ�� const �¼������������ȼ���
		inline PDefH(typename ContainerType::iterator, Insert, const HandlerType& h,
			EventPriority prior = DefaultEventPriority)
			ImplRet(handlers.emplace(prior, h))
			//! \note ʹ�÷� const �¼������������ȼ���
			inline PDefH(typename ContainerType::iterator, Insert, HandlerType&& h,
				EventPriority prior = DefaultEventPriority)
			ImplRet(handlers.emplace(prior, std::move(h)))
			//! \note ʹ�õ�һ�������ָ�����¼������������ȼ���
			template<typename _type>
		inline typename ContainerType::iterator
			Insert(_type&& _arg, EventPriority prior = DefaultEventPriority)
		{
			return Insert(HandlerType(wforward(_arg)), prior);
		}
		//! \note ʹ�ö������á���Ա����ָ������ȼ���
		template<class _tObj, class _type>
		inline typename ContainerType::iterator
			Insert(_tObj& obj, _tRet(_type::*pm)(_tParams...),
				EventPriority prior = DefaultEventPriority)
		{
			return
				Insert(HandlerType(static_cast<_type&>(obj), std::move(pm)), prior);
		}
		//@}

		/*!
		\brief �Ƴ��¼���Ӧ��Ŀ��Ϊָ���������úͳ�Ա����ָ�롣
		*/
		template<class _tObj, class _type>
		inline GEvent&
			Remove(_tObj& obj, _tRet(_type::*pm)(_tParams...))
		{
			return *this -= HandlerType(static_cast<_type&>(obj), std::move(pm));
		}

		/*!
		\brief �ж��Ƿ����ָ���¼���Ӧ��
		*/
		bool
			Contains(const HandlerType& h) const
		{
			using white::get_value;

			return std::count(handlers.cbegin() | get_value,
				handlers.cend() | get_value, h) != 0;
		}
		/*!
		\brief �ж��Ƿ������һ�������ָ�����¼���Ӧ��
		*/
		template<typename _type>
		inline bool
			Contains(_type&& _arg) const
		{
			return Contains(HandlerType(wforward(_arg)));
		}

		/*!
		\brief ���ã����ݲ�������������
		*/
		PDefHOp(result_type, (), _tParams... args) const
			ImplRet(Invoker(handlers.cbegin() | get_value,
				handlers.cend() | get_value, wforward(args)...))

			PDefH(const_iterator, cbegin, ) const wnothrow
			ImplRet(handlers.cbegin())

			//@{
			PDefH(iterator, begin, ) wnothrow
			ImplRet(handlers.begin())
			PDefH(iterator, begin, ) const wnothrow
			ImplRet(handlers.begin())

			PDefH(const_iterator, cend, ) const wnothrow
			ImplRet(handlers.cend())

			//! \brief ������Ƴ������¼���Ӧ��
			PDefH(void, clear, ) wnothrow
			ImplRet(handlers.clear())

			//@{
			PDefH(size_type, count, EventPriority prior) const wnothrow
			ImplRet(handlers.count(prior))

			PDefH(const_reverse_iterator, crbegin, ) const wnothrow
			ImplRet(handlers.crbegin())

			PDefH(const_reverse_iterator, crend, ) const wnothrow
			ImplRet(handlers.crend())
			//@}

			PDefH(bool, empty, ) const wnothrow
			ImplRet(handlers.empty())

			PDefH(iterator, end, ) wnothrow
			ImplRet(handlers.end())
			PDefH(iterator, end, ) const wnothrow
			ImplRet(handlers.end())
			//@}

			//@{
			PDefH(reverse_iterator, rbegin, ) wnothrow
			ImplRet(handlers.rbegin())

			PDefH(reverse_iterator, rend, ) wnothrow
			ImplRet(handlers.rend())

			//! \brief ȡ�б��е���Ӧ����
			PDefH(size_type, size, ) const wnothrow
			ImplRet(handlers.size())
			//@}

			/*!
			\brief ������
			*/
			friend PDefH(void, swap, GEvent& x, GEvent& y) wnothrow
			ImplRet(x.handlers.swap(y.handlers))
	};
	//@}
	//@}

	/*!
	\brief ��ӵ�һ�¼���Ӧ��ɾ������ӡ�
	*/
	//@{
	template<typename _tRet, typename... _tParams>
	inline GEvent<_tRet(_tParams...)>&
		AddUnique(GEvent<_tRet(_tParams...)>& evt,
			const typename GEvent<_tRet(_tParams...)>::HandlerType& h,
			EventPriority prior = DefaultEventPriority)
	{
		return (evt -= h).Add(h, prior);
	}
	template<typename _tRet, typename... _tParams>
	inline GEvent<_tRet(_tParams...)>&
		AddUnique(GEvent<_tRet(_tParams...)>& evt, typename GEvent<_tRet(_tParams...)>
			::HandlerType&& h, EventPriority prior = DefaultEventPriority)
	{
		return (evt -= h).Add(std::move(h), prior);
	}
	template<typename _type, typename _tRet, typename... _tParams>
	inline GEvent<_tRet(_tParams...)>&
		AddUnique(GEvent<_tRet(_tParams...)>& evt, _type&& arg,
			EventPriority prior = DefaultEventPriority)
	{
		return AddUnique(evt, HandlerType(wforward(arg)), prior);
	}
	template<class _type, typename _tRet, typename... _tParams>
	inline GEvent<_tRet(_tParams...)>&
		AddUnique(GEvent<_tRet(_tParams...)>& evt, _type& obj,
			_tRet(_type::*pm)(_tParams...), EventPriority prior = DefaultEventPriority)
	{
		return AddUnique(evt, HandlerType(static_cast<_type&>(obj), std::move(pm)),
			prior);
	}
	//@}


	/*!
	\brief ʹ�� RAII ������¼������ࡣ
	\warning ����������
	*/
	template<typename... _tEventArgs>
	class GEventGuard
	{
	public:
		using EventType = GEvent<_tEventArgs...>;
		using HandlerType = GHEvent<_tEventArgs...>;
		lref<EventType> Event;
		HandlerType Handler;

		template<typename _type>
		GEventGuard(EventType& evt, _type&& handler,
			EventPriority prior = DefaultEventPriority)
			: Event(evt), Handler(wforward(handler))
		{
			Event.get().Add(Handler, prior);
		}
		~GEventGuard()
		{
			Event.get() -= Handler;
		}
	};


	/*!
	\brief �����¼���ģ�塣
	\warning ����������
	*/
	template<class _tEvent, class _tOwnerPointer = shared_ptr<_tEvent>>
	class GDependencyEvent : public GDependency<_tEvent, _tOwnerPointer>
	{
	public:
		using DependentType = typename GDependency<_tEvent>::DependentType;
		using PointerType = typename GDependency<_tEvent>::PointerType;
		using ConstReferenceType
			= typename GDependency<_tEvent>::ConstReferenceType;
		using ReferentType = typename GDependency<_tEvent>::ReferentType;
		using ReferenceType = typename GDependency<_tEvent>::ReferenceType;
		using EventType = DependentType;
		using SEventType = typename EventType::SEventType;
		using FuncType = typename EventType::FuncType;
		using HandlerType = typename EventType::HandlerType;
		using SizeType = typename EventType::SizeType;

		GDependencyEvent(PointerType p = PointerType())
			: GDependency<_tEvent>(p)
		{}

		/*!
		\brief ����¼���Ӧ��
		*/
		template<typename _type>
		inline ReferenceType
			operator+=(_type _arg)
		{
			return this->GetNewRef().operator+=(_arg);
		}

		/*!
		\brief �Ƴ��¼���Ӧ��
		*/
		template<typename _type>
		inline ReferenceType
			operator-=(_type _arg)
		{
			return this->GetNewRef().operator-=(_arg);
		}

		/*!
		\brief ����¼���Ӧ��ʹ�ö������úͳ�Ա����ָ�롣
		*/
		template<class _type, typename _tRet, typename... _tParams>
		inline ReferenceType
			Add(_type& obj, _tRet(_type::*pm)(_tParams...))
		{
			return this->GetNewRef().Add(obj, pm);
		}

		/*!
		\brief �Ƴ��¼���Ӧ��Ŀ��Ϊָ���������úͳ�Ա����ָ�롣
		*/
		template<class _type, typename _tRet, typename... _tParams>
		inline ReferenceType
			Remove(_type& obj, _tRet(_type::*pm)(_tParams...))
		{
			return this->GetNewRef().Remove(obj, pm);
		}

		/*!
		\brief ���ú�����
		*/
		template<typename... _tParams>
		inline SizeType
			operator()(_tParams&&... args) const
		{
			return this->GetRef().operator()(wforward(args)...);
		}

		/*!
		\brief ȡ�б��е���Ӧ����
		*/
		DefGetterMem(const wnothrow, SizeType, Size, this->GetRef())

			/*!
			\brief ������Ƴ������¼���Ӧ��
			*/
			PDefH(void, clear, )
			ImplExpr(this->GetNewRef().clear())
	};


	template<typename... _tParams>
	struct EventArgsHead
	{
		using type = conditional_t<sizeof...(_tParams) == 0, void,
			tuple_element_t<0, tuple<_tParams...>>>;
	};

	template<typename... _tParams>
	struct EventArgsHead<tuple<_tParams...>> : EventArgsHead<_tParams...>
	{};


	/*!
	\brief �¼�������������ģ�塣
	\warning ����������
	*/
	//@{
	template<typename _type, typename _func = std::function<void(_type&)>>
	class GHandlerAdaptor : private GHandlerAdaptor<void, _func>
	{
	private:
		using Base = GHandlerAdaptor<void, _func>;

	public:
		using typename Base::CallerType;

		using Base::Caller;
		lref<_type> ObjectRef;

		GHandlerAdaptor(_type& obj, CallerType f)
			: Base(f), ObjectRef(obj)
		{}
		template<typename _fCaller>
		GHandlerAdaptor(_type& obj, _fCaller&& f)
			: Base(make_expanded<CallerType>(wforward(f))), ObjectRef(obj)
		{}
		//@{
		DefDeCopyMoveCtorAssignment(GHandlerAdaptor)

			using Base::operator();
		//@}

		/*!
		\todo ʵ�ֱȽ� Function ��ȡ�
		*/
		friend PDefHOp(bool, == , const GHandlerAdaptor& x, const GHandlerAdaptor& y)
			wnothrow
			ImplRet(std::addressof(x.ObjectRef.get())
				== std::addressof(y.ObjectRef.get()))
	};

	template<typename _func>
	class GHandlerAdaptor<void, _func>
	{
	public:
		using CallerType = decay_t<_func>;

		CallerType Caller;

		GHandlerAdaptor(CallerType f)
			: Caller(f)
		{}
		template<typename _fCaller, wimpl(
			typename = exclude_self_t<GHandlerAdaptor, _fCaller>)>
			GHandlerAdaptor(_fCaller&& f)
			: Caller(make_expanded<CallerType>(wforward(f)))
		{}
		DefDeCopyCtor(GHandlerAdaptor)
			DefDeMoveCtor(GHandlerAdaptor)

			DefDeCopyAssignment(GHandlerAdaptor)
			DefDeMoveAssignment(GHandlerAdaptor)

			//! \todo ʹ�� <tt>noexpcept</tt> ��
			template<typename... _tParams>
		void
			operator()(_tParams&&... args) const
		{
			TryExpr(Caller(wforward(args)...))
				CatchIgnore(std::bad_function_call&)
		}

		/*!
		\todo ʵ�ֱȽ� Function ��ȡ�
		*/
		friend PDefHOp(bool, == , const GHandlerAdaptor& x, const GHandlerAdaptor& y)
			wnothrow
			ImplRet(&x == &y)
	};
	//@}


	/*!
	\brief �¼���װ��ģ�塣
	*/
	template<class _tEvent, typename _tBaseArgs>
	class GEventWrapper : public _tEvent, implements GIHEvent<_tBaseArgs>
	{
	public:
		using EventType = _tEvent;
		using BaseArgsType = _tBaseArgs;
		using EventArgsType = _t<EventArgsHead<typename _tEvent::TupleType>>;

		/*!
		\brief ί�е��á�
		\warning ��Ҫȷ�� BaseArgsType ���õĶ����ܹ�ת���� EventArgsType ��
		*/
		size_t
			operator()(BaseArgsType e) const ImplI(GIHEvent<_tBaseArgs>)
		{
			return EventType::operator()(EventArgsType(wforward(e)));
		}

		DefClone(const ImplI(GIHEvent<_tBaseArgs>), GEventWrapper)
	};


	/*!
	\brief �¼������͡�
	\warning ����������
	*/
	template<typename _tBaseArgs>
	class GEventPointerWrapper
	{
	public:
		using ItemType = GIHEvent<_tBaseArgs>;
		using PointerType = unique_ptr<ItemType>;

	private:
		PointerType ptr;

	public:
		template<typename _type, wimpl(
			typename = exclude_self_t<GEventPointerWrapper, _type>)>
			inline
			GEventPointerWrapper(_type&& p)
			wnoexcept(std::is_nothrow_constructible<PointerType, _type>())
			: ptr(Nonnull(p))
		{}
		/*!
		\brief ���ƹ��죺��ơ�
		*/
		GEventPointerWrapper(const GEventPointerWrapper& item)
			: ptr(white::clone_polymorphic(Deref(item.ptr)))
		{}
		DefDeMoveCtor(GEventPointerWrapper)

			wconstfn DefCvt(const wnothrow, const ItemType&, *ptr)
			wconstfn DefCvt(const wnothrow, ItemType&, *ptr)
	};


	/*!
	\brief ������չ�¼�ӳ���ࡣ
	*/
#define DefExtendEventMap(_n, _b) \
	DefExtendClass(WS_API, _n, public _b)

} // namespace white;

#endif
