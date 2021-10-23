/*! \file ValueNode.h
\ingroup WFrameWork/Core
\brief 值类型节点。
\par 修改时间:
2017-03-24 09:50 +0800
*/
#ifndef WFramework_Core_ValueNode_H
#define WFramework_Core_ValueNode_H 1

#include <WBase/set.hpp>
#include <WBase/path.hpp>
#include <numeric>
#include <WFramework/Core/WObject.h>

namespace white {
	wconstexpr const struct ListContainerTag {} ListContainer{};

	wconstexpr const struct NoContainerTag {} NoContainer{};

	class WF_API ValueNode : private totally_ordered<ValueNode>,
		private totally_ordered<ValueNode, string>
	{
	public:
		using Container = mapped_set<ValueNode, less<>>;
		using key_type = typename Container::key_type;
		using iterator = Container::iterator;
		using const_iterator = Container::const_iterator;
		using reverse_iterator = Container::reverse_iterator;
		using const_reverse_iterator = Container::const_reverse_iterator;
	private:
		string name{};
		/*!
		\brief 子节点容器。
		*/
		Container container{};

	public:
		ValueObject Value{};

		DefDeCtor(ValueNode)
			/*!
			\brief 构造：使用容器对象。
			*/
			ValueNode(Container con)
			: container(std::move(con))
		{}
		/*!
		\brief 构造：使用字符串引用和值类型对象构造参数。
		\note 不使用容器。
		*/
		template<typename _tString, typename... _tParams>
		inline
			ValueNode(NoContainerTag, _tString&& str, _tParams&&... args)
			: name(wforward(str)), Value(wforward(args)...)
		{}
		/*!
		\brief 构造：使用容器对象、字符串引用和值类型对象构造参数。
		*/
		template<typename _tString, typename... _tParams>
		ValueNode(Container con, _tString&& str, _tParams&&... args)
			: name(wforward(str)), container(std::move(con)),
			Value(wforward(args)...)
		{}
		/*!
		\brief 构造：使用输入迭代器对。
		*/
		template<typename _tIn>
		inline
			ValueNode(const pair<_tIn, _tIn>& pr)
			: container(pr.first, pr.second)
		{}
		/*!
		\brief 构造：使用输入迭代器对、字符串引用和值参数。
		*/
		template<typename _tIn, typename _tString>
		inline
			ValueNode(const pair<_tIn, _tIn>& pr, _tString&& str)
			: name(wforward(str)), container(pr.first, pr.second)
		{}
		/*!
		\brief 原地构造：使用容器、名称和值的参数元组。
		*/
		//@{
		template<typename... _tParams1>
		inline
			ValueNode(std::tuple<_tParams1...> args1)
			: container(white::make_from_tuple<Container>(args1))
		{}
		template<typename... _tParams1, typename... _tParams2>
		inline
			ValueNode(std::tuple<_tParams1...> args1, std::tuple<_tParams2...> args2)
			: name(white::make_from_tuple<string>(args2)),
			container(white::make_from_tuple<Container>(args1))
		{}
		template<typename... _tParams1, typename... _tParams2,
			typename... _tParams3>
			inline
			ValueNode(std::tuple<_tParams1...> args1, std::tuple<_tParams2...> args2,
				std::tuple<_tParams3...> args3)
			: name(white::make_from_tuple<string>(args2)),
			container(white::make_from_tuple<Container>(args1)),
			Value(white::make_from_tuple<ValueObject>(args3))
		{}
		//@}

		DefDeCopyMoveCtor(ValueNode)

			/*!
			\brief 合一赋值：使用值参数和交换函数进行复制或转移赋值。
			*/
			PDefHOp(ValueNode&, =, ValueNode node) wnothrow
			ImplRet(white::copy_and_swap(*this,node))

			DefBoolNeg(explicit, bool(Value) || !container.empty())

			//@{
			PDefHOp(const ValueNode&, +=, const ValueNode& node)
			ImplRet(Add(node), *this)
			PDefHOp(const ValueNode&, +=, ValueNode&& node)
			ImplRet(Add(std::move(node)), *this)

			PDefHOp(const ValueNode&, -=, const ValueNode& node)
			ImplRet(Remove(node), *this)
			PDefHOp(const ValueNode&, -=, const string& str)
			ImplRet(Remove(str), *this)
			/*!
			\brief 替换同名子节点。
			\return 自身引用。
			*/
			//@{
			PDefHOp(ValueNode&, /=, const ValueNode& node)
			ImplRet(*this %= node, *this)
			PDefHOp(ValueNode&, /=, ValueNode&& node)
			ImplRet(*this %= std::move(node), *this)
			//@}
			/*!
			\brief 替换同名子节点。
			\return 子节点引用。
			*/
			//@{
			const ValueNode&
			operator%=(const ValueNode&);
		const ValueNode&
			operator%=(const ValueNode&&);
		//@}
		//@}

		friend PDefHOp(bool, == , const ValueNode& x, const ValueNode& y) wnothrow
			ImplRet(x.name == y.name)
			friend PDefHOp(bool, == , const ValueNode& x, const string& str) wnothrow
			ImplRet(x.name == str)
			template<typename _tKey>
		friend bool
			operator==(const ValueNode& x, const _tKey& str) wnothrow
		{
			return x.name == str;
		}

		friend PDefHOp(bool, <, const ValueNode& x, const ValueNode& y) wnothrow
			ImplRet(x.name < y.name)
			friend PDefHOp(bool, <, const ValueNode& x, const string& str) wnothrow
			ImplRet(x.name < str)
			template<typename _tKey>
		friend bool
			operator<(const ValueNode& x, const _tKey& str) wnothrow
		{
			return x.name < str;
		}
		template<typename _tKey>
		friend bool
			operator<(const _tKey& str, const ValueNode& y) wnothrow
		{
			return str < y.name;
		}
		friend PDefHOp(bool, >, const ValueNode& x, const string& str) wnothrow
			ImplRet(x.name > str)
			template<typename _tKey>
		friend bool
			operator>(const ValueNode& x, const _tKey& str) wnothrow
		{
			return x.name > str;
		}
		template<typename _tKey>
		friend bool
			operator>(const _tKey& str, const ValueNode& y) wnothrow
		{
			return str > y.name;
		}

		template<typename _tString>
		ValueNode&
			operator[](_tString&& str)
		{
			return *white::try_emplace(container, str, NoContainer, wforward(str))
				.first;
		}
		template<class _tCon>
		const ValueNode&
			operator[](const lpath<_tCon>& pth)
		{
			auto p(this);

			for (const auto& n : pth)
				p = &(*p)[n];
			return *p;
		}

		/*!
		\brief 取子节点容器引用。
		*/
		DefGetter(const wnothrow, const Container&, Container, container)
			/*!
			\brief 取子节点容器引用。
			*/
			DefGetter(wnothrow, Container&, ContainerRef, container)
			DefGetter(const wnothrow, const string&, Name, name)

			//@{
			//! \brief 设置子节点容器内容。
			PDefH(void, SetChildren, const Container& con)
			ImplExpr(container = con)
			PDefH(void, SetChildren, Container&& con)
			ImplExpr(container = std::move(con))
			PDefH(void, SetChildren, ValueNode&& node)
			ImplExpr(container = std::move(node.container))
			/*!
			\note 设置子节点容器和值的内容。
			*/
			//@{
			template<class _tCon, class _tValue>
			wimpl(white::enable_if_t)<
			white::and_<std::is_assignable<Container, _tCon&&>,
			std::is_assignable<ValueObject, _tValue&&>>::value>
			SetContent(_tCon&& con, _tValue&& val) wnoexcept(white::and_<
				std::is_nothrow_assignable<Container, _tCon&&>,
				std::is_nothrow_assignable<ValueObject, _tValue&&>>())
			{
				wunseq(container = wforward(con), Value = wforward(val));
			}
			PDefH(void, SetContent, const ValueNode& node)
			ImplExpr(SetContent(node.GetContainer(), node.Value))
			PDefH(void, SetContent, ValueNode&& node)
			ImplExpr(SwapContent(node))
			//@}

			void
			SetContentIndirect(Container, const ValueObject&) wnothrow;
		PDefH(void, SetContentIndirect, const ValueNode& node)
			ImplExpr(SetContentIndirect(node.GetContainer(), node.Value))

			PDefH(bool, Add, const ValueNode& node)
			ImplRet(insert(node).second)
			PDefH(bool, Add, ValueNode&& node)
			ImplRet(insert(std::move(node)).second)

			//@{
			template<typename _tKey>
		bool
			AddChild(_tKey&& k, const ValueNode& node)
		{
			return emplace(node.GetContainer(), wforward(k), node.Value).second;
		}
		template<typename _tKey>
		bool
			AddChild(_tKey&& k, ValueNode&& node)
		{
			return emplace(std::move(node.GetContainerRef()), wforward(k),
				std::move(node.Value)).second;
		}
		template<typename _tKey>
		void
			AddChild(const_iterator hint, _tKey&& k, const ValueNode& node)
		{
			return emplace_hint(hint, node.GetContainer(), wforward(k), node.Value);
		}
		template<typename _tKey>
		void
			AddChild(const_iterator hint, _tKey&& k, ValueNode&& node)
		{
			return emplace_hint(hint, std::move(node.GetContainerRef()),
				wforward(k), std::move(node.Value));
		}
		//@}

			template<typename _tString, typename... _tParams>
		inline bool
			AddValue(_tString&& str, _tParams&&... args)
		{
			return AddValueTo(container, wforward(str), wforward(args)...);
		}

		template<typename _tString, typename... _tParams>
		static bool
			AddValueTo(Container& con, _tString&& str, _tParams&&... args)
		{
			const auto pr(con.equal_range(str));

			if (pr.first == pr.second)
			{
				con.emplace_hint(pr.first, NoContainer, wforward(str),
					wforward(args)...);
				return true;
			}
			return{};
		}

		//! \note 清理容器和修改值的操作之间的顺序未指定。
		//@{
		/*!
		\brief 清除节点。
		\post <tt>!Value && empty()</tt> 。
		*/
		PDefH(void, Clear, ) wnothrow
			ImplExpr(Value.Clear(), ClearContainer())

		/*!
		\brief 清除容器并设置值。
		*/
		PDefH(void, ClearTo, ValueObject vo) wnothrow
		ImplExpr(ClearContainer(), Value = std::move(vo))
		//@}

		/*!
		\brief 清除节点容器。
		\post \c empty() 。
		*/
		PDefH(void, ClearContainer, ) wnothrow
		ImplExpr(container.clear())
		//@}

		/*!
		\brief 递归创建容器副本。
		*/
		//@{
		static Container
		CreateRecursively(const Container&, IValueHolder::Creation);
		template<typename _fCallable>
		static Container
			CreateRecursively(Container& con, _fCallable f)
		{
			Container res;

			for (auto& tm : con)
				res.emplace(CreateRecursively(tm.GetContainerRef(), f),
					tm.GetName(), white::invoke(f, tm.Value));
			return res;
		}
		template<typename _fCallable>
		static Container
			CreateRecursively(const Container& con, _fCallable f)
		{
			Container res;

			for (auto& tm : con)
				res.emplace(CreateRecursively(tm.GetContainer(), f), tm.GetName(),
					white::invoke(f, tm.Value));
			return res;
		}

		PDefH(Container, CreateWith, IValueHolder::Creation c) const
		ImplRet(CreateRecursively(container, c))


		template<typename _fCallable>
		Container
			CreateWith(_fCallable f)
		{
			return CreateRecursively(container, f);
		}
		template<typename _fCallable>
		Container
			CreateWith(_fCallable f) const
		{
			return CreateRecursively(container, f);
		}
		//@}

		/*!
		\brief 若指定名称子节点不存在则按指定值初始化。
		\return 按指定名称查找的指定类型的子节点的值的引用。
		*/
		template<typename _type, typename _tString, typename... _tParams>
		inline _type&
			Place(_tString&& str, _tParams&&... args)
		{
			return this->try_emplace(str, NoContainer, wforward(str),
				white::in_place<_type>, wforward(args)...).first->Value.template
				GetObject<_type>();
		}

		PDefH(bool, Remove, const ValueNode& node)
			ImplRet(erase(node) != 0)
			PDefH(iterator, Remove, const_iterator i)
			ImplRet(erase(i))

			template<typename _tKey, wimpl(typename = white::enable_if_t<
				white::is_interoperable<const _tKey&, const string&>::value>)>
		inline bool
			Remove(const _tKey& k)
		{
			return white::erase_first(container, k);
		}

		/*!
		\brief 复制满足条件的子节点。
		*/
		template<typename _func>
		Container
			SelectChildren(_func f) const
		{
			Container res;

			for_each_if(begin(), end(), f, [&](const ValueNode& nd) {
				res.insert(nd);
			});
			return res;
		}

		//@{
		//! \brief 转移满足条件的子节点。
		template<typename _func>
		Container
			SplitChildren(_func f)
		{
			Container res;

			std::for_each(begin(), end(), [&](const ValueNode& nd) {
				container.emplace(NoContainer, res, nd.GetName());
			});
			for_each_if(begin(), end(), f, [&, this](const ValueNode& nd) {
				const auto& child_name(nd.GetName());

				Deref(res.find(child_name)).Value = std::move(nd.Value);
				Remove(child_name);
			});
			return res;
		}

		//! \warning 不检查容器之间的所有权，保持循环引用状态析构引起未定义行为。
		//@{
		//! \brief 交换容器。
		PDefH(void, SwapContainer, ValueNode& node) wnothrow
			ImplExpr(container.swap(node.container))

			//! \brief 交换容器和值。
			void
			SwapContent(ValueNode&) wnothrow;
		//@}
		//@}

		/*!
		\brief 抛出索引越界异常。
		\throw std::out_of_range 索引越界。
		*/
		WB_NORETURN static void
			ThrowIndexOutOfRange();

		/*!
		\brief 抛出名称错误异常。
		\throw std::out_of_range 名称错误。
		*/
		WB_NORETURN static void
			ThrowWrongNameFound();

		//@{
		PDefH(iterator, begin, )
			ImplRet(GetContainerRef().begin())
			PDefH(const_iterator, begin, ) const
			ImplRet(GetContainer().begin())

			DefFwdTmpl(, pair<iterator WPP_Comma bool>, emplace,
				container.emplace(wforward(args)...))

			DefFwdTmpl(, iterator, emplace_hint,
				container.emplace_hint(wforward(args)...))

			PDefH(bool, empty, ) const wnothrow
			ImplRet(container.empty())

			PDefH(iterator, end, )
			ImplRet(GetContainerRef().end())
			PDefH(const_iterator, end, ) const
			ImplRet(GetContainer().end())
			//@}

			DefFwdTmpl(-> decltype(container.erase(wforward(args)...)), auto,
				erase, container.erase(wforward(args)...))

			DefFwdTmpl(-> decltype(container.insert(wforward(args)...)), auto,
				insert, container.insert(wforward(args)...))

			//@{
			template<typename _tKey, class _tParam>
		wimpl(enable_if_inconvertible_t)<_tKey&&, const_iterator,
			std::pair<iterator, bool>>
			insert_or_assign(_tKey&& k, _tParam&& arg)
		{
			return insert_or_assign(container, wforward(k), wforward(arg));
		}
		template<typename _tKey, class _tParam>
		iterator
			insert_or_assign(const_iterator hint, _tKey&& k, _tParam&& arg)
		{
			return insert_or_assign_hint(container, hint, wforward(k),
				wforward(arg));
		}
		//@}

		//@{
		PDefH(reverse_iterator, rbegin, )
			ImplRet(GetContainerRef().rbegin())
			PDefH(const_reverse_iterator, rbegin, ) const
			ImplRet(GetContainer().rbegin())

			PDefH(reverse_iterator, rend, )
			ImplRet(GetContainerRef().rend())
			PDefH(const_reverse_iterator, rend, ) const
			ImplRet(GetContainer().rend())
			//@}

			/*!
			\sa mapped_set
			\sa set_value_move
			*/
			friend PDefH(ValueNode, set_value_move, ValueNode& node)
			ImplRet({ std::move(node.GetContainerRef()), node.GetName(),
				std::move(node.Value) })

			PDefH(size_t, size, ) const wnothrow
			ImplRet(container.size())

			/*!
			\brief 交换。
			*/
			WF_API friend void
			swap(ValueNode&, ValueNode&) wnothrow;

		//@{
		template<typename _tKey, typename... _tParams>
		wimpl(enable_if_inconvertible_t)<_tKey&&, const_iterator,
			std::pair<iterator, bool>>
			try_emplace(_tKey&& k, _tParams&&... args)
		{
			return white::try_emplace(container, wforward(k),std::forward<_tParams>(args)...);
		}
		template<typename _tKey, typename... _tParams>
		iterator
			try_emplace(const_iterator hint, _tKey&& k, _tParams&&... args)
		{
			return try_emplace_hint(container, hint, wforward(k),
				std::forward<_tParams>(args)...);
		}
		//@}
	};

	/*!
	\relates ValueNode
	*/
	//@{
	/*!
	\brief 访问节点的指定类型对象。
	\exception std::bad_cast 空实例或类型检查失败 。
	*/
	//@{
	template<typename _type>
	inline _type&
		Access(ValueNode& node)
	{
		return node.Value.Access<_type>();
	}
	template<typename _type>
	inline const _type&
		Access(const ValueNode& node)
	{
		return node.Value.Access<_type>();
	}
	//@}

	//@{
	//! \brief 访问节点的指定类型对象指针。
	//@{
	template<typename _type>
	inline observer_ptr<_type>
		AccessPtr(ValueNode& node) wnothrow
	{
		return node.Value.AccessPtr<_type>();
	}
	template<typename _type>
	inline observer_ptr<const _type>
		AccessPtr(const ValueNode& node) wnothrow
	{
		return node.Value.AccessPtr<_type>();
	}
	//@}
	//! \brief 访问节点的指定类型对象指针。
	//@{
	template<typename _type, typename _tNodeOrPointer>
	inline auto
		AccessPtr(observer_ptr<_tNodeOrPointer> p) wnothrow
		-> decltype(white::AccessPtr<_type>(*p))
	{
		return p ? white::AccessPtr<_type>(*p) : nullptr;
	}
	//@}
	//@}
	//@}

	//@{
	//! \brief 取指定名称指称的值。
	WF_API ValueObject
		GetValueOf(observer_ptr<const ValueNode>);

	//! \brief 取指定名称指称的值的指针。
	WF_API observer_ptr<const ValueObject>
		GetValuePtrOf(observer_ptr<const ValueNode>);
	//@}

	//@{
	template<typename _tKey>
	observer_ptr<ValueNode>
		AccessNodePtr(ValueNode::Container*, const _tKey&) wnothrow;
	template<typename _tKey>
	observer_ptr<const ValueNode>
		AccessNodePtr(const ValueNode::Container*, const _tKey&) wnothrow;

	/*!
	\brief 访问节点。
	\throw std::out_of_range 未找到对应节点。
	*/
	//@{
	WF_API ValueNode&
		AccessNode(ValueNode::Container*, const string&);
	WF_API const ValueNode&
		AccessNode(const ValueNode::Container*, const string&);
	template<typename _tKey>
	ValueNode&
		AccessNode(ValueNode::Container* p_con, const _tKey& name)
	{
		if (const auto p = AccessNodePtr(p_con, name))
			return *p;
		ValueNode::ThrowWrongNameFound();
	}
	template<typename _tKey>
	const ValueNode&
		AccessNode(const ValueNode::Container* p_con, const _tKey& name)
	{
		if (const auto p = AccessNodePtr(p_con, name))
			return *p;
		ValueNode::ThrowWrongNameFound();
	}
	template<typename _tKey>
	inline ValueNode&
		AccessNode(observer_ptr<ValueNode::Container> p_con, const _tKey& name)
	{
		return AccessNode(p_con.get(), name);
	}
	template<typename _tKey>
	inline const ValueNode&
		AccessNode(observer_ptr<const ValueNode::Container> p_con, const _tKey& name)
	{
		return AccessNode(p_con.get(), name);
	}
	template<typename _tKey>
	inline ValueNode&
		AccessNode(ValueNode::Container& con, const _tKey& name)
	{
		return AccessNode(&con, name);
	}
	template<typename _tKey>
	inline const ValueNode&
		AccessNode(const ValueNode::Container& con, const _tKey& name)
	{
		return AccessNode(&con, name);
	}
	/*!
	\note 时间复杂度 O(n) 。
	*/
	//@{
	WF_API ValueNode&
		AccessNode(ValueNode&, size_t);
	WF_API const ValueNode&
		AccessNode(const ValueNode&, size_t);
	//@}
	template<typename _tKey, wimpl(typename = white::enable_if_t<
		or_<std::is_constructible<const _tKey&, const string&>,
		std::is_constructible<const string&, const _tKey&>>::value>)>
		inline ValueNode&
		AccessNode(ValueNode& node, const _tKey& name)
	{
		return AccessNode(node.GetContainerRef(), name);
	}
	template<typename _tKey, wimpl(typename = white::enable_if_t<
		or_<std::is_constructible<const _tKey&, const string&>,
		std::is_constructible<const string&, const _tKey&>>::value>)>
		inline const ValueNode&
		AccessNode(const ValueNode& node, const _tKey& name)
	{
		return AccessNode(node.GetContainer(), name);
	}
	//@{
	//! \note 使用 ADL \c AccessNode 。
	template<class _tNode, typename _tIn>
	_tNode&&
		AccessNode(_tNode&& node, _tIn first, _tIn last)
	{
		return std::accumulate(first, last, ref(node),
			[](_tNode&& nd, decltype(*first) c) {
			return ref(AccessNode(nd, c));
		});
	}
	//! \note 使用 ADL \c begin 和 \c end 指定范围迭代器。
	template<class _tNode, typename _tRange,
		wimpl(typename = white::enable_if_t<
			!std::is_constructible<const string&, const _tRange&>::value>)>
		inline auto
		AccessNode(_tNode&& node, const _tRange& c)
		-> decltype(AccessNode(wforward(node), begin(c), end(c)))
	{
		return AccessNode(wforward(node), begin(c), end(c));
	}
	//@}
	//@}

	//! \brief 访问节点指针。
	//@{
	WF_API observer_ptr<ValueNode>
		AccessNodePtr(ValueNode::Container&, const string&) wnothrow;
	WF_API observer_ptr<const ValueNode>
		AccessNodePtr(const ValueNode::Container&, const string&) wnothrow;
	template<typename _tKey>
	observer_ptr<ValueNode>
		AccessNodePtr(ValueNode::Container& con, const _tKey& name) wnothrow
	{
		return make_observer(call_value_or<ValueNode*>(addrof<>(),
			con.find(name), {}, end(con)));
	}
	template<typename _tKey>
	observer_ptr<const ValueNode>
		AccessNodePtr(const ValueNode::Container& con, const _tKey& name) wnothrow
	{
		return make_observer(call_value_or<const ValueNode*>(
			addrof<>(), con.find(name), {}, end(con)));
	}
	template<typename _tKey>
	inline observer_ptr<ValueNode>
		AccessNodePtr(ValueNode::Container* p_con, const _tKey& name) wnothrow
	{
		return p_con ? AccessNodePtr(*p_con, name) : nullptr;
	}
	template<typename _tKey>
	inline observer_ptr<const ValueNode>
		AccessNodePtr(const ValueNode::Container* p_con, const _tKey& name) wnothrow
	{
		return p_con ? AccessNodePtr(*p_con, name) : nullptr;
	}
	template<typename _tKey>
	inline observer_ptr<ValueNode>
		AccessNodePtr(observer_ptr<ValueNode::Container> p_con, const _tKey& name)
		wnothrow
	{
		return p_con ? AccessNodePtr(*p_con, name) : nullptr;
	}
	template<typename _tKey>
	inline observer_ptr<const ValueNode>
		AccessNodePtr(observer_ptr<const ValueNode::Container> p_con, const _tKey& name)
		wnothrow
	{
		return p_con ? AccessNodePtr(*p_con, name) : nullptr;
	}
	/*!
	\note 时间复杂度 O(n) 。
	*/
	//@{
	WF_API observer_ptr<ValueNode>
		AccessNodePtr(ValueNode&, size_t);
	WF_API observer_ptr<const ValueNode>
		AccessNodePtr(const ValueNode&, size_t);
	//@}
	template<typename _tKey, wimpl(typename = white::enable_if_t<
		or_<std::is_constructible<const _tKey&, const string&>,
		std::is_constructible<const string&, const _tKey&>>::value>)>
		inline observer_ptr<ValueNode>
		AccessNodePtr(ValueNode& node, const _tKey& name)
	{
		return AccessNodePtr(node.GetContainerRef(), name);
	}
	template<typename _tKey, wimpl(typename = white::enable_if_t<
		or_<std::is_constructible<const _tKey&, const string&>,
		std::is_constructible<const string&, const _tKey&>>::value>)>
		inline observer_ptr<const ValueNode>
		AccessNodePtr(const ValueNode& node, const _tKey& name)
	{
		return AccessNodePtr(node.GetContainer(), name);
	}
	//@{
	//! \note 使用 ADL \c AccessNodePtr 。
	template<class _tNode, typename _tIn>
	auto
		AccessNodePtr(_tNode&& node, _tIn first, _tIn last)
		-> decltype(make_obsrever(std::addressof(node)))
	{
		// TODO: Simplified using algorithm template?
		for (auto p(make_observer(std::addressof(node))); p && first != last;
			++first)
			p = AccessNodePtr(*p, *first);
		return first;
	}
	//! \note 使用 ADL \c begin 和 \c end 指定范围迭代器。
	template<class _tNode, typename _tRange,
		wimpl(typename = white::enable_if_t<
			!std::is_constructible<const string&, const _tRange&>::value>)>
		inline auto
		AccessNodePtr(_tNode&& node, const _tRange& c)
		-> decltype(AccessNodePtr(wforward(node), begin(c), end(c)))
	{
		return AccessNodePtr(wforward(node), begin(c), end(c));
	}
	//@}
	//@}
	//@}


	//@{
	/*!
	\exception std::bad_cast 空实例或类型检查失败 。
	\relates ValueNode
	*/
	//@{
	/*!
	\brief 访问子节点的指定类型对象。
	\note 使用 ADL \c AccessNode 。
	*/
	//@{
	template<typename _type, typename... _tParams>
	inline _type&
		AccessChild(ValueNode& node, _tParams&&... args)
	{
		return Access<_type>(AccessNode(node, wforward(args)...));
	}
	template<typename _type, typename... _tParams>
	inline const _type&
		AccessChild(const ValueNode& node, _tParams&&... args)
	{
		return Access<_type>(AccessNode(node, wforward(args)...));
	}
	//@}

	//! \brief 访问指定名称的子节点的指定类型对象的指针。
	//@{
	template<typename _type, typename... _tParams>
	inline observer_ptr<_type>
		AccessChildPtr(ValueNode& node, _tParams&&... args) wnothrow
	{
		return AccessPtr<_type>(
			AccessNodePtr(node.GetContainerRef(), wforward(args)...));
	}
	template<typename _type, typename... _tParams>
	inline observer_ptr<const _type>
		AccessChildPtr(const ValueNode& node, _tParams&&... args) wnothrow
	{
		return AccessPtr<_type>(
			AccessNodePtr(node.GetContainer(), wforward(args)...));
	}
	template<typename _type, typename... _tParams>
	inline observer_ptr<_type>
		AccessChildPtr(ValueNode* p_node, _tParams&&... args) wnothrow
	{
		return p_node ? AccessChildPtr<_type>(*p_node, wforward(args)...) : nullptr;
	}
	template<typename _type, typename... _tParams>
	inline observer_ptr<const _type>
		AccessChildPtr(const ValueNode* p_node, _tParams&&... args) wnothrow
	{
		return p_node ? AccessChildPtr<_type>(*p_node, wforward(args)...) : nullptr;
	}
	//@}
	//@}
	//@}


	//! \note 结果不含子节点。
	//@{
	inline PDefH(const ValueNode&, AsNode, const ValueNode& node)
		ImplRet(node)
		/*!
		\brief 传递指定名称和值参数构造值类型节点。
		*/
		template<typename _tString, typename... _tParams>
	inline ValueNode
		AsNode(_tString&& str, _tParams&&... args)
	{
		return{ NoContainer, wforward(str), std::forward<_tParams>(args)... };
	}

	/*!
	\brief 传递指定名称和退化值参数构造值类型节点。
	*/
	template<typename _tString, typename... _tParams>
	inline ValueNode
		MakeNode(_tString&& str, _tParams&&... args)
	{
		return{ NoContainer, wforward(str), white::decay_copy(args)... };
	}
	//@}

	/*!
	\brief 取指定名称和转换为字符串的值类型节点。
	\note 使用非限定 to_string 转换。
	*/
	template<typename _tString, typename... _tParams>
	inline ValueNode
		StringifyToNode(_tString&& str, _tParams&&... args)
	{
		return{ NoContainer, wforward(str), to_string(wforward(args)...) };
	}

	/*!
	\brief 从引用参数取值类型节点：返回自身。
	*/
	//@{
	inline PDefH(const ValueNode&, UnpackToNode, const ValueNode& arg)
		ImplRet(arg)
		inline PDefH(ValueNode&&, UnpackToNode, ValueNode&& arg)
		ImplRet(std::move(arg))
		//@}


		/*!
		\brief 从参数取以指定分量为初始化参数的值类型节点。
		\note 取分量同 std::get ，但使用 ADL 。仅取前两个分量。
		*/
		template<class _tPack>
	inline ValueNode
		UnpackToNode(_tPack&& pk)
	{
		return{ 0, get<0>(wforward(pk)),
			ValueObject(white::decay_copy(get<1>(wforward(pk)))) };
	}

	/*!
	\brief 取指定值类型节点为成员的节点容器。
	*/
	//@{
	template<typename _tElem>
	inline ValueNode::Container
		CollectNodes(std::initializer_list<_tElem> il)
	{
		return il;
	}
	template<typename... _tParams>
	inline ValueNode::Container
		CollectNodes(_tParams&&... args)
	{
		return{ wforward(args)... };
	}
	//@}

	/*!
	\brief 取以指定分量为参数对应初始化得到的值类型节点为子节点的值类型节点。
	*/
	template<typename _tString, typename... _tParams>
	inline ValueNode
		PackNodes(_tString&& name, _tParams&&... args)
	{
		return{ CollectNodes(UnpackToNode(wforward(args))...), wforward(name) };
	}


	//@{
	//! \brief 移除空子节点。
	WF_API void
		RemoveEmptyChildren(ValueNode::Container&) wnothrow;

	/*!
	\brief 移除首个子节点。
	\pre 断言：节点非空。
	*/
	//@{
	WF_API void
		RemoveHead(ValueNode::Container&) wnothrowv;
	inline PDefH(void, RemoveHead, ValueNode& term) wnothrowv
		ImplExpr(RemoveHead(term.GetContainerRef()))
		//@}
		//@}

	template<typename _tNode, typename _fCallable>
	void
		SetContentWith(ValueNode& dst, _tNode& node, _fCallable f)
	{
		wunseq(
			dst.Value = white::invoke(f, node.Value),
			dst.GetContainerRef() = node.CreateWith(f)
		);
	}

	/*!
	\brief 判断字符串是否是一个指定字符和非负整数的组合。
	\pre 断言：字符串参数的数据指针非空。
	\note 仅测试能被 <tt>unsigned long</tt> 表示的整数。
	*/
	WF_API bool
	IsPrefixedIndex(string_view, char = '$');

	/*!
	\brief 转换节点大小为新的节点索引值。
	\return 保证 4 位十进制数内按字典序递增的字符串。
	\throw std::invalid_argument 索引值过大，不能以 4 位十进制数表示。
	\note 重复使用作为新节点的名称，可用于插入不重复节点。
	*/
	//@{
	WF_API string
		MakeIndex(size_t);
	inline PDefH(string, MakeIndex, const ValueNode::Container& con)
		ImplRet(MakeIndex(con.size()))
		inline PDefH(string, MakeIndex, const ValueNode& node)
		ImplRet(MakeIndex(node.GetContainer()))
	//@}

	/*!
	\brief 取最后一个子节点名称的前缀索引。
	\return 若不存在子节点则为 \c size_t(-1) ，否则为最后一个子节点的名称对应的索引。
	\throw std::invalid_argument 存在子节点但名称不是前缀索引。
	\sa IsPrefixedIndex
	*/
	//@{
	WF_API size_t
	GetLastIndexOf(const ValueNode::Container&);
	inline PDefH(size_t, GetLastIndexOf, const ValueNode& term)
		ImplRet(GetLastIndexOf(term.GetContainer()))
	//@}

		template<typename _tParam, typename... _tParams>
	inline ValueNode
		AsIndexNode(_tParam&& arg, _tParams&&... args)
	{
		return AsNode(MakeIndex(wforward(arg)), wforward(args)...);
	}


	/*!
	\brief 节点序列容器。

	除分配器外满足和 std::vector 相同的要求的模板的一个实例，元素为 ValueNode 类型。
	*/
	using NodeSequence = wimpl(white::vector)<ValueNode>;


	/*!
	\brief 包装节点的组合字面量。
	*/
	class WF_API NodeLiteral final
	{
	private:
		ValueNode node;

	public:
		NodeLiteral(const ValueNode& nd)
			: node(nd)
		{}
		NodeLiteral(ValueNode&& nd)
			: node(std::move(nd))
		{}
		NodeLiteral(const string& str)
			: node(NoContainer, str)
		{}
		NodeLiteral(const string& str, string val)
			: node(NoContainer, str, std::move(val))
		{}
		template<typename _tLiteral = NodeLiteral>
		NodeLiteral(const string& str, std::initializer_list<_tLiteral> il)
			: node(NoContainer, str, NodeSequence(il.begin(), il.end()))
		{}
		template<typename _tLiteral = NodeLiteral, class _tString,
			typename... _tParams>
			NodeLiteral(ListContainerTag, _tString&& str,
				std::initializer_list<_tLiteral> il, _tParams&&... args)
			: node(ValueNode::Container(il.begin(), il.end()), wforward(str),
				wforward(args)...)
		{}
		DefDeCopyMoveCtorAssignment(NodeLiteral)

			DefCvt(wnothrow, ValueNode&, node)
			DefCvt(const wnothrow, const ValueNode&, node)
	};

	/*!
	\brief 传递参数构造值类型节点字面量。
	\relates NodeLiteral
	*/
	template<typename _tString, typename _tLiteral = NodeLiteral>
	inline NodeLiteral
		AsNodeLiteral(_tString&& name, std::initializer_list<_tLiteral> il)
	{
		return{ ListContainer, wforward(name), il };
	}
}

#endif