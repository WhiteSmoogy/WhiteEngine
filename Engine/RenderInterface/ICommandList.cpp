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
	bExecuting = false;

	MemManager.Flush();

	Root = nullptr;
	CommandLink = &Root;
}

void platform::Render::CommandList::BeginFrame()
{
	InsertCommand([=](CommandListBase& CmdList) {
		CmdList.GetContext().BeginFrame();
	});
}

void platform::Render::CommandList::EndFrame()
{
	InsertCommand([=](CommandListBase& CmdList) {
		CmdList.GetContext().EndFrame();
	});
	GRenderIF->AdvanceFrameFence();

	CommandListExecutor::GetImmediateCommandList().ImmediateFlush();
}

void platform::Render::CommandList::Present(Display* display)
{
	InsertCommand([=](CommandListBase& CmdList) {
		display->SwapBuffers();
	});

	//get event

	CommandListExecutor::GetImmediateCommandList().ImmediateFlush();

	//wait prev

	GRenderIF->AdvanceDisplayBuffer();
}

SRVRIRef platform::Render::CommandList::CreateShaderResourceView(const platform::Render::GraphicsBuffer* InBuffer, EFormat format)
{
	return GRenderIF->GetDevice().CreateShaderResourceView(InBuffer, format);
}
UAVRIRef platform::Render::CommandList::CreateUnorderedAccessView(const platform::Render::GraphicsBuffer* InBuffer, EFormat format)
{
	return GRenderIF->GetDevice().CreateUnorderedAccessView(InBuffer, format);
}
