/*! \file ValueNode.h
\ingroup WFrameWork/Core
\brief ֵ���ͽڵ㡣
\par �޸�ʱ��:
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
		\brief �ӽڵ�������
		*/
		Container container{};

	public:
		ValueObject Value{};

		DefDeCtor(ValueNode)
			/*!
			\brief ���죺ʹ����������
			*/
			ValueNode(Container con)
			: container(std::move(con))
		{}
		/*!
		\brief ���죺ʹ���ַ������ú�ֵ���Ͷ����������
		\note ��ʹ��������
		*/
		template<typename _tString, typename... _tParams>
		inline
			ValueNode(NoContainerTag, _tString&& str, _tParams&&... args)
			: name(wforward(str)), Value(wforward(args)...)
		{}
		/*!
		\brief ���죺ʹ�����������ַ������ú�ֵ���Ͷ����������
		*/
		template<typename _tString, typename... _tParams>
		ValueNode(Container con, _tString&& str, _tParams&&... args)
			: name(wforward(str)), container(std::move(con)),
			Value(wforward(args)...)
		{}
		/*!
		\brief ���죺ʹ������������ԡ�
		*/
		template<typename _tIn>
		inline
			ValueNode(const pair<_tIn, _tIn>& pr)
			: container(pr.first, pr.second)
		{}
		/*!
		\brief ���죺ʹ������������ԡ��ַ������ú�ֵ������
		*/
		template<typename _tIn, typename _tString>
		inline
			ValueNode(const pair<_tIn, _tIn>& pr, _tString&& str)
			: name(wforward(str)), container(pr.first, pr.second)
		{}
		/*!
		\brief ԭ�ع��죺ʹ�����������ƺ�ֵ�Ĳ���Ԫ�顣
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
			\brief ��һ��ֵ��ʹ��ֵ�����ͽ����������и��ƻ�ת�Ƹ�ֵ��
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
			\brief �滻ͬ���ӽڵ㡣
			\return �������á�
			*/
			//@{
			PDefHOp(ValueNode&, /=, const ValueNode& node)
			ImplRet(*this %= node, *this)
			PDefHOp(ValueNode&, /=, ValueNode&& node)
			ImplRet(*this %= std::move(node), *this)
			//@}
			/*!
			\brief �滻ͬ���ӽڵ㡣
			\return �ӽڵ����á�
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
		\brief ȡ�ӽڵ��������á�
		*/
		DefGetter(const wnothrow, const Container&, Container, container)
			/*!
			\brief ȡ�ӽڵ��������á�
			*/
			DefGetter(wnothrow, Container&, ContainerRef, container)
			DefGetter(const wnothrow, const string&, Name, name)

			//@{
			//! \brief �����ӽڵ��������ݡ�
			PDefH(void, SetChildren, const Container& con)
			ImplExpr(container = con)
			PDefH(void, SetChildren, Container&& con)
			ImplExpr(container = std::move(con))
			PDefH(void, SetChildren, ValueNode&& node)
			ImplExpr(container = std::move(node.container))
			/*!
			\note �����ӽڵ�������ֵ�����ݡ�
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

		//! \note �����������޸�ֵ�Ĳ���֮���˳��δָ����
		//@{
		/*!
		\brief ����ڵ㡣
		\post <tt>!Value && empty()</tt> ��
		*/
		PDefH(void, Clear, ) wnothrow
			ImplExpr(Value.Clear(), ClearContainer())

		/*!
		\brief �������������ֵ��
		*/
		PDefH(void, ClearTo, ValueObject vo) wnothrow
		ImplExpr(ClearContainer(), Value = std::move(vo))
		//@}

		/*!
		\brief ����ڵ�������
		\post \c empty() ��
		*/
		PDefH(void, ClearContainer, ) wnothrow
		ImplExpr(container.clear())
		//@}

		/*!
		\brief �ݹ鴴������������
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
		\brief ��ָ�������ӽڵ㲻������ָ��ֵ��ʼ����
		\return ��ָ�����Ʋ��ҵ�ָ�����͵��ӽڵ��ֵ�����á�
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
		\brief ���������������ӽڵ㡣
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
		//! \brief ת�������������ӽڵ㡣
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

		//! \warning ���������֮�������Ȩ������ѭ������״̬��������δ������Ϊ��
		//@{
		//! \brief ����������
		PDefH(void, SwapContainer, ValueNode& node) wnothrow
			ImplExpr(container.swap(node.container))

			//! \brief ����������ֵ��
			void
			SwapContent(ValueNode&) wnothrow;
		//@}
		//@}

		/*!
		\brief �׳�����Խ���쳣��
		\throw std::out_of_range ����Խ�硣
		*/
		WB_NORETURN static void
			ThrowIndexOutOfRange();

		/*!
		\brief �׳����ƴ����쳣��
		\throw std::out_of_range ���ƴ���
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
			\brief ������
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
	\brief ���ʽڵ��ָ�����Ͷ���
	\exception std::bad_cast ��ʵ�������ͼ��ʧ�� ��
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
	//! \brief ���ʽڵ��ָ�����Ͷ���ָ�롣
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
	//! \brief ���ʽڵ��ָ�����Ͷ���ָ�롣
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
	//! \brief ȡָ������ָ�Ƶ�ֵ��
	WF_API ValueObject
		GetValueOf(observer_ptr<const ValueNode>);

	//! \brief ȡָ������ָ�Ƶ�ֵ��ָ�롣
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
	\brief ���ʽڵ㡣
	\throw std::out_of_range δ�ҵ���Ӧ�ڵ㡣
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
	\note ʱ�临�Ӷ� O(n) ��
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
	//! \note ʹ�� ADL \c AccessNode ��
	template<class _tNode, typename _tIn>
	_tNode&&
		AccessNode(_tNode&& node, _tIn first, _tIn last)
	{
		return std::accumulate(first, last, ref(node),
			[](_tNode&& nd, decltype(*first) c) {
			return ref(AccessNode(nd, c));
		});
	}
	//! \note ʹ�� ADL \c begin �� \c end ָ����Χ��������
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

	//! \brief ���ʽڵ�ָ�롣
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
	\note ʱ�临�Ӷ� O(n) ��
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
	//! \note ʹ�� ADL \c AccessNodePtr ��
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
	//! \note ʹ�� ADL \c begin �� \c end ָ����Χ��������
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
	\exception std::bad_cast ��ʵ�������ͼ��ʧ�� ��
	\relates ValueNode
	*/
	//@{
	/*!
	\brief �����ӽڵ��ָ�����Ͷ���
	\note ʹ�� ADL \c AccessNode ��
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

	//! \brief ����ָ�����Ƶ��ӽڵ��ָ�����Ͷ����ָ�롣
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


	//! \note ��������ӽڵ㡣
	//@{
	inline PDefH(const ValueNode&, AsNode, const ValueNode& node)
		ImplRet(node)
		/*!
		\brief ����ָ�����ƺ�ֵ��������ֵ���ͽڵ㡣
		*/
		template<typename _tString, typename... _tParams>
	inline ValueNode
		AsNode(_tString&& str, _tParams&&... args)
	{
		return{ NoContainer, wforward(str), std::forward<_tParams>(args)... };
	}

	/*!
	\brief ����ָ�����ƺ��˻�ֵ��������ֵ���ͽڵ㡣
	*/
	template<typename _tString, typename... _tParams>
	inline ValueNode
		MakeNode(_tString&& str, _tParams&&... args)
	{
		return{ NoContainer, wforward(str), white::decay_copy(args)... };
	}
	//@}

	/*!
	\brief ȡָ�����ƺ�ת��Ϊ�ַ�����ֵ���ͽڵ㡣
	\note ʹ�÷��޶� to_string ת����
	*/
	template<typename _tString, typename... _tParams>
	inline ValueNode
		StringifyToNode(_tString&& str, _tParams&&... args)
	{
		return{ NoContainer, wforward(str), to_string(wforward(args)...) };
	}

	/*!
	\brief �����ò���ȡֵ���ͽڵ㣺��������
	*/
	//@{
	inline PDefH(const ValueNode&, UnpackToNode, const ValueNode& arg)
		ImplRet(arg)
		inline PDefH(ValueNode&&, UnpackToNode, ValueNode&& arg)
		ImplRet(std::move(arg))
		//@}


		/*!
		\brief �Ӳ���ȡ��ָ������Ϊ��ʼ��������ֵ���ͽڵ㡣
		\note ȡ����ͬ std::get ����ʹ�� ADL ����ȡǰ����������
		*/
		template<class _tPack>
	inline ValueNode
		UnpackToNode(_tPack&& pk)
	{
		return{ 0, get<0>(wforward(pk)),
			ValueObject(white::decay_copy(get<1>(wforward(pk)))) };
	}

	/*!
	\brief ȡָ��ֵ���ͽڵ�Ϊ��Ա�Ľڵ�������
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
	\brief ȡ��ָ������Ϊ������Ӧ��ʼ���õ���ֵ���ͽڵ�Ϊ�ӽڵ��ֵ���ͽڵ㡣
	*/
	template<typename _tString, typename... _tParams>
	inline ValueNode
		PackNodes(_tString&& name, _tParams&&... args)
	{
		return{ CollectNodes(UnpackToNode(wforward(args))...), wforward(name) };
	}


	//@{
	//! \brief �Ƴ����ӽڵ㡣
	WF_API void
		RemoveEmptyChildren(ValueNode::Container&) wnothrow;

	/*!
	\brief �Ƴ��׸��ӽڵ㡣
	\pre ���ԣ��ڵ�ǿա�
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
	\brief �ж��ַ����Ƿ���һ��ָ���ַ��ͷǸ���������ϡ�
	\pre ���ԣ��ַ�������������ָ��ǿա�
	\note �������ܱ� <tt>unsigned long</tt> ��ʾ��������
	*/
	WF_API bool
	IsPrefixedIndex(string_view, char = '$');

	/*!
	\brief ת���ڵ��СΪ�µĽڵ�����ֵ��
	\return ��֤ 4 λʮ�������ڰ��ֵ���������ַ�����
	\throw std::invalid_argument ����ֵ���󣬲����� 4 λʮ��������ʾ��
	\note �ظ�ʹ����Ϊ�½ڵ�����ƣ������ڲ��벻�ظ��ڵ㡣
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
	\brief ȡ���һ���ӽڵ����Ƶ�ǰ׺������
	\return ���������ӽڵ���Ϊ \c size_t(-1) ������Ϊ���һ���ӽڵ�����ƶ�Ӧ��������
	\throw std::invalid_argument �����ӽڵ㵫���Ʋ���ǰ׺������
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
	\brief �ڵ�����������

	��������������� std::vector ��ͬ��Ҫ���ģ���һ��ʵ����Ԫ��Ϊ ValueNode ���͡�
	*/
	using NodeSequence = wimpl(white::vector)<ValueNode>;


	/*!
	\brief ��װ�ڵ�������������
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
	\brief ���ݲ�������ֵ���ͽڵ���������
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