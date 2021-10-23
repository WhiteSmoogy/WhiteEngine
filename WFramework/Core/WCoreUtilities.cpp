#include "WCoreUtilities.h"
#include <cstdlib>

using namespace white;

void
ArgumentsVector::Reset(int argc, char* argv[])
{
	Arguments.clear();
	Arguments.reserve(CheckNonnegative<size_t>(argc, "argument number"));
	for (size_t i(0); i < size_t(argc); ++i)
		Arguments.push_back(Nonnull(argv[i]));
}