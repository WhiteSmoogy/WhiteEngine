/*\par 修改时间 :
	2017-03-27 15:14 +0800
*/

#ifndef WScheme_LSchemeA_H
#define WScheme_LSchemeA_H 1


#include <WBase/utility.hpp> // for white::derived_entity
#include <WBase/any.h> // for white::any

#include <WFrameWork/Core/WEvent.hpp> //for white::indirect, white::fast_any_of,
//	white::GHEvent, white::GEvent, white::GCombinerInvoker,
//	white::GDefaultLastValueInvoker;

#include "SContext.h"  // for WTag, ValueNode, TermNode, string,
//	LoggedEvent, white::isdigit, white::equality_comparable, white::lref,
//	shared_ptr, white::type_info, weak_ptr, white::get_equal_to,
//	white::exchange;

#include <algorithm>


namespace scheme {

	/*!	\defgroup ThunkType Thunk Types
	\brief 中间值类型。

	标记特定求值策略，储存于 TermNode 的 Value 数据成员中不直接表示宿主语言对象的类型。
	*/


	using white::pair;
	using white::to_string;
	using white::MakeIndex;
	using white::NodeSequence;
	using white::NodeLiteral;

	using white::enable_shared_from_this;
	using white::make_shared;
	using white::make_weak;
	using white::observer_ptr;
	using  white::pair;
	using white::to_string;
	using white::shared_ptr;
	using white::weak_ptr;
	/*!
	\brief LA 元标签。
	\note LA 是 LScheme 的抽象实现。
	*/
	struct WS_API WSLATag : WTag
	{};


	/*!
	\brief 插入语法节点。

	在指定的节点插入以节点大小字符串为名称的节点，可用于语法分析树。
	*/
	//@{
	template<class _tNodeOrCon, typename... _tParams>
	ValueNode::iterator
		InsertSyntaxNode(_tNodeOrCon&& node_or_con,
			std::initializer_list<ValueNode> il, _tParams&&... args)
	{
		return node_or_con.emplace_hint(node_or_con.end(), ValueNode::Container(il),
			MakeIndex(node_or_con), wforward(args)...);
	}
	template<class _tNodeOrCon, typename _type, typename... _tParams>
	ValueNode::iterator
		InsertSyntaxNode(_tNodeOrCon&& node_or_con, _type&& arg, _tParams&&... args)
	{
		return node_or_con.emplace_hint(node_or_con.end(), wforward(arg),
			MakeIndex(node_or_con), wforward(args)...);
	}
	//@}


	/*!
	\brief 节点映射操作类型：变换节点为另一个节点。
	*/
	using NodeMapper = std::function<ValueNode(const TermNode&)>;

	//@{
	using NodeToString = std::function<string(const ValueNode&)>;

	template<class _tCon>
	using GNodeInserter = std::function<void(TermNode&&, _tCon&)>;

	using NodeInserter = GNodeInserter<TermNode::Container&>;

	using NodeSequenceInserter = GNodeInserter<NodeSequence>;
	//@}


	//! \return 创建的新节点。
	//@{
	/*!
	\brief 映射 WSLA 叶节点。
	\sa ParseWSLANodeString

	创建新节点。若参数为空则返回值为空串的新节点；否则值以 ParseWSLANodeString 取得。
	*/
	WS_API ValueNode
		MapWSLALeafNode(const TermNode&);

	/*!
	\brief 变换节点为语法分析树叶节点。
	\note 可选参数指定结果名称。
	*/
	WS_API ValueNode
		TransformToSyntaxNode(const ValueNode&, const string& = {});
	//@}

	/*!
	\brief 转义 WSLA 节点字面量。
	\return 调用 EscapeLiteral 转义访问字符串的结果。
	\exception white::bad_any_cast 异常中立：由 Access 抛出。
	*/
	WS_API string
		EscapeNodeLiteral(const ValueNode&);

	/*!
	\brief 转义 WSLA 节点字面量。
	\return 参数为空节点则空串，否则调用 Literalize 字面量化 EscapeNodeLiteral 的结果。
	\exception white::bad_any_cast 异常中立：由 EscapeNodeLiteral 抛出。
	\sa EscapeNodeLiteral
	*/
	WS_API string
		LiteralizeEscapeNodeLiteral(const ValueNode&);

	/*!
	\brief 解析 WSLA 节点字符串。

	以 string 类型访问节点，若失败则结果为空串。
	*/
	WS_API string
		ParseWSLANodeString(const ValueNode&);


	/*!
	\brief 插入语法子节点。

	在指定的节点插入以节点大小字符串为名称的节点，可用于语法分析树。
	*/
	//@{
	template<class _tNodeOrCon>
	ValueNode::iterator
		InsertChildSyntaxNode(_tNodeOrCon&& node_or_con, ValueNode& child)
	{
		return InsertSyntaxNode(wforward(node_or_con), child.GetContainerRef());
	}
	template<class _tNodeOrCon>
	ValueNode::iterator
		InsertChildSyntaxNode(_tNodeOrCon&& node_or_con, ValueNode&& child)
	{
		return InsertSyntaxNode(wforward(node_or_con),
			std::move(child.GetContainerRef()));
	}
	template<class _tNodeOrCon>
	ValueNode::iterator
		InsertChildSyntaxNode(_tNodeOrCon&& node_or_con, const NodeLiteral& nl)
	{
		return
			InsertChildSyntaxNode(wforward(node_or_con), TransformToSyntaxNode(nl));
	}
	//@}


	//! \brief 生成前缀缩进的函数类型。
	using IndentGenerator = std::function<string(size_t)>;

	//! \brief 生成水平制表符为单位的缩进。
	WS_API string
		DefaultGenerateIndent(size_t);

	/*!
	\brief 打印缩进。
	\note 若最后一个参数等于零则无副作用。
	*/
	WS_API void
		PrintIndent(std::ostream&, IndentGenerator = DefaultGenerateIndent, size_t = 1);

	//@{
	/*!
	\brief 遍历子节点。
	\note 使用 ADL AccessPtr 。

	遍历节点容器中的子节点。
	首先调用 AccessPtr 尝试访问 NodeSequence ，否则直接作为节点容器访问。
	*/
	template<typename _fCallable, class _type>
	void
		TraverseSubnodes(_fCallable f, const _type& node)
	{
		using white::AccessPtr;

		// TODO: Null coalescing or variant value?
		if (const auto p = AccessPtr<NodeSequence>(node))
			for (const auto& nd : *p)
				white::invoke(f, nd);
		else
			for (const auto& nd : node)
				white::invoke(f, nd);
	}

	//! \brief 打印容器边界和其中的 NPLA 节点，且在打印边界前调用前置操作。
	template<typename _fCallable>
	void
		PrintContainedNodes(std::ostream& os, std::function<void()> pre, _fCallable f)
	{
		pre();
		os << '(' << '\n';
		TryExpr(white::invoke(f))
			CatchIgnore(std::out_of_range&)
			pre();
		os << ')' << '\n';
	}

	/*!
	\brief 打印有索引前缀的节点或遍历子节点并打印。
	\note 使用 ADL IsPrefixedIndex 。
	\sa IsPrefixedIndex
	\sa TraverseSubnodes

	以第三参数作为边界前置操作，调用 PrintContainedNodes 逐个打印子节点内容。
	调用第四参数输出最后一个参数决定的缩进作为前缀，然后打印子节点内容。
	对满足 IsPrefixedIndex 的节点调用第四参数作为节点字符串打印；
	否则，调用第五参数递归打印子节点，忽略此过程中的 std::out_of_range 异常。
	其中，遍历子节点通过调用 TraverseSubnodes 实现。
	*/
	template<typename _fCallable, typename _fCallable2>
	void
		TraverseNodeChildAndPrint(std::ostream& os, const ValueNode& node,
			std::function<void()> pre, _fCallable print_node_str,
			_fCallable2 print_term_node)
	{
		using white::IsPrefixedIndex;

		TraverseSubnodes([&](const ValueNode& nd) {
			if (IsPrefixedIndex(nd.GetName()))
			{
				pre();
				white::invoke(print_node_str, nd);
			}
			else
				PrintContainedNodes(os, pre, [&] {
				white::invoke(print_term_node, nd);
			});
		}, node);
	}
	//@}

	/*!
	\brief 打印 WSLA 节点。
	\sa PrintIdent
	\sa PrintNodeChild
	\sa PrintNodeString

	调用第四参数输出最后一个参数决定的缩进作为前缀和一个空格，然后打印节点内容：
	先尝试调用 PrintNodeString 打印节点字符串，若成功直接返回；
	否则打印换行，然后尝试调用 PrintNodeChild 打印 NodeSequence ；
	再次失败则调用 PrintNodeChild 打印子节点。
	调用 PrintNodeChild 打印后输出回车。
	*/
	WS_API void
		PrintNode(std::ostream&, const ValueNode&, NodeToString = EscapeNodeLiteral,
			IndentGenerator = DefaultGenerateIndent, size_t = 0);

	/*!
	\brief 打印作为子节点的 WSLA 节点。
	\sa IsPrefixedIndex
	\sa PrintIdent
	\sa PrintNodeString

	调用第四参数输出最后一个参数决定的缩进作为前缀，然后打印子节点内容。
	对满足 IsPrefixedIndex 的节点调用 PrintNodeString 作为节点字符串打印；
	否则，调用 PrintNode 递归打印子节点，忽略此过程中的 std::out_of_range 异常。
	*/
	WS_API void
		PrintNodeChild(std::ostream&, const ValueNode&, NodeToString
			= EscapeNodeLiteral, IndentGenerator = DefaultGenerateIndent, size_t = 0);

	/*!
	\brief 打印节点字符串。
	\return 是否成功访问节点字符串并输出。
	\note white::bad_any_cast 外异常中立。
	\sa PrintNode

	使用最后一个参数指定的访问节点，打印得到的字符串和换行符。
	忽略 white::bad_any_cast 。
	*/
	WS_API bool
		PrintNodeString(std::ostream&, const ValueNode&,
			NodeToString = EscapeNodeLiteral);


	namespace sxml
	{

		//@{
		/*!
		\brief 转换 sxml 节点为 XML 属性字符串。
		\throw LoggedEvent 没有子节点。
		\note 当前不支持 annotation ，在超过 2 个子节点时使用 TraceDe 警告。
		*/
		WS_API string
			ConvertAttributeNodeString(const TermNode&);

		/*!
		\brief 转换 sxml 文档节点为 XML 字符串。
		\throw LoggedEvent 不符合最后一个参数约定的内容被解析。
		\throw white::unimplemented 指定 ParseOption::Strict 时解析未实现内容。
		\sa ConvertStringNode
		\see http://okmij.org/ftp/Scheme/sxml.html#Annotations 。
		\todo 支持 *ENTITY* 和 *NAMESPACES* 标签。

		转换 sxml 文档节点为 XML 。
		尝试使用 ConvertStringNode 转换字符串节点，若失败作为非叶子节点递归转换。
		因为当前 sxml 规范未指定注解(annotation) ，所以直接忽略。
		*/
		WS_API string
			ConvertDocumentNode(const TermNode&, IndentGenerator = DefaultGenerateIndent,
				size_t = 0, ParseOption = ParseOption::Normal);

		/*!
		\brief 转换 sxml 节点为被转义的 XML 字符串。
		\sa EscapeXML
		*/
		WS_API string
			ConvertStringNode(const TermNode&);

		/*!
		\brief 打印 SContext::Analyze 分析取得的 sxml 语法树节点并刷新流。
		\see ConvertDocumentNode
		\see SContext::Analyze
		\see Session

		参数节点中取第一个节点作为 sxml 文档节点调用 ConvertStringNode 输出并刷新流。
		*/
		WS_API void
			PrintSyntaxNode(std::ostream& os, const TermNode&,
				IndentGenerator = DefaultGenerateIndent, size_t = 0);
		//@}


		//@{
		//! \brief 构造 sxml 文档顶级节点。
		//@{
		template<typename... _tParams>
		ValueNode
			MakeTop(const string& name, _tParams&&... args)
		{
			return white::AsNodeLiteral(name,
				{ { MakeIndex(0), "*TOP*" }, NodeLiteral(wforward(args))... });
		}
		inline PDefH(ValueNode, MakeTop, )
			ImplRet(MakeTop({}))
			//@}

			/*!
			\brief 构造 sxml 文档 XML 声明节点。
			\note 第一个参数指定节点名称，其余参数指定节点中 XML 声明的值：版本、编码和独立性。
			\note 最后两个参数可选为空值，此时结果不包括对应的属性。
			\warning 不对参数合规性进行检查。
			*/
			WS_API ValueNode
			MakeXMLDecl(const string& = {}, const string& = "1.0",
				const string& = "UTF-8", const string& = {});

		/*!
		\brief 构造包含 XML 声明的 sxml 文档节点。
		\sa MakeTop
		\sa MakeXMLDecl
		*/
		WS_API ValueNode
			MakeXMLDoc(const string& = {}, const string& = "1.0",
				const string& = "UTF-8", const string& = {});

		//! \brief 构造 sxml 属性标记字面量。
		//@{
		inline PDefH(NodeLiteral, MakeAttributeTagLiteral,
			std::initializer_list<NodeLiteral> il)
			ImplRet({ "@", il })
			template<typename... _tParams>
		NodeLiteral
			MakeAttributeTagLiteral(_tParams&&... args)
		{
			return sxml::MakeAttributeTagLiteral({ NodeLiteral(wforward(args)...) });
		}
		//@}

		//! \brief 构造 XML 属性字面量。
		//@{
		inline PDefH(NodeLiteral, MakeAttributeLiteral, const string& name,
			std::initializer_list<NodeLiteral> il)
			ImplRet({ name,{ MakeAttributeTagLiteral(il) } })
			template<typename... _tParams>
		NodeLiteral
			MakeAttributeLiteral(const string& name, _tParams&&... args)
		{
			return{ name,{ sxml::MakeAttributeTagLiteral(wforward(args)...) } };
		}
		//@}

		//! \brief 插入只有 XML 属性节点到语法节点。
		//@{
		template<class _tNodeOrCon>
		inline void
			InsertAttributeNode(_tNodeOrCon&& node_or_con, const string& name,
				std::initializer_list<NodeLiteral> il)
		{
			InsertChildSyntaxNode(node_or_con, MakeAttributeLiteral(name, il));
		}
		template<class _tNodeOrCon, typename... _tParams>
		inline void
			InsertAttributeNode(_tNodeOrCon&& node_or_con, const string& name,
				_tParams&&... args)
		{
			InsertChildSyntaxNode(node_or_con,
				sxml::MakeAttributeLiteral(name, wforward(args)...));
		}
		//@}
		//@}

	} // namespace sxml;
	  //@}


	  //@{
	  //! \ingroup exceptions
	  //@{
	  //! \brief WSL 异常基类。
	class WS_API WSLException : public LoggedEvent
	{
	public:
		WB_NONNULL(2)
			WSLException(const char* str, white::RecordLevel lv = white::Err)
			: LoggedEvent(str, lv)
		{}
		WSLException(const white::string_view sv,
			white::RecordLevel lv = white::Err)
			: LoggedEvent(sv, lv)
		{}
		DefDeCtor(WSLException)

			//! \brief 虚析构：类定义外默认实现。
			virtual	~WSLException() override;
	};


	/*!
	\brief 列表规约失败。
	\todo 捕获并保存上下文信息。
	*/
	class WS_API ListReductionFailure : public WSLException
	{
	public:
		using WSLException::WSLException;
		DefDeCtor(ListReductionFailure)

			//! \brief 虚析构：类定义外默认实现。
			~ListReductionFailure() override;
	};


	//! \brief 语法错误。
	class WS_API InvalidSyntax : public WSLException
	{
	public:
		using WSLException::WSLException;
		DefDeCtor(InvalidSyntax)

			//! \brief 虚析构：类定义外默认实现。
			~InvalidSyntax() override;
	};

	class WS_API ParameterMismatch : public InvalidSyntax
	{
	public:
		using InvalidSyntax::InvalidSyntax;
		DefDeCtor(ParameterMismatch)

			//! \brief 虚析构：类定义外默认实现。
			~ParameterMismatch() override;
	};

	/*!
	\brief 元数不匹配错误。
	\todo 支持范围匹配。
	*/
	class WS_API ArityMismatch : public WSLException
	{
	private:
		size_t expected;
		size_t received;

	public:
		DefDeCtor(ArityMismatch)
			/*!
			\note 前两个参数表示期望和实际的元数。
			*/
			ArityMismatch(size_t, size_t, white::RecordLevel = white::Err);

		//! \brief 虚析构：类定义外默认实现。
		~ArityMismatch() override;

		DefGetter(const wnothrow, size_t, Expected, expected)
			DefGetter(const wnothrow, size_t, Received, received)
	};

	/*!
	\brief 标识符错误。
	*/
	class WS_API BadIdentifier : public WSLException
	{
	private:
		white::shared_ptr<string> p_identifier;

	public:
		/*!
		\brief 构造：使用作为标识符的字符串、已知实例数和和记录等级。
		\pre 间接断言：第一参数的数据指针非空。

		不检查第一参数内容作为标识符的合法性，直接视为待处理的标识符，
		初始化表示标识符错误的异常对象。
		实例数等于 0 时表示未知标识符；
		实例数等于 1 时表示非法标识符；
		实例数大于 1 时表示重复标识符。
		*/
		WB_NONNULL(2)
			BadIdentifier(const char*, size_t = 0, white::RecordLevel = white::Err);
		BadIdentifier(string_view, size_t = 0, white::RecordLevel = white::Err);
		DefDeCtor(BadIdentifier)

			//! \brief 虚析构：类定义外默认实现。
			~BadIdentifier() override;

		DefGetter(const wnothrow, const string&, Identifier,
			white::Deref(p_identifier))
	};

	//@}
	//@}

	/*!
	\brief 无效引用错误。
	*/
	class WS_API InvalidReference : public WSLException
	{
	public:
		using WSLException::WSLException;
		DefDeCtor(InvalidReference)

			//! \brief 虚析构：类定义外默认实现。
			~InvalidReference() override;
	};
	//@}



		//@{
		//! \brief 字面量类别。
	enum class LexemeCategory
	{
		//! \brief 无：非字面量。
		Symbol,
		//! \brief 代码字面量。
		Code,
		//! \brief 数据字面量。
		Data,
		/*!
		\brief 扩展字面量：由派生实现定义的其它字面量。
		*/
		Extended
	};



	//! \sa LexemeCategory
	//@{
	/*!
	\pre 断言：字符串参数的数据指针非空且字符串非空。
	\return 判断的非扩展字面量分类。
	*/
	//@{
	/*!
	\brief 对排除扩展字面量的词素分类。
	\note 扩展字面量视为非字面量。
	*/
	WS_API LexemeCategory
		CategorizeBasicLexeme(string_view) wnothrowv;

	/*!
	\brief 对词素分类。
	\sa CategorizeBasicLexeme
	*/
	WS_API LexemeCategory
		CategorizeLexeme(string_view) wnothrowv;
	//@}

	/*!
	\brief 判断不是非扩展字面量的词素是否为 WSLA 扩展字面量。
	\pre 断言：字符串参数的数据指针非空且字符串非空。
	\pre 词素不是代码字面量或数据字面量。
	*/
	WS_API bool
		IsWSLAExtendedLiteral(string_view) wnothrowv;

	/*!
	\brief 判断字符是否为 WSLA 扩展字面量非数字前缀。
	*/
	wconstfn PDefH(bool, IsWSLAExtendedLiteralNonDigitPrefix, char c) wnothrow
		ImplRet(c == '#' || c == '+' || c == '-')

		//! \brief 判断字符是否为 WSLA 扩展字面量前缀。
		inline PDefH(bool, IsWSLAExtendedLiteralPrefix, char c) wnothrow
		ImplRet(std::isdigit(c) || IsWSLAExtendedLiteralNonDigitPrefix(c))

		/*!
		\brief 判断词素是否为 WSLA 符号。
		\pre 间接断言：字符串参数的数据指针非空且字符串非空。
		*/
		inline PDefH(bool, IsWSLASymbol, string_view id) wnothrowv
		ImplRet(CategorizeLexeme(id) == LexemeCategory::Symbol)
		//@}
		//@}


		/*!
		\ingroup ThunkType
		\brief 记号值。
		\note 和被求值的字符串不同的包装类型。
		\warning 非空析构。
		*/
		using TokenValue = white::derived_entity<string, WSLATag>;


	/*!
	\brief 访问项的值作为记号。
	\return 通过访问项的值取得的记号的指针，或空指针表示无法取得名称。
	*/
	WS_API observer_ptr<const TokenValue>
		TermToNamePtr(const TermNode&);

	/*!
	\brief 访问项的值并转换为字符串形式的外部表示。
	\return 转换得到的字符串。
	\sa TermToNamePtr

	访问项的值作为名称转换为字符串，若失败则提取值的类型和子项数作为构成值的表示。
	除名称外的外部表示方法未指定；结果可能随实现变化。
	*/
	WS_API string
		TermToString(const TermNode&);

	/*!
	\brief 标记记号节点：递归变换节点，转换其中的词素为记号值。
	\note 先变换子节点。
	*/
	WS_API void
		TokenizeTerm(TermNode& term);

	/*!
	\brief 规约状态：某个项上的一遍规约可能的中间结果。
	*/
	enum class ReductionStatus : wimpl(size_t)
	{
		//@{
		//! \brief 规约成功终止且不需要保留子项。
		Clean = 0,
			//! \brief 规约成功但需要保留子项。
			Retained,
			//! \brief 需要重规约。
			Retrying
			//@}
	};


	/*!
	\ingroup ThunkType
	\brief 延迟求值项。
	\note 和被延迟求值的项及其它节点是不同的包装类型。
	\warning 非空析构。

	直接作为项的值对象包装被延迟求值的项。
	*/
	using DelayedTerm = white::derived_entity<TermNode, WSLATag>;

	//@{
	/*!
	\brief 访问项并取解析 TermReference 间接值后的引用。
	\return 若项的 Value 数据成员为 TermReference 则为其中的引用，否则为参数。
	\sa TermReference
	*/
	WS_API TermNode&
		ReferenceTerm(TermNode&);
	WS_API const TermNode&
		ReferenceTerm(const TermNode&);

	/*!
	\brief 访问解析 TermReference 后的项的指定类型对象。
	\exception std::bad_cast 异常中立：空实例或类型检查失败。
	*/
	template<typename _type>
	inline _type&
		AccessTerm(ValueNode& term)
	{
		return ReferenceTerm(term).Value.Access<_type>();
	}
	template<typename _type>
	inline const _type&
		AccessTerm(const ValueNode& term)
	{
		return ReferenceTerm(term).Value.Access<_type>();
	}

	//! \brief 访问解析 TermReference 后的项的指定类型对象指针。
	//@{
	template<typename _type>
	inline observer_ptr<_type>
		AccessTermPtr(TermNode& term) wnothrow
	{
		return ReferenceTerm(term).Value.AccessPtr<_type>();
	}
	template<typename _type>
	inline observer_ptr<const _type>
		AccessTermPtr(const TermNode& term) wnothrow
	{
		return ReferenceTerm(term).Value.AccessPtr<_type>();
	}
	//@}
	//@}


	//@{
	//! \brief 引用项操作。
	struct ReferenceTermOp
	{
		template<typename _type>
		auto
			operator()(_type&& term) const -> decltype(ReferenceTerm(wforward(term)))
		{
			return ReferenceTerm(wforward(term));
		}
	};

	/*!
	\relates ReferenceTermOp
	*/
	template<typename _func>
	auto
		ComposeReferencedTermOp(_func f)
		->wimpl(decltype(white::compose_n(f, ReferenceTermOp())))
	{
		return white::compose_n(f, ReferenceTermOp());
	}
	//@}


	/*!
	\brief 检查视为范式的节点并提取规约状态。
	*/
	inline PDefH(ReductionStatus, CheckNorm, const TermNode& term) wnothrow
		ImplRet(IsBranch(term) ? ReductionStatus::Retained : ReductionStatus::Clean)

	/*!
	\brief 根据规约状态检查是否可继续规约。
	\see TraceDe

	只根据输入状态确定结果。当且仅当规约成功时不视为继续规约。
	若发现不支持的状态视为不成功，输出警告。
	不直接和 ReductionStatus::Retrying 比较以分离依赖 ReductionStatus 的具体值的实现。
	派生实现可使用类似的接口指定多个不同的状态。
	*/
	WB_PURE WS_API bool
		CheckReducible(ReductionStatus);

	/*!
	\sa CheckReducible
	*/
	template<typename _func, typename... _tParams>
	void
		CheckedReduceWith(_func f, _tParams&&... args)
	{
		white::retry_on_cond(CheckReducible, f, wforward(args)...);
	}

	//! \brief 使用第二个参数指定的项的内容替换第一个项的内容。
	//@{
	inline PDefH(void, LiftTerm, TermNode& term, TermNode& tm)
		ImplExpr(term.SetContent(std::move(tm)))

	inline PDefH(void, LiftTerm, ValueObject& term_v, ValueObject& vo)
		ImplExpr(term_v = std::move(vo))
	inline PDefH(void, LiftTerm, TermNode& term, ValueObject& vo)
		ImplExpr(LiftTerm(term.Value, vo))
	//@}

	/*!
	\brief 提升项间接引用：复制或转移间接引用项使项不含间接值。
	*/
	inline PDefH(void, LiftTermIndirection, TermNode& term, const TermNode& tm)
		// NOTE: See $2018-02 @ %Documentation::Workflow::Annual2018.
		ImplExpr(white::SetContentWith(term, tm, &ValueObject::MakeMoveCopy))

	/*!
	\warning 引入的间接值无所有权，应注意在生存期内使用以保证内存安全。
	\todo 支持消亡值和复制。
	*/
	//@{
	/*!
	\note 用于支持实现对象语言中的左值到右值转换。
	\sa LiftTerm
	\sa LiftTermRef
	\sa TermReference
	*/
	//@{
	/*!
	\brief 提升项或创建引用项。

	项的 Value 数据成员为 TermReference 类型的值时调用 LiftTermRef ；
	否则，同 LiftTerm 。
	*/
	WS_API void
	LiftTermOrRef(TermNode&, TermNode&);

	/*!
	\brief 提升自身引用项。
	\sa LiftTermOrRef

	作用同以相同参数调用 LiftTermOrRef 。
	仅可能调用 LiftTermRef ，不调用 LiftTerm 以节约不必要的开销。
	*/
	WS_API void
		LiftTermRefToSelf(TermNode&);
	//@}

	/*!
	\brief 提升项引用：使用第二个参数指定的项的内容引用替换第一个项的内容。
	\sa ValueObject::MakeIndirect
	*/
	//@{
	inline PDefH(void, LiftTermRef, TermNode& term, const TermNode& tm)
		ImplExpr(white::SetContentWith(term, tm, &ValueObject::MakeIndirect))
	inline PDefH(void, LiftTermRef, ValueObject& term_v, const ValueObject& vo)
		ImplExpr(term_v = vo.MakeIndirect())
	inline PDefH(void, LiftTermRef, TermNode& term, const ValueObject& vo)
		ImplExpr(LiftTermRef(term.Value, vo))
	//@}

	/*!
	\brief 提升项对象为引用。
	\throw WSLException 检查失败：非左值且不具有对象的唯一所有权，不能被外部引用保存。
	\throw white::invalid_construction 参数不持有值。
	\sa LiftTerm
	\todo 使用具体的语义错误异常类型。
	*/
	WS_API void
		LiftToReference(TermNode&, TermNode&);

	/*!
	\brief 递归提升项及其子项或递归创建项和子项对应的包含间接值的引用项到自身。
	\note 先提升项的值再提升子项以确保子项表示引用值时被提升。
	\sa LiftTermRefToSelf
	*/
	WS_API void
		LiftToSelf(TermNode&);

	/*!
	\brief 递归提升项及其子项或递归创建项和子项对应的包含间接值的间接引用项到自身。
	\sa LiftToSelf
	\sa LiftTermIndirection

	调用 LiftToSelf ，然后递归地以相同参数调用 LiftTermIndirection 复制或转移自身。
	*/
	WS_API void
		LiftToSelfSafe(TermNode&);

	/*!
	\brief 递归提升项及其子项或递归创建项和子项对应的包含间接值的引用项到其它项。
	\sa LiftTerm
	\sa LiftToSelf

	以第二参数调用 LiftToSelf 后再调用 LiftTerm 。
	*/
	WS_API void
		LiftToOther(TermNode&, TermNode&);
	//@}

	/*!
	\brief 对每个子项调用 LiftToSelfSafe 。
	\since build 822
	*/
	inline PDefH(void, LiftSubtermsToSelfSafe, TermNode& term)
		ImplExpr(std::for_each(term.begin(), term.end(), LiftToSelfSafe))

	/*!
	\brief 提升延迟项。
	*/
	inline PDefH(void, LiftDelayed, TermNode& term, DelayedTerm& tm)
		ImplExpr(LiftTermRef(term, tm))

	//! \pre 断言：参数指定的项是枝节点。
	//@{
	/*!
	\brief 提升首项：使用首个子项替换项的内容。
	*/
	inline PDefH(void, LiftFirst, TermNode& term)
		ImplExpr(IsBranch(term), LiftTerm(term, Deref(term.begin())))

	/*!
	\brief 提升末项：使用最后一个子项替换项的内容。
	*/
	inline PDefH(void, LiftLast, TermNode& term)
		ImplExpr(IsBranch(term), LiftTerm(term, Deref(term.rbegin())))
	//@}


	/*!
	\pre 间接断言：参数为枝节点。
	\return ReductionStatus::Retained 。
	*/
	//@{
	//! \brief 规约为列表：对枝节点移除第一个子项，保留余下的子项作为列表。
	WS_API ReductionStatus
		ReduceBranchToList(TermNode&) wnothrowv;

	/*!
	\brief 规约为列表值：对枝节点移除第一个子项，保留余下的子项提升后作为列表的值。
	\sa LiftSubtermsToSelfSafe
	*/
	WS_API ReductionStatus
		ReduceBranchToListValue(TermNode&) wnothrowv;
	//@}

	/*!
	\sa RemoveHead
	\note 使用 ADL RemoveHead 。
	*/
	//@{
	/*!
	\brief 规约第一个非结尾空列表子项。
	\return ReductionStatus::Clean 。

	若项具有不少于一个子项且第一个子项是空列表则移除。
	允许空列表作为第一个子项以标记没有操作数的函数应用。
	*/
	WS_API ReductionStatus
		ReduceHeadEmptyList(TermNode&) wnothrow;

	//! \return 移除项时 ReductionStatus::Retained ，否则 ReductionStatus::Clean。
	//@{
	/*!
	\brief 规约为列表：对枝节点移除第一个子项，保留余下的子项作为列表。
	\sa ReduceBranchToList
	*/
	WS_API ReductionStatus
		ReduceToList(TermNode&) wnothrow;

	/*!
	\brief 规约为列表值：对枝节点移除第一个子项，保留余下的子项提升后作为列表的值。
	\sa ReduceBranchToListValue
	*/
	WS_API ReductionStatus
		ReduceToListValue(TermNode&) wnothrow;
	//@}
	//@}


	/*!
	\note 一般第一参数用于指定被合并的之前的规约结果，第二参数指定用于合并的结果。
	\return 合并后的规约结果。
	*/
	/*!
	\brief 合并规约结果。

	若第二参数指定保留项，则合并后的规约结果为第二参数；否则为第一参数。
	*/
	wconstfn PDefH(ReductionStatus, CombineReductionResult, ReductionStatus res,
		ReductionStatus r) wnothrow
		ImplRet(r == ReductionStatus::Retained ? r : res)

	/*!
	\brief 合并序列规约结果。
	\sa CheckReducible

	若第二参数指定可继续规约的项，则合并后的规约结果为第二参数；
	否则同 CombineReductionResult 。
	若可继续规约，则指定当前规约的项的规约已终止，不合并第一参数指定的结果。
	下一轮序列的规约可能使用或忽略合并后的结果。
	*/
	inline PDefH(ReductionStatus, CombineSequenceReductionResult,
			ReductionStatus res, ReductionStatus r) wnothrow
		ImplRet(CheckReducible(r) ? r : CombineReductionResult(res, r))

	//@{
	/*!
	\brief 遍合并器：逐次调用直至成功。
	\note 合并遍结果用于表示及早判断是否应继续规约，可在循环中实现再次规约一个项。
	*/
	struct PassesCombiner
	{
		/*!
		\note 对遍调用异常中立。
		*/
		template<typename _tIn>
		ReductionStatus
			operator()(_tIn first, _tIn last) const
		{
			auto res(ReductionStatus::Clean);

			return white::fast_any_of(first, last, [&](ReductionStatus r) wnothrow{
				if (r == ReductionStatus::Retained)
				res = r;
			return r == ReductionStatus::Retrying;
				}) ? ReductionStatus::Retrying : res;
		}
	};

	class ContextNode;

	/*!
	\note 结果表示判断是否应继续规约。
	\sa PassesCombiner
	*/
	//@{
	//! \brief 一般合并遍。
	template<typename... _tParams>
	using GPasses = white::GEvent<ReductionStatus(_tParams...),
		white::GCombinerInvoker<ReductionStatus, PassesCombiner>>;
	//! \brief 项合并遍。
	using TermPasses = GPasses<TermNode&>;
	//! \brief 求值合并遍。
	using EvaluationPasses = GPasses<TermNode&, ContextNode&>;
	/*!
	\brief 字面量合并遍。
	\pre 字符串参数的数据指针非空。
	*/
	using LiteralPasses = GPasses<TermNode&, ContextNode&, string_view>;
	//@}


	//! \brief 作用域守护类型。
	using Guard = white::any;
	/*!
	\brief 作用域守护遍：用于需在规约例程的入口和出口关联执行的操作。
	\todo 支持迭代使用旧值。
	*/
	using GuardPasses = white::GEvent<Guard(TermNode&, ContextNode&),
		white::GDefaultLastValueInvoker<Guard>>;




	/*!
	\brief 环境列表。

	指定环境对象引用的有序集合。
	*/
	using EnvironmentList = vector<ValueObject>;


	/*!
	\brief 环境。
	\warning 非虚析构。
	\warning 避免 shared_ptr 析构方式不兼容的初始化。
	*/
	class WS_API Environment : private white::equality_comparable<Environment>,
		public enable_shared_from_this<Environment>
	{
	public:
		using BindingMap = ValueNode;
		using NameResolution
			= pair<observer_ptr<ValueNode>, white::lref<const Environment>>;

	private:
		/*!
		\brief 锚对象类型：提供被引用计数。
		*/
		struct SharedAnchor final
		{
			shared_ptr<const void> Ptr{ make_shared<uintptr_t>() };

			DefDeCtor(SharedAnchor)
				SharedAnchor(const SharedAnchor&) wnothrow
				: SharedAnchor()
			{}
			SharedAnchor(SharedAnchor&& a) wnothrow
				: Ptr(a.Ptr)
			{}

			PDefHOp(SharedAnchor&, =, const SharedAnchor& a)
				ImplRet(white::copy_and_swap(*this, a))
				PDefHOp(SharedAnchor&, =, SharedAnchor&& a)
				ImplRet(white::move_and_swap(*this, a))

				friend PDefH(void, swap, SharedAnchor& x, SharedAnchor& y) wnothrow
				ImplRet(swap(x.Ptr, y.Ptr))
		};

	public:
		mutable BindingMap Bindings{};
		/*!
		\exception WSLException 对实现异常中立的未指定派生类型的异常。
		\note 失败时若抛出异常，条件由实现定义。
		*/
		//@{
		/*!
		\brief 重定向解析环境：返回非局部名称引用的环境。
		\return 符合条件的重定向后的环境指针，或无法重定向时的空值。

		在局部解析失败时，尝试提供能进一步解析名称的环境。
		若不支持重定向，总是返回空指针值。
		*/
		std::function<observer_ptr<const Environment>(string_view)> Redirect{
			std::bind(DefaultRedirect, std::cref(*this), std::placeholders::_1) };
		/*!
		\brief 解析名称：处理保留名称并查找名称。
		\return 查找到的名称，或查找失败时的空值。
		\pre 实现断言：第二参数的数据指针非空。
		\sa LookupName
		\sa Redirect

		解析指定环境中的名称。被解析的环境可对特定保留名称重定向。
		实现名称解析的一般步骤包括：
		局部解析：对被处理的上下文查找名称，若成功则返回；
		否则，若支持重定向，尝试重定向解析：在重定向后的环境中重新查找名称；
		否则，名称解析失败。
		名称解析失败时默认返回空值。对特定的失败，实现可约定抛出异常。
		保留名称的集合和是否支持重定向由实现约定。
		只有和名称解析的相关保留名称被处理。其它保留名称被忽略。
		不保证对循环重定向进行检查。
		*/
		std::function<NameResolution(string_view)> Resolve{
			std::bind(DefaultResolve, std::cref(*this), std::placeholders::_1) };
		//@}
		/*!
		\brief 父环境：被解释的重定向目标。
		\sa DefaultRedirect
		*/
		ValueObject Parent{};

	private:
		/*!
		\brief 锚对象指针：提供被引用计数。
		*/
		SharedAnchor anchor{};

	public:
		//! \brief 无参数构造：初始化空环境。
		DefDeCtor(Environment)
			DefDeCopyMoveCtorAssignment(Environment)
		/*!
		\brief 构造：使用包含绑定节点的指针。
		\note 不检查绑定的名称。
		*/
		//@{
		explicit
		Environment(const BindingMap&& m)
			: Bindings(m)
		{}
		explicit
			Environment(BindingMap&& m)
			: Bindings(std::move(m))
		{}
		//@}
		/*!
		\brief 构造：使用父环境。
		\exception WSLException 异常中立：由 CheckParent 抛出。
		\todo 使用专用的异常类型。
		*/
		//@{
		explicit
			Environment(const ValueObject& vo)
			: Parent((CheckParent(vo), vo))
		{}
		explicit
			Environment(ValueObject&& vo)
			: Parent((CheckParent(vo), std::move(vo)))
		{}
		//@}

		friend PDefHOp(bool, == , const Environment& x, const Environment& y)
			wnothrow
			ImplRet(x.Bindings == y.Bindings)

		/*!
		\brief 判断锚对象未被外部引用。
		*/
		DefPred(const wnothrow, NotReferenced, anchor.Ptr.use_count() == 1)

		/*!
		\brief 取名称绑定映射。
		*/
		DefGetter(const wnothrow, BindingMap&, MapRef, Bindings)
		DefGetter(const wnothrow, const shared_ptr<const void>&, AnchorPtr,
			anchor.Ptr)

		/*!
		\brief 引用锚对象。
		*/
		PDefH(shared_ptr<const void>, Anchor, ) const
		ImplRet(anchor.Ptr)

		//@{
		/*!
		\brief 检查可作为父环境的宿主对象。
		\note 若存在父环境，首先对父环境递归检查。
		\exception WSLException 异常中立：由 ThrowForInvalidType 抛出。
		\todo 使用专用的异常类型。
		*/
		static void
		CheckParent(const ValueObject&);

		/*!
		\brief 移除第一参数中名称和第二参数中重复的绑定项。
		\return 移除后的目的结果中没有绑定。
		*/
		static bool
			Deduplicate(BindingMap&, const BindingMap&);

		//! \pre 断言：第二参数的数据指针非空。
		//@{
		/*!
		\brief 重定向解析环境：返回父环境。
		\sa Parent

		解析 Parent 储存的对象作为父环境的引用值。
		被处理的保留名称应指定重定向名称到有限个不同的环境。
		支持的重定向项的宿主值的类型包括：
		EnvironmentList ：环境列表；
		observer_ptr<const Environment> 无所有权的重定向环境；
		EnvironmentReference 可能具有共享所有权的重定向环境；
		shared_ptr<Environment> 具有共享所有权的重定向环境。
		若重定向可能具有共享所有权的失败，则表示资源访问错误，如构成循环引用；
		可能涉及未定义行为。
		以上支持的宿主值类型被依次检查。若检查成功，则使用此宿主值类型访问环境。
		对列表，使用 DFS （深度优先搜索）依次递归检查其元素。
		*/
		static observer_ptr<const Environment>
			DefaultRedirect(const Environment&, string_view);

		/*!
		\brief 解析名称：处理保留名称并查找名称。
		\exception WSLException 访问共享重定向环境失败。
		\sa Lookup
		\sa Redirect
		*/
		static NameResolution
			DefaultResolve(const Environment&, string_view);
		//@}
		//@}

		/*!
		\pre 字符串参数的数据指针非空。
		\throw BadIdentifier 非强制调用时发现标识符不存在或冲突。
		\note 最后一个参数表示强制调用。
		\warning 应避免对被替换或移除的值的悬空引用。
		*/
		//@{
		//! \brief 以字符串为标识符在指定上下文中定义值。
		void
			Define(string_view, ValueObject&&, bool);

		/*!
		\brief 查找名称。
		\pre 断言：第二参数的数据指针非空。
		\return 查找到的名称，或查找失败时的空值。

		在环境中查找名称。
		*/
		observer_ptr<ValueNode>
			LookupName(string_view) const;

		//! \pre 间接断言：第一参数的数据指针非空。
		//@{
		//! \brief 以字符串为标识符在指定上下文中覆盖定义值。
		void
			Redefine(string_view, ValueObject&&, bool);

		/*
		\brief 以字符串为标识符在指定上下文移除对象。
		\return 是否成功移除。
		*/
		bool
			Remove(string_view, bool);
		//@}
		//@}

		/*!
		\brief 对不符合环境要求的类型抛出异常。
		\throw WSLException 环境类型检查失败。
		\todo 使用专用的异常类型。
		*/
		WB_NORETURN static void
			ThrowForInvalidType(const white::type_info&);
	};


	/*!
	\brief 环境引用。

	可能共享所有权环境的引用。
	*/
	class WS_API EnvironmentReference
		: private white::equality_comparable<EnvironmentReference>
	{
	private:
		weak_ptr<Environment> p_weak;
		//! \brief 引用的锚对象指针。
		shared_ptr<const void> p_anchor;

	public:
		//! \brief 构造：使用指定的环境指针和此环境的锚对象指针。
		EnvironmentReference(const shared_ptr<Environment>&);
		//! \brief 构造：使用指定的环境指针和锚对象指针。
		//@{
		template<typename _tParam1, typename _tParam2>
		EnvironmentReference(_tParam1&& arg1, _tParam2&& arg2)
			: p_weak(wforward(arg1)), p_anchor(wforward(arg2))
		{}
		//@}
		DefDeCopyMoveCtorAssignment(EnvironmentReference)

			DefGetter(const wnothrow, const weak_ptr<Environment>&, Ptr, p_weak)
			DefGetter(const wnothrow, const shared_ptr<const void>&, AnchorPtr,
				p_anchor)

			PDefH(shared_ptr<Environment>, Lock, ) const wnothrow
			ImplRet(p_weak.lock())

			//! \since build 824
			friend PDefHOp(bool, == , const EnvironmentReference& x,
				const EnvironmentReference& y) wnothrow
			ImplRet(x.p_weak.lock() == y.p_weak.lock())
	};


	/*!
	\ingroup ThunkType
	\brief 项引用。
	\warning 非虚析构。

	表示列表项的引用的中间求值结果的项。
	*/
	class WS_API TermReference
	{
	private:
		white::lref<TermNode> term_ref;
		/*!
		\brief 引用的锚对象指针。
		*/
		shared_ptr<const void> p_anchor{};

	public:
		TermReference(TermNode& term)
			: term_ref(term)
		{}
		template<typename _tParam, typename... _tParams>
		TermReference(TermNode& term, _tParam&& arg, _tParams&&... args)
			: term_ref(term), p_anchor(wforward(arg), wforward(args)...)
		{}
		DefDeCopyCtor(TermReference)

			DefDeCopyAssignment(TermReference)

			friend PDefHOp(bool, == , const TermReference& x, const TermReference& y)
			ImplRet(white::get_equal_to<>()(x.term_ref, y.term_ref))

			DefCvtMem(const wnothrow, TermNode&, term_ref)

			DefGetter(const wnothrow, const shared_ptr<const void>&, AnchorPtr,
				p_anchor)

			PDefH(TermNode&, get, ) const wnothrow
			ImplRet(term_ref.get())
	};

	/*!
	\brief 规约函数类型：和绑定所有参数的求值遍的处理器等价。
	\warning 假定转移不抛出异常。
	*/
	using Reducer = white::GHEvent<ReductionStatus()>;


	/*!
	\brief 上下文节点。
	\warning 非虚析构。
	*/
	class WS_API ContextNode
	{
	private:
		/*!
		\brief 环境记录指针。
		\invariant p_record 。
		*/
		shared_ptr<Environment> p_record{ make_shared<Environment>() };

	public:
		EvaluationPasses EvaluateLeaf{};
		EvaluationPasses EvaluateList{};
		LiteralPasses EvaluateLiteral{};
		GuardPasses Guard{};
		/*!
		\brief 当前动作。
		\note 为便于确保释放，不使用 white::one_shot 。
		*/
		Reducer Current{};
		/*!
		\brief 跳过表达式求值标记。

		不影响 Current 状态而直接支持取消特定求值规约的共享标记。
		用于派生实现在单一表达式内的逃逸控制流。
		*/
		bool SkipToNextEvaluation{};
		/*!
		\brief 定界动作：边界外的剩余动作。
		*/
		white::deque<Reducer> Delimited{};
		/*!
		\brief 最后一次规约状态。
		\sa ApplyTail
		*/
		ReductionStatus LastStatus = ReductionStatus::Clean;
		/*!
		\brief 上下文日志追踪。
		*/
		white::Logger Trace{};

		DefDeCtor(ContextNode)
			/*!
			\throw std::invalid_argument 参数指针为空。
			\note 遍和日志追踪对象被复制。
			*/
			ContextNode(const ContextNode&, shared_ptr<Environment>&&);
		DefDeCopyCtor(ContextNode)
			/*!
			\brief 转移构造。
			\post <tt>p_record->Bindings.empty()</tt> 。
			*/
			ContextNode(ContextNode&&) wnothrow;
		DefDeCopyMoveAssignment(ContextNode)

			DefGetter(const wnothrow, Environment::BindingMap&, BindingsRef,
				GetRecordRef().GetMapRef())
			DefGetter(const wnothrow, Environment&, RecordRef, *p_record)

			/*!
			\brief 转移并应用尾调用，更新规约状态。
			\note 调用前脱离 Current 以允许调用 SetupTail 设置新的尾调用。
			\pre 断言： \c Current 。
			\sa LastStatus
			*/
			ReductionStatus
			ApplyTail();

		/*!
		\sa Delimited
		\sa SetupTail
		\sa Current
		*/
		//@{
		/*!
		\brief 转移首个定界动作为当前动作。
		\pre 断言： \c !Delimited.empty() 。
		\pre 间接断言： \c !Current 。
		\post \c Current 。
		\sa Delimited
		\sa SetupTail
		*/
		void
			Pop() wnothrow;

		/*!
		\brief 转移当前动作为首个定界动作。
		\pre 断言： \c Current 。
		*/
		//@{
		//@{
		void
			Push(const Reducer&);
		void
			Push(Reducer&&);
		//@}
		//! \pre \c !Current 。
		PDefH(void, Push, )
			ImplExpr(Push(Reducer()))
			//@}
			//@}

		//! \exception std::bad_function_call Reducer 参数为空。
		//@{
		/*!
		\brief 重写项。
		\pre 间接断言：\c !Current 。
		\post \c Current 。
		\sa ApplyTail
		\sa SetupTail
		\sa Transit
		\note 不处理重规约。

		调用 Transit 转变状态，当可规约时调用 ApplyTail 迭代规约重写。
		因为递归重写平摊到单一的循环， CheckReducible 不用于判断是否需要继续重写循环。
		每次调用 Current 的结果同步到 TailResult 。
		返回值为最后一次 Current 调用结果。
		*/
		ReductionStatus
		Rewrite(Reducer);

		/*!
		\brief 构造作用域守卫并重写项。
		\sa Guard
		\sa Rewrite

		重写逻辑包括以下顺序的步骤：
		调用 ContextNode::Guard 进行必要的上下文重置；
		调用 Rewrite 。
		*/
		ReductionStatus
			RewriteGuarded(TermNode&, Reducer);
		//@}

		/*!
		\brief 设置当前动作以重规约。
		\pre 断言：\c !Current 。
		\pre 动作转移无异常抛出。
		*/
		template<typename _func>
		void
			SetupTail(_func&& f)
		{
#if false
			// XXX: Not applicable for functor with %Reducer is captured.
			using func_t = white::decay_t<_func>;
			static_assert(white::or_<std::is_same<func_t, Reducer>,
				white::is_nothrow_move_constructible<func_t>>(),
				"Invalid type found.");
#endif

			WAssert(!Current, "Old continuation is overriden.");
			Current = wforward(f);
		}

		/*!
		\brief 切换当前动作。
		*/
		PDefH(Reducer, Switch, Reducer f = {})
			ImplRet(white::exchange(Current, std::move(f)))

		/*!
		\brief 切换环境。
		*/
		//@{
		/*!
		\throw std::invalid_argument 参数指针为空。
		\sa SwitchEnvironmentUnchecked
		*/
		shared_ptr<Environment>
		SwitchEnvironment(shared_ptr<Environment>);

		//! \pre 断言：参数指针非空。
		shared_ptr<Environment>
			SwitchEnvironmentUnchecked(shared_ptr<Environment>) wnothrowv;
		//@}

		/*!
		\brief 按需转移首个定界动作为当前动作。
		\pre 间接断言： \c !Current 。
		\return 是否可继续规约。
		\sa Pop
		*/
		bool
			Transit() wnothrow;

		//@{
		PDefH(shared_ptr<Environment>, ShareRecord, ) const
			ImplRet(GetRecordRef().shared_from_this())

		PDefH(EnvironmentReference, WeakenRecord, ) const
			ImplRet(ShareRecord())

		friend PDefH(void, swap, ContextNode& x, ContextNode& y) wnothrow
			ImplExpr(swap(x.p_record, y.p_record), swap(x.EvaluateLeaf,
				y.EvaluateLeaf), swap(x.EvaluateList, y.EvaluateList),
				swap(x.EvaluateLiteral, y.EvaluateLiteral), swap(x.Guard, y.Guard),
				swap(x.Trace, y.Trace))
		//@}
	};


	/*!
	\brief 环境切换器。
	\warning 非虚析构。
	*/
	struct EnvironmentSwitcher
	{
		white::lref<ContextNode> Context;
		mutable shared_ptr<Environment> SavedPtr;

		EnvironmentSwitcher(ContextNode& ctx,
			shared_ptr<Environment>&& p_saved = {})
			: Context(ctx), SavedPtr(std::move(p_saved))
		{}
		DefDeMoveCtor(EnvironmentSwitcher)

		DefDeMoveAssignment(EnvironmentSwitcher)

		void
		operator()() const wnothrowv
		{
			if (SavedPtr)
				Context.get().SwitchEnvironmentUnchecked(std::move(SavedPtr));
		}
	};

	/*!
	\note 参数分别为上下文、捕获的当前动作和捕获的后继动作。
	\note 以参数声明的相反顺序捕获参数作为动作，结果以参数声明的顺序析构捕获的动作。
	*/
	//@{
	/*!
	\brief 合并规约动作：创建指定上下文中的连续异步规约当前和后继动作的规约动作。
	\note 若当前动作为空，则直接使用后继动作作为结果。
	*/
	WS_API Reducer
		CombineActions(ContextNode&, Reducer&&, Reducer&&);

	//! \return ReductionStatus::Retrying 。
	//@{
	/*!
	\brief 异步规约当前和后继动作。
	\sa CombineActions
	*/
	WS_API ReductionStatus
		RelayNext(ContextNode&, Reducer&&, Reducer&&);
	//@}

	/*!
	\brief 异步规约指定动作和非空的当前动作。
	\pre 断言： \tt ctx.Current 。
	*/
	inline PDefH(ReductionStatus, RelaySwitchedUnchecked, ContextNode& ctx,
		Reducer&& cur)
		ImplRet(WAssert(ctx.Current, "No action found to be the next action."),
			RelayNext(ctx, std::move(cur), ctx.Switch()))

	/*!
	\brief 异步规约指定动作和当前动作。
	\sa RelaySwitchedUnchecked
	*/
	inline PDefH(ReductionStatus, RelaySwitched, ContextNode& ctx, Reducer&& cur)
	ImplRet(ctx.Current ? RelaySwitchedUnchecked(ctx, std::move(cur))
		: (ctx.SetupTail(std::move(cur)), ReductionStatus::Retrying))
	//@}

	/*!
	\brief 转移动作到当前动作及定界动作。
	*/
	inline PDefH(void, MoveAction, ContextNode& ctx, Reducer&& act)
		ImplExpr(!ctx.Current ? ctx.SetupTail(std::move(act))
			: ctx.Push(std::move(act)))

	//@{
	//! \brief 上下文处理器类型。
	using ContextHandler = white::GHEvent<ReductionStatus(TermNode&, ContextNode&)>;
	//! \brief 字面量处理器类型。
	using LiteralHandler = white::GHEvent<ReductionStatus(const ContextNode&)>;

	//! \brief 注册上下文处理器。
	inline PDefH(void, RegisterContextHandler, ContextNode& ctx,
		const string& name, ContextHandler f)
		ImplExpr(ctx.GetBindingsRef()[name].Value = std::move(f))

		//! \brief 注册字面量处理器。
		inline PDefH(void, RegisterLiteralHandler, ContextNode& ctx,
			const string& name, LiteralHandler f)
		ImplExpr(ctx.GetBindingsRef()[name].Value = std::move(f))
		//@}

	//@{
	//! \brief 从指定环境查找名称对应的节点。
	template<typename _tKey>
	inline observer_ptr<ValueNode>
		LookupName(Environment& ctx, const _tKey& id) wnothrow
	{
		return white::AccessNodePtr(ctx.GetMapRef(), id);
	}

	template<typename _tKey>
	inline observer_ptr<const ValueNode>
		LookupName(const Environment& ctx, const _tKey& id) wnothrow
	{
		return white::AccessNodePtr(ctx.GetMapRef(), id);
	}

	//! \brief 从指定环境取指定名称指称的值。
	template<typename _tKey>
	ValueObject
		FetchValue(const Environment& ctx, const _tKey& name)
	{
		return GetValueOf(scheme::LookupName(ctx, name));
	}

	template<typename _tKey>
	static observer_ptr<const ValueObject>
		FetchValuePtr(const Environment& ctx, const _tKey& name)
	{
		return GetValuePtrOf(scheme::LookupName(ctx, name));
	}
	//@}

} // namespace scheme;

#endif
