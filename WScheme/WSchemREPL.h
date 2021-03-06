#ifndef WScheme_WSchemeREPL_H
#define WScheme_WSchemeREPL_H 1

#include "WScheme.h"

namespace scheme {
	namespace v1 {
		/*
		\brief REPL 上下文。
		\warning 非虚析构。

		REPL 表示读取-求值-输出循环。
		每个循环包括一次翻译。
		这只是一个基本的可扩展实现。功能通过操作数据成员控制。
		*/
		class WS_API REPLContext
		{
		public:
			//! \brief 上下文根节点。
			ContextNode Root{};
			//! \brief 预处理节点：每次翻译时预先处理调用的公共例程。
			TermPasses Preprocess{};
			//! \brief 表项处理例程：每次翻译中规约回调处理调用的公共例程。
			EvaluationPasses ListTermPreprocess{};

			/*!
			\brief 构造：使用默认的解释。
			\note 参数指定是否启用对规约深度进行跟踪。
			\sa ListTermPreprocess
			\sa SetupDefaultInterpretation
			\sa SetupTraceDepth
			*/
			REPLContext(bool = {});

			/*!
			\brief 加载：从指定参数指定的来源读取并处理源代码。
			\exception std::invalid_argument 异常中立：由 ReadFrom 抛出。
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
			\brief 执行循环：对非空输入进行翻译。
			\pre 断言：字符串的数据指针非空。
			\throw LoggedEvent 输入为空串。
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
			\brief 处理：分析输入并预处理后进行规约。
			\sa Preprocess
			\sa TokenizeTerm
			*/
			//@{
			void
				Prepare(TermNode&) const;
			/*!
			\return 从参数输入读取的准备的项。
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
			\brief 处理：准备规约项并进行规约。
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
			\brief 读取：从指定参数指定的来源输入源代码并准备规约项。
			\return 从参数输入读取的准备的项。
			\sa Prepare
			*/
			//@{
			//! \throw std::invalid_argument 流状态错误或缓冲区不存在。
			TermNode
				ReadFrom(std::istream&) const;
			TermNode
				ReadFrom(std::streambuf&) const;
			//@}
		};

		/*!
		\brief 尝试加载源代码。
		\exception NPLException 嵌套异常：加载失败。
		\note 第二个参数表示来源，仅用于诊断消息。
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
			//! \brief 创建参数指定的 REPL 的副本并在其中对翻译单元规约以求值。
			WS_API void
				EvaluateUnit(TermNode&, const REPLContext&);
			//@}
		}
	}
}

#endif
