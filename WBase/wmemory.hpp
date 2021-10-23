/*! \file lmemory.hpp
\ingroup WBase
\brief LeoEngine专用的内存操作。

*/

#pragma once

#include "memory.hpp"

namespace white
{
	template<typename T, typename U>
	void inline memcpy(T& dst, const U& src)
	{
		constexpr auto size = sizeof(T) < sizeof(U) ? sizeof(T) : sizeof(U);
		std::memcpy(&dst, &src, size);
	}

	template<typename T>
	void inline memset(T& dst, int val)
	{
		std::memset(&dst, val, sizeof(T));
	}

}
