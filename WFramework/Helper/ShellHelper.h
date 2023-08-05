/*!	\file ShellHelper.h
\ingroup WFrameWorkHelper
\brief Shell °ïÖúÄ£¿é¡£
*/

#ifndef WFrameWork_Helper_ShellHelper_h
#define WFrameWork_Helper_ShellHelper_h 1

#include <WBase/wdef.h>
#include <spdlog/spdlog.h>
#include <spdlog/stopwatch.h>
namespace white {
	class SpdlogTrace
	{
	public:
		SpdlogTrace(std::string&& name)
			:post_name(std::move(name))
		{}

		~SpdlogTrace()
		{
			spdlog::info("{}: {} seconds", post_name, sw);
		}
	private:
		std::string post_name;
		spdlog::stopwatch sw;
	};

#define LOG_TRACE(...) white::SpdlogTrace WPP_Join(trace_,__LINE__){std::format(__VA_ARGS__)}
}

#endif
