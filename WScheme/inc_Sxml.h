#ifndef WScheme_Inc_SXML_h
#define WScheme_Inc_SXML_h 1

#include "sdef.h"
#include <WFramework/Adaptor/WAdaptor.h>

namespace scheme {
	struct WS_API WTag {};

	using white::byte;

	namespace sxml {
		enum class ParseOption
		{
			Normal,
			Strict,
			String,
			Attribute
		};
	}
}

#endif
