#include "VisBuffer.h"
#include "RenderInterface/BuiltinShader.h"

using namespace platform::Render;

class FilterTriangleCS :public BuiltInShader
{
public:
	EXPORTED_BUILTIN_SHADER(FilterTriangleCS);

};

IMPLEMENT_BUILTIN_SHADER(FilterTriangleCS, "FilterTriangle.wsl", "FilterTriangleCS", platform::Render::ComputeShader);


void VisBufferTest::RenderTrinf(CommandList& CmdList)
{

}