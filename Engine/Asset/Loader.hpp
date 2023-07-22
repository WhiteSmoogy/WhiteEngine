/*! \file Engine\Asset\Loader.h
\ingroup Engine
\brief Asset Base Interface ...
*/
#ifndef WE_ASSET_LOADER_HPP
#define WE_ASSET_LOADER_HPP 1

#include "emacro.h"
#include <WBase/memory.hpp>
#include <WBase/winttype.hpp>

#include "Core/Coroutine/Task.h"
#include "FileFormat.h"

#include <coroutine>
#include <filesystem>

namespace asset {
	using path = std::filesystem::path;

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