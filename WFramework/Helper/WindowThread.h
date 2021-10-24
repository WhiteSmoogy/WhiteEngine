/*!	\file WindowThread.h
\ingroup WFrameWorkHelper
\brief ���������̳߳���
*/

#ifndef WFrameWork_Helper_WindowThread_h
#define WFrameWork_Helper_WindowThread_h 1

#include <WFramework/Helper/HostWindow.h>

namespace white {
#if WF_Hosted
	namespace Host {
		/*!
		\brief ���������̡߳�

		ʹ�ú��ʵĲ���ȡ�ض����͵�ֵ��ʾ�������ڣ��������߳�������������ڵ�ѭ����
		�����������Ϳ����� NativeWindowHandle �� WindowReference �� unique_ptr<Window> ��
		*/
		class WF_API WindowThread : private OwnershipTag<Window>
		{
		public:
			using Guard = white::any;
			using GuardGenerator = std::function<Guard(Window&)>;

		private:
			/*!
			\brief ����ָ�롣
			\note ��ʹ�� \c unique_ptr �Ա���ʵ���̰߳�ȫ��״̬��顣
			*/
			atomic<Window*> p_window{ {} };
			std::thread thrd;

		public:
			/*!
			\brief �ڽ����߳�ʱȡ�ػ�����
			*/
			GuardGenerator GenerateGuard{};

			template<typename... _tParams>
			WindowThread(_tParams&&... args)
				: WindowThread({}, wforward(args)...)
			{}
			//! \since build 589
			template<typename... _tParams>
			WindowThread(GuardGenerator guard_gen, _tParams&&... args)
				: thrd(&WindowThread::ThreadFunc<_tParams...>, this, wforward(args)...),
				GenerateGuard(guard_gen)
			{}
			DefDelMoveCtor(WindowThread)
				/*!
				\brief �������رմ��ڡ�
				\note ����ʵ���׳����쳣��
				*/
				~WindowThread();

			/*!
			\note �̰߳�ȫ��
			*/
			DefGetter(const wnothrow, observer_ptr<Window>, WindowPtr,
				make_observer(static_cast<Window*>(p_window)))

				/*!
				\brief Ĭ�������ػ�����

				���ɵ��ػ������ڹ��������ʱ�ֱ���� EnterWindowThread �� LeaveWindowThread ��
				*/
				static Guard
				DefaultGenerateGuard(Window&);

			std::thread::native_handle_type GetNativeHandle() {
				return thrd.native_handle();
			}

		private:
			//! \since build 623
			template<typename _func, typename... _tParams>
			void
				ThreadFunc(_func&& f, _tParams&&... args) wnothrow
			{
				FilterExceptions([&, this] {
					// XXX: Blocked. 'wforward' cause G++ 5.2 crash: segmentation
					//	fault.
					ThreadLoop(white::invoke(std::forward<_func&&>(f),
						std::forward<_tParams&&>(args)...));
				});
			}

			void
				ThreadLoop(platform_ex::NativeWindowHandle);
			PDefH(void, ThreadLoop, platform_ex::WindowReference wnd_ref)
				ImplRet(ThreadLoop(wnd_ref.GetNativeHandle()))
				void
				ThreadLoop(unique_ptr<Window>);

		public:
			static void
				WindowLoop(Window&);

		private:
			static void
				WindowLoop(Window&, GuardGenerator);
		};
	}
#endif
}

#endif
