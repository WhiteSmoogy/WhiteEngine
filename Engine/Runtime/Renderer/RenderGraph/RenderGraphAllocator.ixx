module;
#include "Runtime/MemStack.h"

export module Render;

class RGAllocator
{

};

export namespace RenderGraph
{
	RGAllocator& GetAllocator()
	{
		static RGAllocator Instance;
		return Instance;
	}
}

