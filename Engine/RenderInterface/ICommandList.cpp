#include "ICommandList.h"

#include "IContext.h"
#include <utility>
#include <mutex>

using namespace platform::Render;

bool platform::Render::GRenderInterfaceSupportCommandThread = true;

CommandListExecutor platform::Render::GCommandList;

platform::Render::CommandListBase::CommandListBase()
	:Context(nullptr),MemManager(0)
{
	Reset();
}

void platform::Render::CommandListBase::Reset()
{
	MemManager.Flush();

	Root = nullptr;
	CommandLink = &Root;
}

void platform::Render::CommandList::BeginFrame()
{
	GetContext().BeginFrame();
}

void platform::Render::CommandList::EndFrame()
{
	Flush();
	GetContext().EndFrame();
	platform::Render::Context::Instance().AdvanceFrameFence();
	RObject::FlushPendingDeletes();
}
