/*! \file System\TimeValue.h
\ingroup Engine
\brief ʱ�������λ������������API��
*/

#ifndef WE_System_TimeValue_H
#define WE_System_TimeValue_H 1

#include <WFramework/Core/WClock.h>

namespace platform::chrono {
	
	using ::white::Timers::HighResolutionClock;

	//������ʹ�õ����ʱ�侫��Ϊ10΢��
	using EngineTick = std::ratio<1, 100000>;
	using EngineTickType = white::make_signed_t<std::chrono::nanoseconds::rep>;

	using Duration = white::Timers::Duration;
	using TimePoint = white::Timers::TimePoint;
	using Tick = Duration::period;
	using TickType = Duration::rep;

	class TimeValue : public std::chrono::duration<
		EngineTickType, EngineTick>
	{
	public:
		using base = std::chrono::duration<
			EngineTickType, EngineTick>;
		using base::base;

		template<typename _rep>
		void SetSecondDuration(const std::chrono::duration<_rep, std::chrono::seconds::period>& duration) {
			static_cast<base&>(*this) =std::chrono::duration_cast<base>(duration);
		}
	};
}

#endif