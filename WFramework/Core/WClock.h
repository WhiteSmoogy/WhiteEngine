/*!	\file WClock.h
\ingroup WFrameWork/Core
\brief ʱ�ӽӿ�
*/

#ifndef WFrameWork_Core_WClock_h
#define WFrameWork_Core_WClock_h 1

#include <WFramework/Core/WShellDefinition.h>
#include <WFramework/WCLib/Timer.h>
#include <chrono>

namespace white {
	namespace Timers {
		/*!
		\brief �߾���ʱ�ӡ�
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

			//! \warning �״ε���ǰ���̰߳�ȫ��
			static PDefH(time_point, now, ) wnothrow
				ImplRet(time_point(std::chrono::nanoseconds(platform::GetHighResolutionTicks())))
		};


		/*!
		\brief �߾���ʱ������
		\note ��λΪ���롣
		*/
		using Duration = HighResolutionClock::duration;

		/*!
		\brief ʱ�̡�
		*/
		using TimePoint = HighResolutionClock::time_point;

		/*!
		\brief �;���ʱ������
		\note ��λΪ���롣
		*/
		using TimeSpan = std::chrono::duration<Duration::rep, std::milli>;
	}
}


#endif