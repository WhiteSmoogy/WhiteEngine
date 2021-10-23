/*! \file Core\WSLEvaluator.h
\ingroup Engine
\brief WSL求值器(解释器内核对象)。
*/
#ifndef WE_Core_WSLEvaluator_H
#define WE_Core_WSLEvaluator_H 1

#include <WScheme/SContext.h>
#include <WScheme/WScheme.h>
#include <WScheme/WSchemREPL.h>

namespace platform {
	using REPLContext = scheme::v1::REPLContext;

	/*
	\brief 对REPL上下文的包装,并屏蔽一些函数
	\brief 主要目的是扩展Debug输出,并且可以继承扩展包含多个上下文
	*/
	class WSLEvaluator {
	protected:
		//Terminal
		REPLContext context;

	public:
		WSLEvaluator(std::function<void(REPLContext&)>);

		virtual ~WSLEvaluator();

		/*!
		\brief 加载：从指定参数指定的来源读取并处理源代码。
		\sa REPLContext::LoadFrom
		*/
		template<class _type>
		void
			LoadFrom(_type& input)
		{
			context.LoadFrom(input);
		}
		

		/*
		\brief 执行求值循环:对非空输入进行翻译
		\sa REPLContext::Process
		*/
		//@{
		template<class _type>
		scheme::TermNode
			Eval(const _type& input)
		{
			return context.Process(input);
		}

		/*
		\sa REPLContext::Perform
		*/
		scheme::TermNode 
			Eval(scheme::string_view unit) {
			return context.Perform(unit);
		}

		//@}
	};
}

#endif