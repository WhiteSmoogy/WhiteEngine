#ifndef IndePlatform_Abstract_hpp
#define IndePlatform_Abstract_hpp

#include "wdef.h"

namespace white
{
	namespace details
	{
		class AbstractBase {
		protected:
			virtual ~AbstractBase() wnothrow = default;

		private:
			virtual void WHITE_PureAbstract_() wnothrow = 0;
		};

		template<typename RealBase>
		class ConcreteBase : public RealBase {
		protected:
			template<typename ...BaseParams>
			explicit ConcreteBase(BaseParams &&...vBaseParams)
				wnoexcept(std::is_nothrow_constructible<RealBase, BaseParams &&...>::value)
				: RealBase(std::forward<BaseParams>(vBaseParams)...)
			{
			}

		private:
			void WHITE_PureAbstract_() wnothrow override{
			}
		};

	}
}

#define ABSTRACT					private ::white::details::AbstractBase
#define CONCRETE(base)				public ::white::details::ConcreteBase<base>

#define CONCRETE_INIT(base, ...)	::white::details::ConcreteBase<base>(__VA_ARGS__)


#endif