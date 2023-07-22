#pragma once

#include <WBase/winttype.hpp>
#include <filesystem>

namespace asset {
	using path = std::filesystem::path;

	template<unsigned char c0, unsigned char c1, unsigned char c2, unsigned char c3>
	struct four_cc {
		enum { value = (c0 << 0) + (c1 << 8) + (c2 << 16) + (c3 << 24) };
	};

	template<unsigned char c0, unsigned char c1, unsigned char c2, unsigned char c3>
	wconstexpr white::uint32 four_cc_v = four_cc<c0, c1, c2, c3>::value;
}