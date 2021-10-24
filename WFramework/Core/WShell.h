/*!	\file WShell.h
\ingroup WFrameWork/Core
\brief Shell ����
*/

#ifndef WFrameWork_Core_WShell_H
#define WFrameWork_Core_WShell_H 1

#include <WFramework/Core/wmsgdef.h>

namespace white
{

	namespace Shells
	{
		//! \brief ��ǳ���ʵ�������ڿ�����ӳ�����塣
		class WF_API Shell : private noncopyable, public enable_shared_from_this<Shell>
		{
		public:
			/*!
			\brief �޲������졣
			*/
			DefDeCtor(Shell)
				/*!
				\brief ������
				*/
				virtual
				~Shell();

			/*!
			\brief �ж� Shell �Ƿ��ڼ���״̬��
			*/
			bool
				IsActive() const;

			/*!
			\brief Ĭ�� Shell ��������
			\note ����Ĭ�� Shell ����ΪӦ�ó���û�д���� Shell ��Ϣ�ṩĬ�ϴ���
			ȷ��ÿһ����Ϣ�õ�����
			*/
			static void
				DefShlProc(const Message&);

			/*!
			\brief ������Ϣ����Ӧ�̵߳�ֱ�ӵ��á�
			*/
			virtual PDefH(void, OnGotMessage, const Message& msg)
				ImplExpr(DefShlProc(msg))
		};

	} // namespace Shells;

} // namespace white;

#endif
