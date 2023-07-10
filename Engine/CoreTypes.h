#pragma once

#include <WBase/winttype.hpp>
#include "RenderInterface/IFormat.hpp"
#include "Runtime/Core/Math/PlatformMath.h"
#include "emacro.h"

#include <format>

namespace wm = white::math;

namespace WhiteEngine
{
	using namespace platform::Render::IFormat;
	using namespace white::inttype;
}

using namespace white::inttype;

template<class CharT>
struct std::formatter<WhiteEngine::EFormat, CharT> :std::formatter<std::underlying_type_t<WhiteEngine::EFormat>, CharT> {
	using base = std::formatter<std::underlying_type_t<WhiteEngine::EFormat>, CharT>;
	template<class FormatContext>
	auto format(WhiteEngine::EFormat format, FormatContext& fc) {
		return base::format((uint64)(format), fc);
	}
};

