/*!	\file ShellHelper.h
\ingroup WFrameWorkHelper
\brief Shell °ïÖúÄ£¿é¡£
*/

#ifndef WFrameWork_Helper_ShellHelper_h
#define WFrameWork_Helper_ShellHelper_h 1

#include <WFramework/Core/WShell.h>
#include <WFramework/WCLib/Debug.h>
#include <WFramework/Core/WClock.h>
#include <WFramework/Adaptor/WAdaptor.h>

namespace white {
#ifndef NDEBUG
	class WF_API DebugTimer {
	protected:
		string event_info;
		Timers::HighResolutionClock::time_point base_tick;
	public:
		DebugTimer(string_view = "");
		~DebugTimer();
	};
#	define WFL_DEBUG_DECL_TIMER(_name, ...) white::DebugTimer _name(__VA_ARGS__);
#else
#	define WFL_DEBUG_DECL_TIMER(...)
#endif
}

#endif
