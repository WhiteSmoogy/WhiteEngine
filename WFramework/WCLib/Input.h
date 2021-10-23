/*!	\file Input.h
\ingroup WFrameWork/WCLib
\brief 平台相关的扩展输入接口。
*/

#ifndef WFrameWork_WCLib_Input_h
#define WFrameWork_WCLib_Input_h 1


#include <WFramework/WCLib/FCommon.h>

namespace platform {

}

namespace platform_ex {
	/*!
	\brief 清除按键缓冲。
	*/
	WF_API void
		ClearKeyStates();
}

#endif