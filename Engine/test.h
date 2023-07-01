#include <WBase/wdef.h>
#include <WFramework/Win32/WCLib/COM.h> //NoMinMax

#include <dxgi1_6.h>
#include "emacro.h"


namespace platform {
	namespace Window {
		enum WindowRotation {
			WR_Rotate90,
			WR_Rotate270
		};
	}

	namespace Render {
		
	}
}

namespace platform_ex {
	 namespace Windows {
		namespace D3D12 {
		}
	}
}

IDXGISwapChain* WE_API Create(HWND hwnd);