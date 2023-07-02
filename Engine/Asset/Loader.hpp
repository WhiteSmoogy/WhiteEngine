/*! \file Engine\Asset\Loader.h
\ingroup Engine
\brief Asset Base Interface ...
*/
#ifndef WE_ASSET_LOADER_HPP
#define WE_ASSET_LOADER_HPP 1

#include "emacro.h"
#include <WBase/memory.hpp>
#include <WBase/winttype.hpp>

#include "Runtime/Core/Coroutine/Task.h"

#include <coroutine>
#include <filesystem>

namespace asset {
	using path = std::filesystem::path;

	template<unsigned char c0, unsigned char c1, unsigned char c2, unsigned char c3>
	struct four_cc {
		enum { value = (c0 << 0) + (c1 << 8) + (c2 << 16) + (c3 << 24) };
	};

	template<unsigned char c0, unsigned char c1, unsigned char c2, unsigned char c3>
	wconstexpr white::uint32 four_cc_v = four_cc<c0, c1, c2, c3>::value;

	class IAssetLoading {
	public:
		virtual ~IAssetLoading();

		virtual std::size_t Type() const = 0;

		virtual std::size_t Hash() const = 0;

		virtual const path& Path() const = 0;
	};

	template<typename T>
	class AssetLoading :public IAssetLoading
	{
	public:
		using AssetType = T;

		virtual white::coroutine::Task<std::shared_ptr<AssetType>> GetAwaiter() = 0;
	};
}

#endif