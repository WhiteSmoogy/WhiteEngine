#pragma once
#include <WBase/wmath.hpp>
#include <WBase/id.hpp>
#include <bit>
#include <functional>

namespace white::math
{
	/**
	 * Converts a float to an integer with truncation towards zero.
	 * @param F		Floating point value to convert
	 * @return		Truncated integer.
	 */
	constexpr inline int32 TruncToInt(float F)
	{
		return (int32)F;
	}

	/**
	 * Converts a float to a nearest less or equal integer.
	 * @param F		Floating point value to convert
	 * @return		An integer less or equal to 'F'.
	 */
	inline int32 FloorToInt(float F)
	{
		return TruncToInt(std::floorf(F));
	}

	/**
	 *	Checks whether a number is a power of two.
	 *	@param Value	Number to check
	 *	@return			true if Value is a power of two
	 */
	template <typename T>
	inline bool IsPowerOfTwo(T Value)
	{
		return ((Value & (Value - 1)) == (T)0);
	}

	template<typename T>
	inline constexpr T RoundUpToPowerOfTwo(T Value)
	{
		return std::bit_ceil<T>(Value);
	}

	template<typename T>
	inline constexpr T  FloorLog2(T Value)
	{
		if (Value == 0)
			return 0;
		
		return std::numeric_limits<T>::digits - std::countl_zero(Value) - 1;
	}

	inline constexpr uint8 CeilLog2(uint64 Value)
	{
		if (Value == 0)
			return 0;

		return static_cast<uint8>(std::bit_width(Value-1));
	}


	/** Multiples value by itself */
	template< class T >
	inline constexpr T Square(const T A)
	{
		return A * A;
	}

	/** Spreads bits to every 3rd. */
	inline uint32 MortonCode3(uint32 x)
	{
		x &= 0x000003ff;
		x = (x ^ (x << 16)) & 0xff0000ff;
		x = (x ^ (x << 8)) & 0x0300f00f;
		x = (x ^ (x << 4)) & 0x030c30c3;
		x = (x ^ (x << 2)) & 0x09249249;
		return x;
	}

	template <class T>
	inline constexpr T DivideAndRoundUp(T Dividend, T Divisor)
	{
		return (Dividend + Divisor - 1) / Divisor;
	}

	/** Divides two integers and rounds to nearest */
	template <class T>
	static T DivideAndRoundNearest(T Dividend, T Divisor)
	{
		return (Dividend >= 0)
			? (Dividend + Divisor / 2) / Divisor
			: (Dividend - Divisor / 2 + 1) / Divisor;
	}

	/**
	* Converts a float to the nearest greater or equal integer.
	* @param F		Floating point value to convert
	* @return		An integer greater or equal to 'F'.
	*/
	inline int32 CeilToInt(float F)
	{
		return TruncToInt(std::ceilf(F));
	}
}

namespace white::math
{
	constexpr float MAX_flt = (3.402823466e+38F);
	constexpr uint32 MAX_uint32 = 0xffffffff;
	constexpr int32 MIN_int32 = std::numeric_limits<int32>::min();
	constexpr int32 MAX_int32 = std::numeric_limits<int32>::max();

	constexpr float3 UpVector = { 0,1,0 };
}

namespace std
{
	template<typename type>
	requires white::math::is_wmathtype_v<type>
	struct hash<type>
	{
		constexpr size_t operator()(const type& val) const wnoexcept
		{
			return white::hash(0,val.begin(),val.end());
		}
	};
}