/*!	\file Timer.h
\ingroup WFrameWork/WCLib
\brief ƽ̨��صļ�ʱ���ӿڡ�
*/

#ifndef WFrameWork_WCLib_Timer_h
#define WFrameWork_WCLib_Timer_h 1

#include <WFramework/WCLib/FCommon.h>

namespace platform {
	/*!
	\brief ȡ tick ����
	\note ��λΪ���롣
	\warning �״ε��� StartTicks ǰ���̰߳�ȫ��
	*/
	WF_API std::uint32_t
		GetTicks() wnothrow;

	/*!
	\brief ȡ�߾��� tick ����
	\note ��λΪ���롣
	\warning �״ε��� StartTicks ǰ���̰߳�ȫ��
	*/
	WF_API std::uint64_t
		GetHighResolutionTicks() wnothrow;
}

#endif