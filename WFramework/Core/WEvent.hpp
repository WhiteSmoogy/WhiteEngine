/*! \file WEvent.hpp
\ingroup WFrameWork/Core
\brief 事件回调。

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
	\brief 事件处理器接口模板。
	*/
	template<typename... _tParams>
	DeclDerivedI(, GIHEvent, cloneable)
		DeclIEntry(size_t operator()(_tParams...) const)
		DeclIEntry(GIHEvent* clone() const ImplI(cloneable))
		EndDecl

		template<typename... _tParams>
	GIHEvent<_tParams...>::DefDeDtor(GIHEvent)


		/*!
		\brief 标准事件处理器模板。
		\note 若使用仿函数，可以不满足 \c EqualityComparable 的接口，即
		可使用返回 \c bool 的 \c operator== ，但此模板类无法检查其语义正确性。
		*/
		//@{
		template<typename>
	class GHEvent;

	//! \warning 非虚析构。
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
		//! \brief 比较函数类型。
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

		Comparer comp_eq; //!< 比较函数：相等关系。

	public:
		/*!
		\brief 构造：使用函数指针。
		*/
		wconstfn
			GHEvent(FuncType* f = {})
			: BaseType(f), comp_eq(GEquality<FuncType>::AreEqual)
		{}
		/*!
		\brief 使用函数对象。
		*/
		template<class _fCallable>
		wconstfn
			GHEvent(_fCallable f, white::enable_if_t<
				std::is_constructible<BaseType, _fCallable>::value, int> = 0)
			: BaseType(f), 
			comp_eq(GEquality<white::decay_t<_fCallable>>::AreEqual)
		{}
		/*!
		\brief 使用扩展函数对象。
		\todo 推断比较相等操作。
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
		\brief 构造：使用对象引用和成员函数指针。
		\warning 使用空成员指针构造的函数对象调用引起未定义行为。
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

		//! \brief 调用。
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
	\brief 事件优先级。
	*/
	using EventPriority = std::uint8_t;


	/*!
	\brief 默认事件优先级。
	*/
	wconstexpr const EventPriority DefaultEventPriority(0x80);


	//@{
	//! \brief 计数调用器：调用事件处理器并计数。
	struct CountedHandlerInvoker
	{
		/*!
		\exception std::bad_function_call 以外异常中立。
		\return 成功调用的事件处理器个数。
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


	//! \brief 结果组合调用器。
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


	//! \brief 默认结果组合调用器。
	template<typename _type>
	using GDefaultLastValueInvoker
		= GCombinerInvoker<_type, default_last_value<_type>>;

	//! \brief 可选结果组合调用器。
	template<typename _type>
	using GOptionalLastValueInvoker = GCombinerInvoker<cond_t<std::is_void<
		_type>, void, optional<_type>>,optional_last_value<_type>>;



	/*!
	\brief 事件模板。
	\note 支持顺序多播。
	*/
	//@{
	template<typename, typename = CountedHandlerInvoker>
	class GEvent;

	/*!
	\note 深复制。
	\warning 非虚析构。
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
		\brief 容器类型。
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
		\brief 调用器。
		*/
		InvokerType Invoker{};

	private:
		/*!
		\brief 响应列表。
		*/
		ContainerType handlers{};

	public:
		/*!
		\brief 无参数构造：默认实现。
		\note 得到空实例。
		*/
		wconstfn DefDeCtor(GEvent)
			/*!
			\brief 构造：使用指定调用器。
			*/
			GEvent(InvokerType ivk)
			: Invoker(std::move(ivk))
		{}
		/*!
		\brief 构造：使用指定调用器并添加事件处理器。
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
			\brief 赋值：覆盖事件响应：使用单一构造参数指定的指定事件处理器。
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
		\brief 添加事件响应：使用 const 事件处理器和优先级。
		\note 不检查是否已经在列表中。
		*/
		inline PDefHOp(GEvent&, +=, const HandlerType& h)
			ImplRet(Add(h))
			/*!
			\brief 添加事件响应：使用事件处理器。
			\note 不检查是否已经在列表中。
			*/
			inline PDefHOp(GEvent&, +=, HandlerType&& h)
			ImplRet(Add(std::move(h)))
			/*!
			\brief 添加事件响应：目标为单一构造参数指定的指定事件处理器。
			\note 不检查是否已经在列表中。
			*/
			template<typename _type>
		inline GEvent&
			operator+=(_type&& _arg)
		{
			return Add(HandlerType(wforward(_arg)));
		}

		//! \brief 移除事件响应：指定 const 事件处理器。
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
		\brief 移除事件响应：指定非 const 事件处理器。
		\note 防止模板 <tt>operator-=</tt> 递归。
		*/
		inline PDefHOp(GEvent&, -=, HandlerType&& h)
			ImplRet(*this -= static_cast<const HandlerType&>(h))
			/*!
			\brief 移除事件响应：目标为单一构造参数指定的指定事件处理器。
			*/
			template<typename _type>
		inline GEvent&
			operator-=(_type&& _arg)
		{
			return *this -= HandlerType(wforward(_arg));
		}
		//@}

		/*!
		\brief 插入事件响应。
		\note 不检查是否已经在列表中。
		\sa Insert
		*/
		//@{
		//! \note 使用 const 事件处理器和优先级。
		inline PDefH(GEvent&, Add, const HandlerType& h,
			EventPriority prior = DefaultEventPriority)
			ImplRet(Insert(h, prior), *this)
			//! \note 使用非 const 事件处理器和优先级。
			inline PDefH(GEvent&, Add, HandlerType&& h,
				EventPriority prior = DefaultEventPriority)
			ImplRet(Insert(std::move(h), prior), *this)
			//! \note 使用单一构造参数指定的事件处理器和优先级。
			template<typename _type>
		inline GEvent&
			Add(_type&& _arg, EventPriority prior = DefaultEventPriority)
		{
			return Add(HandlerType(wforward(_arg)), prior);
		}
		/*!
		\note 使用对象引用、成员函数指针和优先级。
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
		\brief 插入事件响应。
		\note 不检查是否已经在列表中。
		*/
		//@{
		//! \note 使用 const 事件处理器和优先级。
		inline PDefH(typename ContainerType::iterator, Insert, const HandlerType& h,
			EventPriority prior = DefaultEventPriority)
			ImplRet(handlers.emplace(prior, h))
			//! \note 使用非 const 事件处理器和优先级。
			inline PDefH(typename ContainerType::iterator, Insert, HandlerType&& h,
				EventPriority prior = DefaultEventPriority)
			ImplRet(handlers.emplace(prior, std::move(h)))
			//! \note 使用单一构造参数指定的事件处理器和优先级。
			template<typename _type>
		inline typename ContainerType::iterator
			Insert(_type&& _arg, EventPriority prior = DefaultEventPriority)
		{
			return Insert(HandlerType(wforward(_arg)), prior);
		}
		//! \note 使用对象引用、成员函数指针和优先级。
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
		\brief 移除事件响应：目标为指定对象引用和成员函数指针。
		*/
		template<class _tObj, class _type>
		inline GEvent&
			Remove(_tObj& obj, _tRet(_type::*pm)(_tParams...))
		{
			return *this -= HandlerType(static_cast<_type&>(obj), std::move(pm));
		}

		/*!
		\brief 判断是否包含指定事件响应。
		*/
		bool
			Contains(const HandlerType& h) const
		{
			using white::get_value;

			return std::count(handlers.cbegin() | get_value,
				handlers.cend() | get_value, h) != 0;
		}
		/*!
		\brief 判断是否包含单一构造参数指定的事件响应。
		*/
		template<typename _type>
		inline bool
			Contains(_type&& _arg) const
		{
			return Contains(HandlerType(wforward(_arg)));
		}

		/*!
		\brief 调用：传递参数到调用器。
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

			//! \brief 清除：移除所有事件响应。
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

			//! \brief 取列表中的响应数。
			PDefH(size_type, size, ) const wnothrow
			ImplRet(handlers.size())
			//@}

			/*!
			\brief 交换。
			*/
			friend PDefH(void, swap, GEvent& x, GEvent& y) wnothrow
			ImplRet(x.handlers.swap(y.handlers))
	};
	//@}
	//@}

	/*!
	\brief 添加单一事件响应：删除后添加。
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
	\brief 使用 RAII 管理的事件辅助类。
	\warning 非虚析构。
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
	\brief 依赖事件项模板。
	\warning 非虚析构。
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
		\brief 添加事件响应。
		*/
		template<typename _type>
		inline ReferenceType
			operator+=(_type _arg)
		{
			return this->GetNewRef().operator+=(_arg);
		}

		/*!
		\brief 移除事件响应。
		*/
		template<typename _type>
		inline ReferenceType
			operator-=(_type _arg)
		{
			return this->GetNewRef().operator-=(_arg);
		}

		/*!
		\brief 添加事件响应：使用对象引用和成员函数指针。
		*/
		template<class _type, typename _tRet, typename... _tParams>
		inline ReferenceType
			Add(_type& obj, _tRet(_type::*pm)(_tParams...))
		{
			return this->GetNewRef().Add(obj, pm);
		}

		/*!
		\brief 移除事件响应：目标为指定对象引用和成员函数指针。
		*/
		template<class _type, typename _tRet, typename... _tParams>
		inline ReferenceType
			Remove(_type& obj, _tRet(_type::*pm)(_tParams...))
		{
			return this->GetNewRef().Remove(obj, pm);
		}

		/*!
		\brief 调用函数。
		*/
		template<typename... _tParams>
		inline SizeType
			operator()(_tParams&&... args) const
		{
			return this->GetRef().operator()(wforward(args)...);
		}

		/*!
		\brief 取列表中的响应数。
		*/
		DefGetterMem(const wnothrow, SizeType, Size, this->GetRef())

			/*!
			\brief 清除：移除所有事件响应。
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
	\brief 事件处理器适配器模板。
	\warning 非虚析构。
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
		\todo 实现比较 Function 相等。
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

			//! \todo 使用 <tt>noexpcept</tt> 。
			template<typename... _tParams>
		void
			operator()(_tParams&&... args) const
		{
			TryExpr(Caller(wforward(args)...))
				CatchIgnore(std::bad_function_call&)
		}

		/*!
		\todo 实现比较 Function 相等。
		*/
		friend PDefHOp(bool, == , const GHandlerAdaptor& x, const GHandlerAdaptor& y)
			wnothrow
			ImplRet(&x == &y)
	};
	//@}


	/*!
	\brief 事件包装类模板。
	*/
	template<class _tEvent, typename _tBaseArgs>
	class GEventWrapper : public _tEvent, implements GIHEvent<_tBaseArgs>
	{
	public:
		using EventType = _tEvent;
		using BaseArgsType = _tBaseArgs;
		using EventArgsType = _t<EventArgsHead<typename _tEvent::TupleType>>;

		/*!
		\brief 委托调用。
		\warning 需要确保 BaseArgsType 引用的对象能够转换至 EventArgsType 。
		*/
		size_t
			operator()(BaseArgsType e) const ImplI(GIHEvent<_tBaseArgs>)
		{
			return EventType::operator()(EventArgsType(wforward(e)));
		}

		DefClone(const ImplI(GIHEvent<_tBaseArgs>), GEventWrapper)
	};


	/*!
	\brief 事件项类型。
	\warning 非虚析构。
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
		\brief 复制构造：深复制。
		*/
		GEventPointerWrapper(const GEventPointerWrapper& item)
			: ptr(white::clone_polymorphic(Deref(item.ptr)))
		{}
		DefDeMoveCtor(GEventPointerWrapper)

			wconstfn DefCvt(const wnothrow, const ItemType&, *ptr)
			wconstfn DefCvt(const wnothrow, ItemType&, *ptr)
	};


	/*!
	\brief 定义扩展事件映射类。
	*/
#define DefExtendEventMap(_n, _b) \
	DefExtendClass(WS_API, _n, public _b)

} // namespace white;

#endif
