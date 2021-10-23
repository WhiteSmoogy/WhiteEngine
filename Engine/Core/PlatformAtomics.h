#pragma once

#include <WBase/winttype.hpp>
#include <atomic>

namespace white
{
#if WB_IMPL_MSCPP
	inline int8 InterlockedExchange(volatile int8* Value, int8 Exchange)
	{
		return (int8)::_InterlockedExchange8((char*)Value, (char)Exchange);
	}

	inline int16 InterlockedExchange(volatile int16* Value, int16 Exchange)
	{
		return (int16)::_InterlockedExchange16((short*)Value, (short)Exchange);
	}

	inline int32 InterlockedExchange(volatile int32* Value, int32 Exchange)
	{
		return (int32)::_InterlockedExchange((long*)Value, (long)Exchange);
	}
#endif
}