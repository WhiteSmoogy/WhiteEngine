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
	\brief �м�ֵ���͡�

	����ض���ֵ���ԣ������� TermNode �� Value ���ݳ�Ա�в�ֱ�ӱ�ʾ�������Զ�������͡�
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
	\brief LA Ԫ��ǩ��
	\note LA �� LScheme �ĳ���ʵ�֡�
	*/
	struct WS_API WSLATag : WTag
	{};


	/*!
	\brief �����﷨�ڵ㡣

	��ָ���Ľڵ�����Խڵ��С�ַ���Ϊ���ƵĽڵ㣬�������﷨��������
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
	\brief �ڵ�ӳ��������ͣ��任�ڵ�Ϊ��һ���ڵ㡣
	*/
	using NodeMapper = std::function<ValueNode(const TermNode&)>;

	//@{
	using NodeToString = std::function<string(const ValueNode&)>;

	template<class _tCon>
	using GNodeInserter = std::function<void(TermNode&&, _tCon&)>;

	using NodeInserter = GNodeInserter<TermNode::Container&>;

	using NodeSequenceInserter = GNodeInserter<NodeSequence>;
	//@}


	//! \return �������½ڵ㡣
	//@{
	/*!
	\brief ӳ�� WSLA Ҷ�ڵ㡣
	\sa ParseWSLANodeString

	�����½ڵ㡣������Ϊ���򷵻�ֵΪ�մ����½ڵ㣻����ֵ�� ParseWSLANodeString ȡ�á�
	*/
	WS_API ValueNode
		MapWSLALeafNode(const TermNode&);

	/*!
	\brief �任�ڵ�Ϊ�﷨������Ҷ�ڵ㡣
	\note ��ѡ����ָ��������ơ�
	*/
	WS_API ValueNode
		TransformToSyntaxNode(const ValueNode&, const string& = {});
	//@}

	/*!
	\brief ת�� WSLA �ڵ���������
	\return ���� EscapeLiteral ת������ַ����Ľ����
	\exception white::bad_any_cast �쳣�������� Access �׳���
	*/
	WS_API string
		EscapeNodeLiteral(const ValueNode&);

	/*!
	\brief ת�� WSLA �ڵ���������
	\return ����Ϊ�սڵ���մ���������� Literalize �������� EscapeNodeLiteral �Ľ����
	\exception white::bad_any_cast �쳣�������� EscapeNodeLiteral �׳���
	\sa EscapeNodeLiteral
	*/
	WS_API string
		LiteralizeEscapeNodeLiteral(const ValueNode&);

	/*!
	\brief ���� WSLA �ڵ��ַ�����

	�� string ���ͷ��ʽڵ㣬��ʧ������Ϊ�մ���
	*/
	WS_API string
		ParseWSLANodeString(const ValueNode&);


	/*!
	\brief �����﷨�ӽڵ㡣

	��ָ���Ľڵ�����Խڵ��С�ַ���Ϊ���ƵĽڵ㣬�������﷨��������
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


	//! \brief ����ǰ׺�����ĺ������͡�
	using IndentGenerator = std::function<string(size_t)>;

	//! \brief ����ˮƽ�Ʊ��Ϊ��λ��������
	WS_API string
		DefaultGenerateIndent(size_t);

	/*!
	\brief ��ӡ������
	\note �����һ���������������޸����á�
	*/
	WS_API void
		PrintIndent(std::ostream&, IndentGenerator = DefaultGenerateIndent, size_t = 1);

	//@{
	/*!
	\brief �����ӽڵ㡣
	\note ʹ�� ADL AccessPtr ��

	�����ڵ������е��ӽڵ㡣
	���ȵ��� AccessPtr ���Է��� NodeSequence ������ֱ����Ϊ�ڵ��������ʡ�
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

	//! \brief ��ӡ�����߽�����е� NPLA �ڵ㣬���ڴ�ӡ�߽�ǰ����ǰ�ò�����
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
	\brief ��ӡ������ǰ׺�Ľڵ������ӽڵ㲢��ӡ��
	\note ʹ�� ADL IsPrefixedIndex ��
	\sa IsPrefixedIndex
	\sa TraverseSubnodes

	�Ե���������Ϊ�߽�ǰ�ò��������� PrintContainedNodes �����ӡ�ӽڵ����ݡ�
	���õ��Ĳ���������һ������������������Ϊǰ׺��Ȼ���ӡ�ӽڵ����ݡ�
	������ IsPrefixedIndex �Ľڵ���õ��Ĳ�����Ϊ�ڵ��ַ�����ӡ��
	���򣬵��õ�������ݹ��ӡ�ӽڵ㣬���Դ˹����е� std::out_of_range �쳣��
	���У������ӽڵ�ͨ������ TraverseSubnodes ʵ�֡�
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
	\brief ��ӡ WSLA �ڵ㡣
	\sa PrintIdent
	\sa PrintNodeChild
	\sa PrintNodeString

	���õ��Ĳ���������һ������������������Ϊǰ׺��һ���ո�Ȼ���ӡ�ڵ����ݣ�
	�ȳ��Ե��� PrintNodeString ��ӡ�ڵ��ַ��������ɹ�ֱ�ӷ��أ�
	�����ӡ���У�Ȼ���Ե��� PrintNodeChild ��ӡ NodeSequence ��
	�ٴ�ʧ������� PrintNodeChild ��ӡ�ӽڵ㡣
	���� PrintNodeChild ��ӡ������س���
	*/
	WS_API void
		PrintNode(std::ostream&, const ValueNode&, NodeToString = EscapeNodeLiteral,
			IndentGenerator = DefaultGenerateIndent, size_t = 0);

	/*!
	\brief ��ӡ��Ϊ�ӽڵ�� WSLA �ڵ㡣
	\sa IsPrefixedIndex
	\sa PrintIdent
	\sa PrintNodeString

	���õ��Ĳ���������һ������������������Ϊǰ׺��Ȼ���ӡ�ӽڵ����ݡ�
	������ IsPrefixedIndex �Ľڵ���� PrintNodeString ��Ϊ�ڵ��ַ�����ӡ��
	���򣬵��� PrintNode �ݹ��ӡ�ӽڵ㣬���Դ˹����е� std::out_of_range �쳣��
	*/
	WS_API void
		PrintNodeChild(std::ostream&, const ValueNode&, NodeToString
			= EscapeNodeLiteral, IndentGenerator = DefaultGenerateIndent, size_t = 0);

	/*!
	\brief ��ӡ�ڵ��ַ�����
	\return �Ƿ�ɹ����ʽڵ��ַ����������
	\note white::bad_any_cast ���쳣������
	\sa PrintNode

	ʹ�����һ������ָ���ķ��ʽڵ㣬��ӡ�õ����ַ����ͻ��з���
	���� white::bad_any_cast ��
	*/
	WS_API bool
		PrintNodeString(std::ostream&, const ValueNode&,
			NodeToString = EscapeNodeLiteral);


	namespace sxml
	{

		//@{
		/*!
		\brief ת�� sxml �ڵ�Ϊ XML �����ַ�����
		\throw LoggedEvent û���ӽڵ㡣
		\note ��ǰ��֧�� annotation ���ڳ��� 2 ���ӽڵ�ʱʹ�� TraceDe ���档
		*/
		WS_API string
			ConvertAttributeNodeString(const TermNode&);

		/*!
		\brief ת�� sxml �ĵ��ڵ�Ϊ XML �ַ�����
		\throw LoggedEvent ���������һ������Լ�������ݱ�������
		\throw white::unimplemented ָ�� ParseOption::Strict ʱ����δʵ�����ݡ�
		\sa ConvertStringNode
		\see http://okmij.org/ftp/Scheme/sxml.html#Annotations ��
		\todo ֧�� *ENTITY* �� *NAMESPACES* ��ǩ��

		ת�� sxml �ĵ��ڵ�Ϊ XML ��
		����ʹ�� ConvertStringNode ת���ַ����ڵ㣬��ʧ����Ϊ��Ҷ�ӽڵ�ݹ�ת����
		��Ϊ��ǰ sxml �淶δָ��ע��(annotation) ������ֱ�Ӻ��ԡ�
		*/
		WS_API string
			ConvertDocumentNode(const TermNode&, IndentGenerator = DefaultGenerateIndent,
				size_t = 0, ParseOption = ParseOption::Normal);

		/*!
		\brief ת�� sxml �ڵ�Ϊ��ת��� XML �ַ�����
		\sa EscapeXML
		*/
		WS_API string
			ConvertStringNode(const TermNode&);

		/*!
		\brief ��ӡ SContext::Analyze ����ȡ�õ� sxml �﷨���ڵ㲢ˢ������
		\see ConvertDocumentNode
		\see SContext::Analyze
		\see Session

		�����ڵ���ȡ��һ���ڵ���Ϊ sxml �ĵ��ڵ���� ConvertStringNode �����ˢ������
		*/
		WS_API void
			PrintSyntaxNode(std::ostream& os, const TermNode&,
				IndentGenerator = DefaultGenerateIndent, size_t = 0);
		//@}


		//@{
		//! \brief ���� sxml �ĵ������ڵ㡣
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
			\brief ���� sxml �ĵ� XML �����ڵ㡣
			\note ��һ������ָ���ڵ����ƣ��������ָ���ڵ��� XML ������ֵ���汾������Ͷ����ԡ�
			\note �������������ѡΪ��ֵ����ʱ�����������Ӧ�����ԡ�
			\warning ���Բ����Ϲ��Խ��м�顣
			*/
			WS_API ValueNode
			MakeXMLDecl(const string& = {}, const string& = "1.0",
				const string& = "UTF-8", const string& = {});

		/*!
		\brief ������� XML ������ sxml �ĵ��ڵ㡣
		\sa MakeTop
		\sa MakeXMLDecl
		*/
		WS_API ValueNode
			MakeXMLDoc(const string& = {}, const string& = "1.0",
				const string& = "UTF-8", const string& = {});

		//! \brief ���� sxml ���Ա����������
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

		//! \brief ���� XML ������������
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

		//! \brief ����ֻ�� XML ���Խڵ㵽�﷨�ڵ㡣
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
	  //! \brief WSL �쳣���ࡣ
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

			//! \brief ���������ඨ����Ĭ��ʵ�֡�
			virtual	~WSLException() override;
	};


	/*!
	\brief �б��Լʧ�ܡ�
	\todo ���񲢱�����������Ϣ��
	*/
	class WS_API ListReductionFailure : public WSLException
	{
	public:
		using WSLException::WSLException;
		DefDeCtor(ListReductionFailure)

			//! \brief ���������ඨ����Ĭ��ʵ�֡�
			~ListReductionFailure() override;
	};


	//! \brief �﷨����
	class WS_API InvalidSyntax : public WSLException
	{
	public:
		using WSLException::WSLException;
		DefDeCtor(InvalidSyntax)

			//! \brief ���������ඨ����Ĭ��ʵ�֡�
			~InvalidSyntax() override;
	};

	class WS_API ParameterMismatch : public InvalidSyntax
	{
	public:
		using InvalidSyntax::InvalidSyntax;
		DefDeCtor(ParameterMismatch)

			//! \brief ���������ඨ����Ĭ��ʵ�֡�
			~ParameterMismatch() override;
	};

	/*!
	\brief Ԫ����ƥ�����
	\todo ֧�ַ�Χƥ�䡣
	*/
	class WS_API ArityMismatch : public WSLException
	{
	private:
		size_t expected;
		size_t received;

	public:
		DefDeCtor(ArityMismatch)
			/*!
			\note ǰ����������ʾ������ʵ�ʵ�Ԫ����
			*/
			ArityMismatch(size_t, size_t, white::RecordLevel = white::Err);

		//! \brief ���������ඨ����Ĭ��ʵ�֡�
		~ArityMismatch() override;

		DefGetter(const wnothrow, size_t, Expected, expected)
			DefGetter(const wnothrow, size_t, Received, received)
	};

	/*!
	\brief ��ʶ������
	*/
	class WS_API BadIdentifier : public WSLException
	{
	private:
		white::shared_ptr<string> p_identifier;

	public:
		/*!
		\brief ���죺ʹ����Ϊ��ʶ�����ַ�������֪ʵ�����ͺͼ�¼�ȼ���
		\pre ��Ӷ��ԣ���һ����������ָ��ǿա�

		������һ����������Ϊ��ʶ���ĺϷ��ԣ�ֱ����Ϊ������ı�ʶ����
		��ʼ����ʾ��ʶ��������쳣����
		ʵ�������� 0 ʱ��ʾδ֪��ʶ����
		ʵ�������� 1 ʱ��ʾ�Ƿ���ʶ����
		ʵ�������� 1 ʱ��ʾ�ظ���ʶ����
		*/
		WB_NONNULL(2)
			BadIdentifier(const char*, size_t = 0, white::RecordLevel = white::Err);
		BadIdentifier(string_view, size_t = 0, white::RecordLevel = white::Err);
		DefDeCtor(BadIdentifier)

			//! \brief ���������ඨ����Ĭ��ʵ�֡�
			~BadIdentifier() override;

		DefGetter(const wnothrow, const string&, Identifier,
			white::Deref(p_identifier))
	};

	//@}
	//@}

	/*!
	\brief ��Ч���ô���
	*/
	class WS_API InvalidReference : public WSLException
	{
	public:
		using WSLException::WSLException;
		DefDeCtor(InvalidReference)

			//! \brief ���������ඨ����Ĭ��ʵ�֡�
			~InvalidReference() override;
	};
	//@}



		//@{
		//! \brief ���������
	enum class LexemeCategory
	{
		//! \brief �ޣ�����������
		Symbol,
		//! \brief ������������
		Code,
		//! \brief ������������
		Data,
		/*!
		\brief ��չ��������������ʵ�ֶ����������������
		*/
		Extended
	};



	//! \sa LexemeCategory
	//@{
	/*!
	\pre ���ԣ��ַ�������������ָ��ǿ����ַ����ǿա�
	\return �жϵķ���չ���������ࡣ
	*/
	//@{
	/*!
	\brief ���ų���չ�������Ĵ��ط��ࡣ
	\note ��չ��������Ϊ����������
	*/
	WS_API LexemeCategory
		CategorizeBasicLexeme(string_view) wnothrowv;

	/*!
	\brief �Դ��ط��ࡣ
	\sa CategorizeBasicLexeme
	*/
	WS_API LexemeCategory
		CategorizeLexeme(string_view) wnothrowv;
	//@}

	/*!
	\brief �жϲ��Ƿ���չ�������Ĵ����Ƿ�Ϊ WSLA ��չ��������
	\pre ���ԣ��ַ�������������ָ��ǿ����ַ����ǿա�
	\pre ���ز��Ǵ�����������������������
	*/
	WS_API bool
		IsWSLAExtendedLiteral(string_view) wnothrowv;

	/*!
	\brief �ж��ַ��Ƿ�Ϊ WSLA ��չ������������ǰ׺��
	*/
	wconstfn PDefH(bool, IsWSLAExtendedLiteralNonDigitPrefix, char c) wnothrow
		ImplRet(c == '#' || c == '+' || c == '-')

		//! \brief �ж��ַ��Ƿ�Ϊ WSLA ��չ������ǰ׺��
		inline PDefH(bool, IsWSLAExtendedLiteralPrefix, char c) wnothrow
		ImplRet(std::isdigit(c) || IsWSLAExtendedLiteralNonDigitPrefix(c))

		/*!
		\brief �жϴ����Ƿ�Ϊ WSLA ���š�
		\pre ��Ӷ��ԣ��ַ�������������ָ��ǿ����ַ����ǿա�
		*/
		inline PDefH(bool, IsWSLASymbol, string_view id) wnothrowv
		ImplRet(CategorizeLexeme(id) == LexemeCategory::Symbol)
		//@}
		//@}


		/*!
		\ingroup ThunkType
		\brief �Ǻ�ֵ��
		\note �ͱ���ֵ���ַ�����ͬ�İ�װ���͡�
		\warning �ǿ�������
		*/
		using TokenValue = white::derived_entity<string, WSLATag>;


	/*!
	\brief �������ֵ��Ϊ�Ǻš�
	\return ͨ���������ֵȡ�õļǺŵ�ָ�룬���ָ���ʾ�޷�ȡ�����ơ�
	*/
	WS_API observer_ptr<const TokenValue>
		TermToNamePtr(const TermNode&);

	/*!
	\brief �������ֵ��ת��Ϊ�ַ�����ʽ���ⲿ��ʾ��
	\return ת���õ����ַ�����
	\sa TermToNamePtr

	�������ֵ��Ϊ����ת��Ϊ�ַ�������ʧ������ȡֵ�����ͺ���������Ϊ����ֵ�ı�ʾ��
	����������ⲿ��ʾ����δָ�������������ʵ�ֱ仯��
	*/
	WS_API string
		TermToString(const TermNode&);

	/*!
	\brief ��ǼǺŽڵ㣺�ݹ�任�ڵ㣬ת�����еĴ���Ϊ�Ǻ�ֵ��
	\note �ȱ任�ӽڵ㡣
	*/
	WS_API void
		TokenizeTerm(TermNode& term);

	/*!
	\brief ��Լ״̬��ĳ�����ϵ�һ���Լ���ܵ��м�����
	*/
	enum class ReductionStatus : wimpl(size_t)
	{
		//@{
		//! \brief ��Լ�ɹ���ֹ�Ҳ���Ҫ�������
		Clean = 0,
			//! \brief ��Լ�ɹ�����Ҫ�������
			Retained,
			//! \brief ��Ҫ�ع�Լ��
			Retrying
			//@}
	};


	/*!
	\ingroup ThunkType
	\brief �ӳ���ֵ�
	\note �ͱ��ӳ���ֵ��������ڵ��ǲ�ͬ�İ�װ���͡�
	\warning �ǿ�������

	ֱ����Ϊ���ֵ�����װ���ӳ���ֵ���
	*/
	using DelayedTerm = white::derived_entity<TermNode, WSLATag>;

	//@{
	/*!
	\brief �����ȡ���� TermReference ���ֵ������á�
	\return ����� Value ���ݳ�ԱΪ TermReference ��Ϊ���е����ã�����Ϊ������
	\sa TermReference
	*/
	WS_API TermNode&
		ReferenceTerm(TermNode&);
	WS_API const TermNode&
		ReferenceTerm(const TermNode&);

	/*!
	\brief ���ʽ��� TermReference ������ָ�����Ͷ���
	\exception std::bad_cast �쳣��������ʵ�������ͼ��ʧ�ܡ�
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

	//! \brief ���ʽ��� TermReference ������ָ�����Ͷ���ָ�롣
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
	//! \brief �����������
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
	\brief �����Ϊ��ʽ�Ľڵ㲢��ȡ��Լ״̬��
	*/
	inline PDefH(ReductionStatus, CheckNorm, const TermNode& term) wnothrow
		ImplRet(IsBranch(term) ? ReductionStatus::Retained : ReductionStatus::Clean)

	/*!
	\brief ���ݹ�Լ״̬����Ƿ�ɼ�����Լ��
	\see TraceDe

	ֻ��������״̬ȷ����������ҽ�����Լ�ɹ�ʱ����Ϊ������Լ��
	�����ֲ�֧�ֵ�״̬��Ϊ���ɹ���������档
	��ֱ�Ӻ� ReductionStatus::Retrying �Ƚ��Է������� ReductionStatus �ľ���ֵ��ʵ�֡�
	����ʵ�ֿ�ʹ�����ƵĽӿ�ָ�������ͬ��״̬��
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

	//! \brief ʹ�õڶ�������ָ������������滻��һ��������ݡ�
	//@{
	inline PDefH(void, LiftTerm, TermNode& term, TermNode& tm)
		ImplExpr(term.SetContent(std::move(tm)))

	inline PDefH(void, LiftTerm, ValueObject& term_v, ValueObject& vo)
		ImplExpr(term_v = std::move(vo))
	inline PDefH(void, LiftTerm, TermNode& term, ValueObject& vo)
		ImplExpr(LiftTerm(term.Value, vo))
	//@}

	/*!
	\brief �����������ã����ƻ�ת�Ƽ��������ʹ������ֵ��
	*/
	inline PDefH(void, LiftTermIndirection, TermNode& term, const TermNode& tm)
		// NOTE: See $2018-02 @ %Documentation::Workflow::Annual2018.
		ImplExpr(white::SetContentWith(term, tm, &ValueObject::MakeMoveCopy))

	/*!
	\warning ����ļ��ֵ������Ȩ��Ӧע������������ʹ���Ա�֤�ڴ氲ȫ��
	\todo ֧������ֵ�͸��ơ�
	*/
	//@{
	/*!
	\note ����֧��ʵ�ֶ��������е���ֵ����ֵת����
	\sa LiftTerm
	\sa LiftTermRef
	\sa TermReference
	*/
	//@{
	/*!
	\brief ������򴴽������

	��� Value ���ݳ�ԱΪ TermReference ���͵�ֵʱ���� LiftTermRef ��
	����ͬ LiftTerm ��
	*/
	WS_API void
	LiftTermOrRef(TermNode&, TermNode&);

	/*!
	\brief �������������
	\sa LiftTermOrRef

	����ͬ����ͬ�������� LiftTermOrRef ��
	�����ܵ��� LiftTermRef �������� LiftTerm �Խ�Լ����Ҫ�Ŀ�����
	*/
	WS_API void
		LiftTermRefToSelf(TermNode&);
	//@}

	/*!
	\brief ���������ã�ʹ�õڶ�������ָ����������������滻��һ��������ݡ�
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
	\brief ���������Ϊ���á�
	\throw WSLException ���ʧ�ܣ�����ֵ�Ҳ����ж����Ψһ����Ȩ�����ܱ��ⲿ���ñ��档
	\throw white::invalid_construction ����������ֵ��
	\sa LiftTerm
	\todo ʹ�þ������������쳣���͡�
	*/
	WS_API void
		LiftToReference(TermNode&, TermNode&);

	/*!
	\brief �ݹ�������������ݹ鴴����������Ӧ�İ������ֵ�����������
	\note ���������ֵ������������ȷ�������ʾ����ֵʱ��������
	\sa LiftTermRefToSelf
	*/
	WS_API void
		LiftToSelf(TermNode&);

	/*!
	\brief �ݹ�������������ݹ鴴����������Ӧ�İ������ֵ�ļ�����������
	\sa LiftToSelf
	\sa LiftTermIndirection

	���� LiftToSelf ��Ȼ��ݹ������ͬ�������� LiftTermIndirection ���ƻ�ת������
	*/
	WS_API void
		LiftToSelfSafe(TermNode&);

	/*!
	\brief �ݹ�������������ݹ鴴����������Ӧ�İ������ֵ������������
	\sa LiftTerm
	\sa LiftToSelf

	�Եڶ��������� LiftToSelf ���ٵ��� LiftTerm ��
	*/
	WS_API void
		LiftToOther(TermNode&, TermNode&);
	//@}

	/*!
	\brief ��ÿ��������� LiftToSelfSafe ��
	\since build 822
	*/
	inline PDefH(void, LiftSubtermsToSelfSafe, TermNode& term)
		ImplExpr(std::for_each(term.begin(), term.end(), LiftToSelfSafe))

	/*!
	\brief �����ӳ��
	*/
	inline PDefH(void, LiftDelayed, TermNode& term, DelayedTerm& tm)
		ImplExpr(LiftTermRef(term, tm))

	//! \pre ���ԣ�����ָ��������֦�ڵ㡣
	//@{
	/*!
	\brief �������ʹ���׸������滻������ݡ�
	*/
	inline PDefH(void, LiftFirst, TermNode& term)
		ImplExpr(IsBranch(term), LiftTerm(term, Deref(term.begin())))

	/*!
	\brief ����ĩ�ʹ�����һ�������滻������ݡ�
	*/
	inline PDefH(void, LiftLast, TermNode& term)
		ImplExpr(IsBranch(term), LiftTerm(term, Deref(term.rbegin())))
	//@}


	/*!
	\pre ��Ӷ��ԣ�����Ϊ֦�ڵ㡣
	\return ReductionStatus::Retained ��
	*/
	//@{
	//! \brief ��ԼΪ�б���֦�ڵ��Ƴ���һ������������µ�������Ϊ�б�
	WS_API ReductionStatus
		ReduceBranchToList(TermNode&) wnothrowv;

	/*!
	\brief ��ԼΪ�б�ֵ����֦�ڵ��Ƴ���һ������������µ�������������Ϊ�б��ֵ��
	\sa LiftSubtermsToSelfSafe
	*/
	WS_API ReductionStatus
		ReduceBranchToListValue(TermNode&) wnothrowv;
	//@}

	/*!
	\sa RemoveHead
	\note ʹ�� ADL RemoveHead ��
	*/
	//@{
	/*!
	\brief ��Լ��һ���ǽ�β���б����
	\return ReductionStatus::Clean ��

	������в�����һ�������ҵ�һ�������ǿ��б����Ƴ���
	������б���Ϊ��һ�������Ա��û�в������ĺ���Ӧ�á�
	*/
	WS_API ReductionStatus
		ReduceHeadEmptyList(TermNode&) wnothrow;

	//! \return �Ƴ���ʱ ReductionStatus::Retained ������ ReductionStatus::Clean��
	//@{
	/*!
	\brief ��ԼΪ�б���֦�ڵ��Ƴ���һ������������µ�������Ϊ�б�
	\sa ReduceBranchToList
	*/
	WS_API ReductionStatus
		ReduceToList(TermNode&) wnothrow;

	/*!
	\brief ��ԼΪ�б�ֵ����֦�ڵ��Ƴ���һ������������µ�������������Ϊ�б��ֵ��
	\sa ReduceBranchToListValue
	*/
	WS_API ReductionStatus
		ReduceToListValue(TermNode&) wnothrow;
	//@}
	//@}


	/*!
	\note һ���һ��������ָ�����ϲ���֮ǰ�Ĺ�Լ������ڶ�����ָ�����ںϲ��Ľ����
	\return �ϲ���Ĺ�Լ�����
	*/
	/*!
	\brief �ϲ���Լ�����

	���ڶ�����ָ���������ϲ���Ĺ�Լ���Ϊ�ڶ�����������Ϊ��һ������
	*/
	wconstfn PDefH(ReductionStatus, CombineReductionResult, ReductionStatus res,
		ReductionStatus r) wnothrow
		ImplRet(r == ReductionStatus::Retained ? r : res)

	/*!
	\brief �ϲ����й�Լ�����
	\sa CheckReducible

	���ڶ�����ָ���ɼ�����Լ�����ϲ���Ĺ�Լ���Ϊ�ڶ�������
	����ͬ CombineReductionResult ��
	���ɼ�����Լ����ָ����ǰ��Լ����Ĺ�Լ����ֹ�����ϲ���һ����ָ���Ľ����
	��һ�����еĹ�Լ����ʹ�û���Ժϲ���Ľ����
	*/
	inline PDefH(ReductionStatus, CombineSequenceReductionResult,
			ReductionStatus res, ReductionStatus r) wnothrow
		ImplRet(CheckReducible(r) ? r : CombineReductionResult(res, r))

	//@{
	/*!
	\brief ��ϲ�������ε���ֱ���ɹ���
	\note �ϲ��������ڱ�ʾ�����ж��Ƿ�Ӧ������Լ������ѭ����ʵ���ٴι�Լһ���
	*/
	struct PassesCombiner
	{
		/*!
		\note �Ա�����쳣������
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
	\note �����ʾ�ж��Ƿ�Ӧ������Լ��
	\sa PassesCombiner
	*/
	//@{
	//! \brief һ��ϲ��顣
	template<typename... _tParams>
	using GPasses = white::GEvent<ReductionStatus(_tParams...),
		white::GCombinerInvoker<ReductionStatus, PassesCombiner>>;
	//! \brief ��ϲ��顣
	using TermPasses = GPasses<TermNode&>;
	//! \brief ��ֵ�ϲ��顣
	using EvaluationPasses = GPasses<TermNode&, ContextNode&>;
	/*!
	\brief �������ϲ��顣
	\pre �ַ�������������ָ��ǿա�
	*/
	using LiteralPasses = GPasses<TermNode&, ContextNode&, string_view>;
	//@}


	//! \brief �������ػ����͡�
	using Guard = white::any;
	/*!
	\brief �������ػ��飺�������ڹ�Լ���̵���ںͳ��ڹ���ִ�еĲ�����
	\todo ֧�ֵ���ʹ�þ�ֵ��
	*/
	using GuardPasses = white::GEvent<Guard(TermNode&, ContextNode&),
		white::GDefaultLastValueInvoker<Guard>>;




	/*!
	\brief �����б�

	ָ�������������õ����򼯺ϡ�
	*/
	using EnvironmentList = vector<ValueObject>;


	/*!
	\brief ������
	\warning ����������
	\warning ���� shared_ptr ������ʽ�����ݵĳ�ʼ����
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
		\brief ê�������ͣ��ṩ�����ü�����
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
		\exception WSLException ��ʵ���쳣������δָ���������͵��쳣��
		\note ʧ��ʱ���׳��쳣��������ʵ�ֶ��塣
		*/
		//@{
		/*!
		\brief �ض���������������طǾֲ��������õĻ�����
		\return �����������ض����Ļ���ָ�룬���޷��ض���ʱ�Ŀ�ֵ��

		�ھֲ�����ʧ��ʱ�������ṩ�ܽ�һ���������ƵĻ�����
		����֧���ض������Ƿ��ؿ�ָ��ֵ��
		*/
		std::function<observer_ptr<const Environment>(string_view)> Redirect{
			std::bind(DefaultRedirect, std::cref(*this), std::placeholders::_1) };
		/*!
		\brief �������ƣ����������Ʋ��������ơ�
		\return ���ҵ������ƣ������ʧ��ʱ�Ŀ�ֵ��
		\pre ʵ�ֶ��ԣ��ڶ�����������ָ��ǿա�
		\sa LookupName
		\sa Redirect

		����ָ�������е����ơ��������Ļ����ɶ��ض����������ض���
		ʵ�����ƽ�����һ�㲽�������
		�ֲ��������Ա�����������Ĳ������ƣ����ɹ��򷵻أ�
		������֧���ض��򣬳����ض�����������ض����Ļ��������²������ƣ�
		�������ƽ���ʧ�ܡ�
		���ƽ���ʧ��ʱĬ�Ϸ��ؿ�ֵ�����ض���ʧ�ܣ�ʵ�ֿ�Լ���׳��쳣��
		�������Ƶļ��Ϻ��Ƿ�֧���ض�����ʵ��Լ����
		ֻ�к����ƽ�������ر������Ʊ����������������Ʊ����ԡ�
		����֤��ѭ���ض�����м�顣
		*/
		std::function<NameResolution(string_view)> Resolve{
			std::bind(DefaultResolve, std::cref(*this), std::placeholders::_1) };
		//@}
		/*!
		\brief �������������͵��ض���Ŀ�ꡣ
		\sa DefaultRedirect
		*/
		ValueObject Parent{};

	private:
		/*!
		\brief ê����ָ�룺�ṩ�����ü�����
		*/
		SharedAnchor anchor{};

	public:
		//! \brief �޲������죺��ʼ���ջ�����
		DefDeCtor(Environment)
			DefDeCopyMoveCtorAssignment(Environment)
		/*!
		\brief ���죺ʹ�ð����󶨽ڵ��ָ��
		\note �����󶨵����ơ�
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
		\brief ���죺ʹ�ø�������
		\exception WSLException �쳣�������� CheckParent �׳���
		\todo ʹ��ר�õ��쳣���͡�
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
		\brief �ж�ê����δ���ⲿ���á�
		*/
		DefPred(const wnothrow, NotReferenced, anchor.Ptr.use_count() == 1)

		/*!
		\brief ȡ���ư�ӳ�䡣
		*/
		DefGetter(const wnothrow, BindingMap&, MapRef, Bindings)
		DefGetter(const wnothrow, const shared_ptr<const void>&, AnchorPtr,
			anchor.Ptr)

		/*!
		\brief ����ê����
		*/
		PDefH(shared_ptr<const void>, Anchor, ) const
		ImplRet(anchor.Ptr)

		//@{
		/*!
		\brief ������Ϊ����������������
		\note �����ڸ����������ȶԸ������ݹ��顣
		\exception WSLException �쳣�������� ThrowForInvalidType �׳���
		\todo ʹ��ר�õ��쳣���͡�
		*/
		static void
		CheckParent(const ValueObject&);

		/*!
		\brief �Ƴ���һ���������ƺ͵ڶ��������ظ��İ��
		\return �Ƴ����Ŀ�Ľ����û�а󶨡�
		*/
		static bool
			Deduplicate(BindingMap&, const BindingMap&);

		//! \pre ���ԣ��ڶ�����������ָ��ǿա�
		//@{
		/*!
		\brief �ض���������������ظ�������
		\sa Parent

		���� Parent ����Ķ�����Ϊ������������ֵ��
		������ı�������Ӧָ���ض������Ƶ����޸���ͬ�Ļ�����
		֧�ֵ��ض����������ֵ�����Ͱ�����
		EnvironmentList �������б�
		observer_ptr<const Environment> ������Ȩ���ض��򻷾���
		EnvironmentReference ���ܾ��й�������Ȩ���ض��򻷾���
		shared_ptr<Environment> ���й�������Ȩ���ض��򻷾���
		���ض�����ܾ��й�������Ȩ��ʧ�ܣ����ʾ��Դ���ʴ����繹��ѭ�����ã�
		�����漰δ������Ϊ��
		����֧�ֵ�����ֵ���ͱ����μ�顣�����ɹ�����ʹ�ô�����ֵ���ͷ��ʻ�����
		���б�ʹ�� DFS ������������������εݹ�����Ԫ�ء�
		*/
		static observer_ptr<const Environment>
			DefaultRedirect(const Environment&, string_view);

		/*!
		\brief �������ƣ����������Ʋ��������ơ�
		\exception WSLException ���ʹ����ض��򻷾�ʧ�ܡ�
		\sa Lookup
		\sa Redirect
		*/
		static NameResolution
			DefaultResolve(const Environment&, string_view);
		//@}
		//@}

		/*!
		\pre �ַ�������������ָ��ǿա�
		\throw BadIdentifier ��ǿ�Ƶ���ʱ���ֱ�ʶ�������ڻ��ͻ��
		\note ���һ��������ʾǿ�Ƶ��á�
		\warning Ӧ����Ա��滻���Ƴ���ֵ���������á�
		*/
		//@{
		//! \brief ���ַ���Ϊ��ʶ����ָ���������ж���ֵ��
		void
			Define(string_view, ValueObject&&, bool);

		/*!
		\brief �������ơ�
		\pre ���ԣ��ڶ�����������ָ��ǿա�
		\return ���ҵ������ƣ������ʧ��ʱ�Ŀ�ֵ��

		�ڻ����в������ơ�
		*/
		observer_ptr<ValueNode>
			LookupName(string_view) const;

		//! \pre ��Ӷ��ԣ���һ����������ָ��ǿա�
		//@{
		//! \brief ���ַ���Ϊ��ʶ����ָ���������и��Ƕ���ֵ��
		void
			Redefine(string_view, ValueObject&&, bool);

		/*
		\brief ���ַ���Ϊ��ʶ����ָ���������Ƴ�����
		\return �Ƿ�ɹ��Ƴ���
		*/
		bool
			Remove(string_view, bool);
		//@}
		//@}

		/*!
		\brief �Բ����ϻ���Ҫ��������׳��쳣��
		\throw WSLException �������ͼ��ʧ�ܡ�
		\todo ʹ��ר�õ��쳣���͡�
		*/
		WB_NORETURN static void
			ThrowForInvalidType(const white::type_info&);
	};


	/*!
	\brief �������á�

	���ܹ�������Ȩ���������á�
	*/
	class WS_API EnvironmentReference
		: private white::equality_comparable<EnvironmentReference>
	{
	private:
		weak_ptr<Environment> p_weak;
		//! \brief ���õ�ê����ָ�롣
		shared_ptr<const void> p_anchor;

	public:
		//! \brief ���죺ʹ��ָ���Ļ���ָ��ʹ˻�����ê����ָ�롣
		EnvironmentReference(const shared_ptr<Environment>&);
		//! \brief ���죺ʹ��ָ���Ļ���ָ���ê����ָ�롣
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
	\brief �����á�
	\warning ����������

	��ʾ�б�������õ��м���ֵ������
	*/
	class WS_API TermReference
	{
	private:
		white::lref<TermNode> term_ref;
		/*!
		\brief ���õ�ê����ָ�롣
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
	\brief ��Լ�������ͣ��Ͱ����в�������ֵ��Ĵ������ȼۡ�
	\warning �ٶ�ת�Ʋ��׳��쳣��
	*/
	using Reducer = white::GHEvent<ReductionStatus()>;


	/*!
	\brief �����Ľڵ㡣
	\warning ����������
	*/
	class WS_API ContextNode
	{
	private:
		/*!
		\brief ������¼ָ�롣
		\invariant p_record ��
		*/
		shared_ptr<Environment> p_record{ make_shared<Environment>() };

	public:
		EvaluationPasses EvaluateLeaf{};
		EvaluationPasses EvaluateList{};
		LiteralPasses EvaluateLiteral{};
		GuardPasses Guard{};
		/*!
		\brief ��ǰ������
		\note Ϊ����ȷ���ͷţ���ʹ�� white::one_shot ��
		*/
		Reducer Current{};
		/*!
		\brief �������ʽ��ֵ��ǡ�

		��Ӱ�� Current ״̬��ֱ��֧��ȡ���ض���ֵ��Լ�Ĺ����ǡ�
		��������ʵ���ڵ�һ���ʽ�ڵ����ݿ�������
		*/
		bool SkipToNextEvaluation{};
		/*!
		\brief ���綯�����߽����ʣ�ද����
		*/
		white::deque<Reducer> Delimited{};
		/*!
		\brief ���һ�ι�Լ״̬��
		\sa ApplyTail
		*/
		ReductionStatus LastStatus = ReductionStatus::Clean;
		/*!
		\brief ��������־׷�١�
		*/
		white::Logger Trace{};

		DefDeCtor(ContextNode)
			/*!
			\throw std::invalid_argument ����ָ��Ϊ�ա�
			\note �����־׷�ٶ��󱻸��ơ�
			*/
			ContextNode(const ContextNode&, shared_ptr<Environment>&&);
		DefDeCopyCtor(ContextNode)
			/*!
			\brief ת�ƹ��졣
			\post <tt>p_record->Bindings.empty()</tt> ��
			*/
			ContextNode(ContextNode&&) wnothrow;
		DefDeCopyMoveAssignment(ContextNode)

			DefGetter(const wnothrow, Environment::BindingMap&, BindingsRef,
				GetRecordRef().GetMapRef())
			DefGetter(const wnothrow, Environment&, RecordRef, *p_record)

			/*!
			\brief ת�Ʋ�Ӧ��β���ã����¹�Լ״̬��
			\note ����ǰ���� Current ��������� SetupTail �����µ�β���á�
			\pre ���ԣ� \c Current ��
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
		\brief ת���׸����綯��Ϊ��ǰ������
		\pre ���ԣ� \c !Delimited.empty() ��
		\pre ��Ӷ��ԣ� \c !Current ��
		\post \c Current ��
		\sa Delimited
		\sa SetupTail
		*/
		void
			Pop() wnothrow;

		/*!
		\brief ת�Ƶ�ǰ����Ϊ�׸����綯����
		\pre ���ԣ� \c Current ��
		*/
		//@{
		//@{
		void
			Push(const Reducer&);
		void
			Push(Reducer&&);
		//@}
		//! \pre \c !Current ��
		PDefH(void, Push, )
			ImplExpr(Push(Reducer()))
			//@}
			//@}

		//! \exception std::bad_function_call Reducer ����Ϊ�ա�
		//@{
		/*!
		\brief ��д�
		\pre ��Ӷ��ԣ�\c !Current ��
		\post \c Current ��
		\sa ApplyTail
		\sa SetupTail
		\sa Transit
		\note �������ع�Լ��

		���� Transit ת��״̬�����ɹ�Լʱ���� ApplyTail ������Լ��д��
		��Ϊ�ݹ���дƽ̯����һ��ѭ���� CheckReducible �������ж��Ƿ���Ҫ������дѭ����
		ÿ�ε��� Current �Ľ��ͬ���� TailResult ��
		����ֵΪ���һ�� Current ���ý����
		*/
		ReductionStatus
		Rewrite(Reducer);

		/*!
		\brief ������������������д�
		\sa Guard
		\sa Rewrite

		��д�߼���������˳��Ĳ��裺
		���� ContextNode::Guard ���б�Ҫ�����������ã�
		���� Rewrite ��
		*/
		ReductionStatus
			RewriteGuarded(TermNode&, Reducer);
		//@}

		/*!
		\brief ���õ�ǰ�������ع�Լ��
		\pre ���ԣ�\c !Current ��
		\pre ����ת�����쳣�׳���
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
		\brief �л���ǰ������
		*/
		PDefH(Reducer, Switch, Reducer f = {})
			ImplRet(white::exchange(Current, std::move(f)))

		/*!
		\brief �л�������
		*/
		//@{
		/*!
		\throw std::invalid_argument ����ָ��Ϊ�ա�
		\sa SwitchEnvironmentUnchecked
		*/
		shared_ptr<Environment>
		SwitchEnvironment(shared_ptr<Environment>);

		//! \pre ���ԣ�����ָ��ǿա�
		shared_ptr<Environment>
			SwitchEnvironmentUnchecked(shared_ptr<Environment>) wnothrowv;
		//@}

		/*!
		\brief ����ת���׸����綯��Ϊ��ǰ������
		\pre ��Ӷ��ԣ� \c !Current ��
		\return �Ƿ�ɼ�����Լ��
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
	\brief �����л�����
	\warning ����������
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
	\note �����ֱ�Ϊ�����ġ�����ĵ�ǰ�����Ͳ���ĺ�̶�����
	\note �Բ����������෴˳�򲶻������Ϊ����������Բ���������˳����������Ķ�����
	*/
	//@{
	/*!
	\brief �ϲ���Լ����������ָ���������е������첽��Լ��ǰ�ͺ�̶����Ĺ�Լ������
	\note ����ǰ����Ϊ�գ���ֱ��ʹ�ú�̶�����Ϊ�����
	*/
	WS_API Reducer
		CombineActions(ContextNode&, Reducer&&, Reducer&&);

	//! \return ReductionStatus::Retrying ��
	//@{
	/*!
	\brief �첽��Լ��ǰ�ͺ�̶�����
	\sa CombineActions
	*/
	WS_API ReductionStatus
		RelayNext(ContextNode&, Reducer&&, Reducer&&);
	//@}

	/*!
	\brief �첽��Լָ�������ͷǿյĵ�ǰ������
	\pre ���ԣ� \tt ctx.Current ��
	*/
	inline PDefH(ReductionStatus, RelaySwitchedUnchecked, ContextNode& ctx,
		Reducer&& cur)
		ImplRet(WAssert(ctx.Current, "No action found to be the next action."),
			RelayNext(ctx, std::move(cur), ctx.Switch()))

	/*!
	\brief �첽��Լָ�������͵�ǰ������
	\sa RelaySwitchedUnchecked
	*/
	inline PDefH(ReductionStatus, RelaySwitched, ContextNode& ctx, Reducer&& cur)
	ImplRet(ctx.Current ? RelaySwitchedUnchecked(ctx, std::move(cur))
		: (ctx.SetupTail(std::move(cur)), ReductionStatus::Retrying))
	//@}

	/*!
	\brief ת�ƶ�������ǰ���������綯����
	*/
	inline PDefH(void, MoveAction, ContextNode& ctx, Reducer&& act)
		ImplExpr(!ctx.Current ? ctx.SetupTail(std::move(act))
			: ctx.Push(std::move(act)))

	//@{
	//! \brief �����Ĵ��������͡�
	using ContextHandler = white::GHEvent<ReductionStatus(TermNode&, ContextNode&)>;
	//! \brief ���������������͡�
	using LiteralHandler = white::GHEvent<ReductionStatus(const ContextNode&)>;

	//! \brief ע�������Ĵ�������
	inline PDefH(void, RegisterContextHandler, ContextNode& ctx,
		const string& name, ContextHandler f)
		ImplExpr(ctx.GetBindingsRef()[name].Value = std::move(f))

		//! \brief ע����������������
		inline PDefH(void, RegisterLiteralHandler, ContextNode& ctx,
			const string& name, LiteralHandler f)
		ImplExpr(ctx.GetBindingsRef()[name].Value = std::move(f))
		//@}

	//@{
	//! \brief ��ָ�������������ƶ�Ӧ�Ľڵ㡣
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

	//! \brief ��ָ������ȡָ������ָ�Ƶ�ֵ��
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
