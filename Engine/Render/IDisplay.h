/*! \file Engine\Render\IDisplay.h
\ingroup Engine
\brief 显示逻辑相关,平台UI层的桥接。
*/

#ifndef WE_RENDER_IDisplay_h
#define WE_RENDER_IDisplay_h 1

#include <WBase/sutility.h>
#include "IFormat.hpp"

namespace platform::Render {
	enum StereoMethod {
		Stereo_None,
		Stereo_LCDShutter,
	};

	//\brief 默认数值为默认设置
	struct DisplaySetting {
		white::uint32 sample_count = 1;
		white::uint32 sample_quality = 0;

		bool full_screen = false;
		bool sync_interval = true;

		white::uint32  screen_width, screen_height;//属于用户设置

		StereoMethod stereo_method = Stereo_None;

		EFormat depth_stencil_format = EF_D32FS8X24;
		EFormat color_format = EF_ARGB8;
	};

	/* \warning 非虚析构
	*/
	class Display :private wimpl(white::noncopyable), private wimpl(white::nonmovable) {
	public:
		virtual void SwapBuffers() = 0;
		virtual void WaitOnSwapBuffers() = 0;
	};
}

#endif