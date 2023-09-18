/*! \file Engine\Render\IFormat.hpp
\ingroup Engine
\brief GPU×ÊÔ´View¡£
*/
#ifndef WE_RENDER_IGPUResourceView_h
#define WE_RENDER_IGPUResourceView_h 1

#include "ITexture.hpp"

namespace platform {
	namespace Render {
		class FrameBuffer;
		class GraphicsBuffer;
		class ConstantBuffer;

		class ShaderResourceView :public RObject {
		public:
		};

		class UnorderedAccessView :public RObject
		{
		public:
		};

		using SRVRIRef = std::shared_ptr<platform::Render::ShaderResourceView>;
		using UAVRIRef = std::shared_ptr<platform::Render::UnorderedAccessView>;

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
}

#endif