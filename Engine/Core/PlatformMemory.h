#pragma once

#include <WFramework/WCLib/Platform.h>
#include <WBase/wmemory.hpp>

#if WFL_Win32
#define lalloca(Size) ((Size==0) ? 0 : (void*)(((std::intptr_t)_alloca(Size + 15) + 15) & ~15))
#endif