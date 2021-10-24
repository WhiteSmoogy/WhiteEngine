/*!	\file HostWindow.h
\ingroup Helper
\brief �����������ڡ�
*/

#ifndef WFrameWork_Helper_HostWindow_h
#define WFrameWork_Helper_HostWindow_h 1

#include <WFramework/WCLib/FCommon.h>
#include <WFramework/Core/wmsgdef.h>


namespace white {
	class GUIHost;

#if WF_Hosted
	namespace Host {
		class Window;
		class WindowThread;
	}
#endif
}

#include <WFramework/WCLib/HostGUI.h>
#if WFL_Win32
#include <WFramework/Core/WString.h>
#endif

namespace white {

#if WF_Hosted
	namespace Host
	{
		using namespace platform_ex;

		/*!
		\brief ��������֧�ֵĴ��ڡ�
		*/
		class WF_API Window : public HostWindow
		{
		private:
			lref<GUIHost> host;

#if WFL_Win32
		public:
			/*!
			\brief ����Ƿ�ʹ�ò�͸���Գ�Ա��
			\note ʹ�� Windows �������ʵ�֣����� WindowReference ʵ�ֲ�ͬ��ʹ��
			::UpdateLayeredWindows ���� WM_PAINT ���´��ڡ�
			\warning ʹ�ò�͸���Գ�Աʱ�ڴ˴����ϵ��� ::SetLayeredWindowAttributes ��
			GetOpacity �� SetOpacity ���ܳ���
			*/
			bool UseOpacity{};
			/*!
			\brief ��͸���ԡ�
			\note ������������ WS_EX_LAYERED ��ʽ�� UseOpacity ����Ϊ true ʱ��Ч��
			*/
			Drawing::AlphaType Opacity{ 0xFF };

			WindowInputHost InputHost;
#endif

		public:
			/*!
			\exception LoggedEvent �쳣������������������ WindowClassName ��
			*/
			//@{
			Window(NativeWindowHandle);
			Window(NativeWindowHandle, GUIHost&);
			//@}
			~Window() override;

			DefGetter(const wnothrow, GUIHost&, GUIHostRef, host)

			/*!
			\brief ˢ�£�������Ⱦ״̬ͬ����
			*/
			virtual PDefH(void, Refresh, )
			ImplExpr(void())
		};

	} // namespace Host;
#endif
}

#endif
