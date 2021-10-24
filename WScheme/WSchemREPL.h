#ifndef WScheme_WSchemeREPL_H
#define WScheme_WSchemeREPL_H 1

#include "WScheme.h"

namespace scheme {
	namespace v1 {
		/*
		\brief REPL �����ġ�
		\warning ����������

		REPL ��ʾ��ȡ-��ֵ-���ѭ����
		ÿ��ѭ������һ�η��롣
		��ֻ��һ�������Ŀ���չʵ�֡�����ͨ���������ݳ�Ա���ơ�
		*/
		class WS_API REPLContext
		{
		public:
			//! \brief �����ĸ��ڵ㡣
			ContextNode Root{};
			//! \brief Ԥ����ڵ㣺ÿ�η���ʱԤ�ȴ�����õĹ������̡�
			TermPasses Preprocess{};
			//! \brief ��������̣�ÿ�η����й�Լ�ص�������õĹ������̡�
			EvaluationPasses ListTermPreprocess{};

			/*!
			\brief ���죺ʹ��Ĭ�ϵĽ��͡�
			\note ����ָ���Ƿ����öԹ�Լ��Ƚ��и��١�
			\sa ListTermPreprocess
			\sa SetupDefaultInterpretation
			\sa SetupTraceDepth
			*/
			REPLContext(bool = {});

			/*!
			\brief ���أ���ָ������ָ������Դ��ȡ������Դ���롣
			\exception std::invalid_argument �쳣�������� ReadFrom �׳���
			\sa ReadFrom
			\sa Reduce
			*/
			//@{
			template<class _type>
			void
				LoadFrom(_type& input)
			{
				LoadFrom(input, Root);
			}
			template<class _type>
			void
				LoadFrom(_type& input, ContextNode& ctx) const
			{
				auto term(ReadFrom(input));

				Reduce(term, ctx);
			}
			//@}


			/*!
			\brief ִ��ѭ�����Էǿ�������з��롣
			\pre ���ԣ��ַ���������ָ��ǿա�
			\throw LoggedEvent ����Ϊ�մ���
			\sa Process
			*/
			//@{
			PDefH(TermNode, Perform, string_view unit)
				ImplRet(Perform(unit, Root))
			TermNode
			Perform(string_view, ContextNode&);
			PDefH(TermNode, Perform, u8string_view unit)
				ImplRet(Perform(unit, Root))
			TermNode
			Perform(u8string_view, ContextNode&);
			//@}

			/*!
			\brief �����������벢Ԥ�������й�Լ��
			\sa Preprocess
			\sa TokenizeTerm
			*/
			//@{
			void
				Prepare(TermNode&) const;
			/*!
			\return �Ӳ��������ȡ��׼�����
			\sa SContext::Analyze
			*/
			//@{
			TermNode
				Prepare(const TokenList&) const;
			TermNode
				Prepare(const Session&) const;
			//@}
			//@}

			/*!
			\brief ����׼����Լ����й�Լ��
			\sa Prepare
			\sa Reduce
			*/
			//@{
			PDefH(void, Process, TermNode& term)
				ImplExpr(Process(term, Root))
			//@{
			void
			Process(TermNode&, ContextNode&) const;
			template<class _type>
			TermNode
				Process(const _type& input)
			{
				return Process(input, Root);
			}
			template<class _type>
			TermNode
				Process(const _type& input, ContextNode& ctx) const
			{
				auto term(Prepare(input));

				Reduce(term, ctx);
				return term;
			}
			//@}
			//@}

			/*!
			\brief ��ȡ����ָ������ָ������Դ����Դ���벢׼����Լ�
			\return �Ӳ��������ȡ��׼�����
			\sa Prepare
			*/
			//@{
			//! \throw std::invalid_argument ��״̬����򻺳��������ڡ�
			TermNode
				ReadFrom(std::istream&) const;
			TermNode
				ReadFrom(std::streambuf&) const;
			//@}
		};

		/*!
		\brief ���Լ���Դ���롣
		\exception NPLException Ƕ���쳣������ʧ�ܡ�
		\note �ڶ���������ʾ��Դ�������������Ϣ��
		\relates REPLContext
		*/
		template<typename... _tParams>
		WB_NONNULL(2) void
			TryLoadSource(REPLContext& context, const char* name, _tParams&&... args)
		{
			TryExpr(context.LoadFrom(wforward(args)...))
				CatchExpr(..., std::throw_with_nested(WSLException(
					white::sfmt("Failed loading external unit '%s'.", name))));
		}

		namespace Forms
		{
			//@{
			//! \brief ��������ָ���� REPL �ĸ����������жԷ��뵥Ԫ��Լ����ֵ��
			WS_API void
				EvaluateUnit(TermNode&, const REPLContext&);
			//@}
		}
	}
}

#endif
