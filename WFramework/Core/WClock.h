/*!	\file WClock.h
\ingroup WFrameWork/Core
\brief 时钟接口
*/

#ifndef WFrameWork_Core_WClock_h
#define WFrameWork_Core_WClock_h 1

#include <WFramework/Core/WShellDefinition.h>
#include <WFramework/WCLib/Timer.h>
#include <chrono>

namespace white {
	namespace Timers {
		/*!
		\brief 高精度时钟。
		*/
		class WF_API HighResolutionClock
		{
		public:
			using duration = std::chrono::duration<
				white::make_signed_t<std::chrono::nanoseconds::rep>, std::nano>;
			using rep = duration::rep;
			using period = duration::period;
			using time_point = std::chrono::time_point<HighResolutionClock,
				std::chrono::nanoseconds>;

			static wconstexpr const bool is_steady = {};

			//! \warning 首次调用前非线程安全。
			static PDefH(time_point, now, ) wnothrow
				ImplRet(time_point(std::chrono::nanoseconds(platform::GetHighResolutionTicks())))
		};


		/*!
		\brief 高精度时间间隔。
		\note 单位为纳秒。
		*/
		using Duration = HighResolutionClock::duration;

		/*!
		\brief 时刻。
		*/
		using TimePoint = HighResolutionClock::time_point;

		/*!
		\brief 低精度时间间隔。
		\note 单位为毫秒。
		*/
		using TimeSpan = std::chrono::duration<Duration::rep, std::milli>;
	}
}


#endif