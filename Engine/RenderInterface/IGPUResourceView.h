/*! \file Engine\Render\IFormat.hpp
\ingroup Engine
\brief GPU×ÊÔ´View¡£
*/
#ifndef WE_RENDER_IGPUResourceView_h
#define WE_RENDER_IGPUResourceView_h 1

#include "RenderObject.h"
#include "WBase/cassert.h"

namespace platform::Render {

	class ShaderResourceView :public RObject {
	public:
	};

	class UnorderedAccessView :public RObject
	{
	public:
	};

	using SRVRIRef = white::ref_ptr<ShaderResourceView, RObjectController>;
	using UAVRIRef = white::ref_ptr<UnorderedAccessView, RObjectController>;

	template<typename Type>
	struct Range
	{
		Type First = 0;
		Type Num = 0;

		Range() = default;

		template<std::integral T>
		Range(T InFirst, T InNum)
			:First(InFirst), Num(InNum)
		{
			wassume(InFirst < std::numeric_limits<Type>::max()
				&& InNum < std::numeric_limits<Type>::max()
				&& (InFirst + InNum) < std::numeric_limits<Type>::max());
		}

		Type ExclusiveLast() const { return First + Num; }
		Type InclusiveLast() const { return First + Num - 1; }

		bool IsInRange(std::integral auto Value) const
		{
			wassume(Value < std::numeric_limits<Type>::max());
			return Value >= First && Value < ExclusiveLast();
		}

	};
}

#endif