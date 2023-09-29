#pragma once

#include <WFramework/WCLib/Platform.h>

namespace white::threading {
	void SetThreadDescription(void* hThread, const char* lpThreadDescription);
}
