/*! \file System\NinthTimer.h
\ingroup Engine
\brief ��Ϸ���������õ�ʱ�ӡ�
*/

#ifndef WE_System_NinthTimer_H
#define WE_System_NinthTimer_H 1

#include "TimeValue.h"
#include <WBase/winttype.hpp>
#include <WBase/wmacro.h>

namespace platform::chrono {
	class NinthTimer {
	public:
		enum class TimerType : white::uint8 {
			//! \brief Pausable, serialized, frametime is smoothed/scaled/clamped.
			Normal =0, 
			//! \brief Non-pausable, non-serialized, frametime unprocessed.
			Monotonic ,
		};

		NinthTimer();

		DefDeDtor(NinthTimer)

	public:
		void Reset();

	public:
		//! \brief Updates the timer every frame, needs to be called by the system.
		void UpdateOnFrameStart();
		float GetFrameTime(TimerType type = TimerType::Normal);
		float GetRealFrameTime() const;

		/*! 
		\brief try to pause/unpause the TimerType::Normal timer returns true if successfully paused/unpaused, false otherwise
		*/
		//@{
		bool Pasue();
		bool Continue();
		//@}

		//! \brief determine if the TimerType::Normal timer is paused returns true if paused, false otherwise
		bool IsPaused();
	public:
		const TimeValue& GetFrameStartTime(TimerType type = TimerType::Normal) const;

	private:
		static const unsigned int TimerTypeCount = 2;
		using AdapterDuration = std::chrono::duration<double, std::chrono::seconds::period>;

		void OffsetToGameTime(Duration pasue_duration);

		//return the give type count in seconds
		template<typename _target,typename _duration>
		_target GetAdapterDurationCount(const _duration& duration) {
			return static_cast<_target>(std::chrono::duration_cast<AdapterDuration>(duration).count());
		}

		void RefreshNormalTime(Duration duration);
		void RefreshMonotonicTime(Duration duration);
	private:
		TimeValue timespans[TimerTypeCount];

		bool enable = true;
		white::uint64 frame_counter = 0;

		//�༭����,��Ⱦ���Ա�ֹͣ
		//ע��,������ʱ��ֹͣ��Ҳ����ֹͣ��Ⱦ
#if ENGINE_TOOL
		white::uint64 render_frame_counter = 0;
#endif

		//TimePoint when since system boot, all other tick-unit variables are relative to this.
		TimePoint start_point;
		//This is the base for Monotonic time. it always moves forward at a constant rate until the timer is Reset()).
		Duration last_duration;
		// Additional ticks for Normal time (relative to Monotonic time). Game time can be affected by loading, pausing, time smoothing and time clamping.
		Duration offset_duration;

		//In seconds since the last Update(), clamped/smoothed etc.
		float frame_time;
		// In real seconds since the last Update(), non-clamped/un-smoothed etc.
		float real_frametime;

		//if normal time is paused.GetFrameTime will return 0;
		bool paused_normal_timer;
		//when the normal time is paused,On un-pause,offset will be adjuseted to match.
		Duration normal_pasued_duration;
	};

	NinthTimer& FetchGlobalTimer();
}

#endif