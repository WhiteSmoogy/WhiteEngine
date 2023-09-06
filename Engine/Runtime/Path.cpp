#include "Path.h"
#include <WFramework/Win32/WCLib/Mingw32.h>
using namespace WhiteEngine;

struct PathSetStaticData
{
	path WorkingRoot;
	path EngineRoot;
	path EngineDirectory;

	PathSetStaticData()
	{
		WorkingRoot = platform_ex::Windows::FetchModuleFileName();
		WorkingRoot.remove_filename();

		EngineRoot = (WorkingRoot / ".." / "..").lexically_normal();

		EngineDirectory = EngineRoot / "Engine";
	}
};

const PathSetStaticData& GetStaticData()
{
	static PathSetStaticData  StaticData {};
	return StaticData;
}

path WhiteEngine::PathSet::WorkingRoot()
{
	return GetStaticData().WorkingRoot;
}

path WhiteEngine::PathSet::EngineDir()
{
	return GetStaticData().EngineDirectory;

}

struct PathSetStaticDataEx
{
	PathSetStaticDataEx(const path& engineroot)
	{
		IntermediateDirectory = engineroot / "Intermediate";
		fs::create_directories(IntermediateDirectory);
	}

	path IntermediateDirectory;
};

PathSetStaticDataEx& GetStaticDataEx()
{
	static PathSetStaticDataEx StaticDataEx{ GetStaticData().EngineRoot };
	return StaticDataEx;
}

path WhiteEngine::PathSet::EngineIntermediateDir()
{
	return GetStaticDataEx().IntermediateDirectory;
}
