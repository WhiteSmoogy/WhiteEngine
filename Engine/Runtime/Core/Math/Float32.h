#pragma once
#include <WFramework/WCLib/Platform.h>
#include <WBase/winttype.hpp>
namespace white
{
	/**
	* 32 bit float components
	*/
	class FFloat32
	{
	public:

		union
		{
			struct
			{
#if WFL_Win32
				uint32	Mantissa : 23;
				uint32	Exponent : 8;
				uint32	Sign : 1;
#else
				uint32	Sign : 1;
				uint32	Exponent : 8;
				uint32	Mantissa : 23;
#endif
			} Components;

			float	FloatValue;
		};

		/**
		 * Constructor
		 *
		 * @param InValue value of the float.
		 */
		FFloat32(float InValue = 0.0f)
			:FloatValue(InValue)
		{}
	};
}