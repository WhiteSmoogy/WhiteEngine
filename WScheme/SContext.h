/*!	\file SContext.h
\ingroup WSL
\brief S ���ʽ�����ġ�
\par �޸�ʱ��:
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

	//! \brief ��ڵ㣺�洢�﷨���������ֵ���ͽڵ㡣
	using TermNode = wimpl(ValueNode);

	using TNIter = TermNode::iterator;
	using TNCIter = TermNode::const_iterator;

	/*!
	\brief ��ڵ�����жϲ�����
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
		\brief ����֦�ڵ㡣
		\pre ���ԣ�����ָ��������֦�ڵ㡣
		*/
		inline PDefH(void, AssertBranch, const TermNode& term,
			const char* msg = "Invalid term found.") wnothrowv
		ImplExpr(wunused(msg), WAssert(IsBranch(term), msg))

		/*!
		\brief �����ڵ��Ƿ����ָ����ֵ��
		*/
		template<typename _type>
		inline bool
			HasValue(const TermNode& term, const _type& x)
		{
			return term.Value == x;
		}


		/*!
		\brief �Ự������ָ�� WSL ���롣
		*/
		class WS_API Session
	{
	public:
		//@{
		using CharParser = std::function<void(LexicalAnalyzer&, char)>;

		LexicalAnalyzer Lexer;

		DefDeCtor(Session)
			//! \throw LoggedEvent �ؼ�ʧ�ܣ��޷�����Դ���ݡ�
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
			\brief Ĭ���ַ�����ʵ�֣�ֱ��ʹ�� LexicalAnalyzer::ParseByte ��
			*/
			static void
			DefaultParseByte(LexicalAnalyzer&, char);

		/*!
		\brief Ĭ���ַ�����ʵ�֣�ֱ��ʹ�� LexicalAnalyzer::ParseQuoted ��
		*/
		static void
			DefaultParseQuoted(LexicalAnalyzer&, char);
	};


	/*!
	\brief S ���ʽ�����ģ����� S ���ʽ��
	*/
	namespace SContext
	{

		/*!
		\brief �����Ǻ��б���֤�����Ϸ��ԣ�Բ�����Ƿ��Ӧ��
		\param b ��ʼ��������
		\param e ��ֹ��������
		\pre ��������ͬһ���Ǻ��б�ĵ����������� b ����ɽ����ã����� e ֮ǰ��
		\return e ��ָ������� ')' �ĵ�������
		\throw LoggedEvent �������ҵ������ '(' ��
		*/
		WS_API TLCIter
			Validate(TLCIter b, TLCIter e);

		//@{
		/*!
		\brief ������Լ�Ǻ��б�ȡ�����﷨��������ָ��ֵ���ͽڵ㡣
		\param term ��ڵ㡣
		\param b ��ʼ��������
		\param e ��ֹ��������
		\pre ��������ͬһ���Ǻ��б�ĵ����������� b ����ɽ����ã����� e ֮ǰ��
		\return e ��ָ������� ')' �ĵ�������
		\throw LoggedEvent �������ҵ������ '(' ��
		*/
		WS_API TLCIter
			Reduce(TermNode& term, TLCIter b, TLCIter e);


		/*!
		\brief ����ָ��Դ��ȡ�����﷨��������ָ��ֵ���ͽڵ㡣
		\throw LoggedEvent �������ҵ������ ')' ��
		*/
		//@{
		WS_API void
			Analyze(TermNode&, const TokenList&);
		WS_API void
			Analyze(TermNode&, const Session&);
		//@}
		//! \note ���� ADL \c Analyze �����ڵ㡣
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
