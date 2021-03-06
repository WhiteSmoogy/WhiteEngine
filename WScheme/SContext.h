/*!	\file SContext.h
\ingroup WSL
\brief S 表达式上下文。
\par 修改时间:
2017-03-12 12:40 +0800
*/

#ifndef WScheme_SContext_H
#define WScheme_SContext_H 1

#include "Lexical.h"
#include "WFramework/Core/ValueNode.h"

namespace scheme
{
	using white::ValueObject;
	using white::ValueNode;
	using TokenList = list<string>;
	using TLIter = TokenList::iterator;
	using TLCIter = TokenList::const_iterator;
	//@{
	using white::observer_ptr;
	using white::LoggedEvent;

	//! \brief 项节点：存储语法分析结果的值类型节点。
	using TermNode = wimpl(ValueNode);

	using TNIter = TermNode::iterator;
	using TNCIter = TermNode::const_iterator;

	/*!
	\brief 项节点分类判断操作。
	*/
	//@{
	inline PDefH(bool, IsBranch, const TermNode& term) wnothrow
		ImplRet(!term.empty())

		inline PDefH(bool, IsEmpty, const TermNode& term) wnothrow
		ImplRet(!term)

		inline PDefH(bool, IsLeaf, const TermNode& term) wnothrow
		ImplRet(term.empty())

		inline PDefH(bool, IsList, const TermNode& term) wnothrow
		ImplRet(!term.empty() || !term.Value)
		//@}

		inline PDefH(TermNode&, MapToTermNode, TermNode& term)
		ImplRet(term)
		inline PDefH(const TermNode&, MapToTermNode, const TermNode& term)
		ImplRet(term)

		inline PDefH(ValueNode&, MapToValueNode, ValueNode& node)
		ImplRet(node)
		inline PDefH(const ValueNode&, MapToValueNode, const ValueNode& node)
		ImplRet(node)
		//@}


		/*!
		\brief 断言枝节点。
		\pre 断言：参数指定的项是枝节点。
		*/
		inline PDefH(void, AssertBranch, const TermNode& term,
			const char* msg = "Invalid term found.") wnothrowv
		ImplExpr(wunused(msg), WAssert(IsBranch(term), msg))

		/*!
		\brief 检查项节点是否具有指定的值。
		*/
		template<typename _type>
		inline bool
			HasValue(const TermNode& term, const _type& x)
		{
			return term.Value == x;
		}


		/*!
		\brief 会话：分析指定 WSL 代码。
		*/
		class WS_API Session
	{
	public:
		//@{
		using CharParser = std::function<void(LexicalAnalyzer&, char)>;

		LexicalAnalyzer Lexer;

		DefDeCtor(Session)
			//! \throw LoggedEvent 关键失败：无法访问源内容。
			template<typename _tIn>
		Session(_tIn first, _tIn last, CharParser parse = DefaultParseByte)
			: Lexer()
		{
			std::for_each(first, last,
				std::bind(parse, std::ref(Lexer), std::placeholders::_1));
		}
		template<typename _tRange,
			wimpl(typename = white::exclude_self_t<Session, _tRange>)>
			Session(const _tRange& c, CharParser parse = DefaultParseByte)
			: Session(begin(c), end(c), parse)
		{}
		DefDeCopyMoveCtorAssignment(Session)

			DefGetterMem(const wnothrow, const string&, Buffer, Lexer)
			//@}
			DefGetter(const, TokenList, TokenList, Tokenize(Lexer.Literalize()))

			/*!
			\brief 默认字符解析实现：直接使用 LexicalAnalyzer::ParseByte 。
			*/
			static void
			DefaultParseByte(LexicalAnalyzer&, char);

		/*!
		\brief 默认字符解析实现：直接使用 LexicalAnalyzer::ParseQuoted 。
		*/
		static void
			DefaultParseQuoted(LexicalAnalyzer&, char);
	};


	/*!
	\brief S 表达式上下文：处理 S 表达式。
	*/
	namespace SContext
	{

		/*!
		\brief 遍历记号列表，验证基本合法性：圆括号是否对应。
		\param b 起始迭代器。
		\param e 终止迭代器。
		\pre 迭代器是同一个记号列表的迭代器，其中 b 必须可解引用，且在 e 之前。
		\return e 或指向冗余的 ')' 的迭代器。
		\throw LoggedEvent 警报：找到冗余的 '(' 。
		*/
		WS_API TLCIter
			Validate(TLCIter b, TLCIter e);

		//@{
		/*!
		\brief 遍历规约记号列表，取抽象语法树储存至指定值类型节点。
		\param term 项节点。
		\param b 起始迭代器。
		\param e 终止迭代器。
		\pre 迭代器是同一个记号列表的迭代器，其中 b 必须可解引用，且在 e 之前。
		\return e 或指向冗余的 ')' 的迭代器。
		\throw LoggedEvent 警报：找到冗余的 '(' 。
		*/
		WS_API TLCIter
			Reduce(TermNode& term, TLCIter b, TLCIter e);


		/*!
		\brief 分析指定源，取抽象语法树储存至指定值类型节点。
		\throw LoggedEvent 警报：找到冗余的 ')' 。
		*/
		//@{
		WS_API void
			Analyze(TermNode&, const TokenList&);
		WS_API void
			Analyze(TermNode&, const Session&);
		//@}
		//! \note 调用 ADL \c Analyze 分析节点。
		template<typename _type>
		TermNode
			Analyze(const _type& arg)
		{
			TermNode root;

			Analyze(root, arg);
			return root;
		}
		//@}

	} // namespace SContext;

} // namespace scheme;


#endif
