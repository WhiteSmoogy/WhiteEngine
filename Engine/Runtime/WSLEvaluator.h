/*! \file Core\WSLEvaluator.h
\ingroup Engine
\brief WSL��ֵ��(�������ں˶���)��
*/
#ifndef WE_Core_WSLEvaluator_H
#define WE_Core_WSLEvaluator_H 1

#include <WScheme/SContext.h>
#include <WScheme/WScheme.h>
#include <WScheme/WSchemREPL.h>

namespace platform {
	using REPLContext = scheme::v1::REPLContext;

	/*
	\brief ��REPL�����ĵİ�װ,������һЩ����
	\brief ��ҪĿ������չDebug���,���ҿ��Լ̳���չ�������������
	*/
	class WSLEvaluator {
	protected:
		//Terminal
		REPLContext context;

	public:
		WSLEvaluator(std::function<void(REPLContext&)>);

		virtual ~WSLEvaluator();

		/*!
		\brief ���أ���ָ������ָ������Դ��ȡ������Դ���롣
		\sa REPLContext::LoadFrom
		*/
		template<class _type>
		void
			LoadFrom(_type& input)
		{
			context.LoadFrom(input);
		}
		

		/*
		\brief ִ����ֵѭ��:�Էǿ�������з���
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