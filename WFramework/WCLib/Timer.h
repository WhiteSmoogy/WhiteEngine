/*!	\file Timer.h
\ingroup WFrameWork/WCLib
\brief 平台相关的计时器接口。
*/

#ifndef WFrameWork_WCLib_Timer_h
#define WFrameWork_WCLib_Timer_h 1

#include <WFramework/WCLib/FCommon.h>

namespace platform {
	/*!
	\brief 取 tick 数。
	\note 单位为毫秒。
	\warning 首次调用 StartTicks 前非线程安全。
	*/
	WF_API std::uint32_t
		GetTicks() wnothrow;

	/*!
	\brief 取高精度 tick 数。
	\note 单位为纳秒。
	\warning 首次调用 StartTicks 前非线程安全。
	*/
	WF_API std::uint64_t
		GetHighResolutionTicks() wnothrow;
}

#endif