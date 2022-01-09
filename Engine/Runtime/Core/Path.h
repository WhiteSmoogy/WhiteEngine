#pragma once

#include <WBase/wdef.h>
#include <filesystem>

namespace WhiteEngine
{
	namespace fs = std::filesystem;

	using path = fs::path;

	class PathSet
	{
	public:
		static path WorkingRoot();

		static path EngineDir();

		static path EngineIntermediateDir();
	};
}