#include "ICommandList.h"

#include "IContext.h"
#include "Core/Threading/TaskScheduler.h"
#include <utility>
#include <mutex>

using namespace platform::Render;

bool platform::Render::GRenderInterfaceSupportCommandThread = true;

CommandListExecutor platform::Render::GCommandList;

#define ALLOC_COMMAND(...) new ( AllocCommand(sizeof(__VA_ARGS__), alignof(__VA_ARGS__)) ) __VA_ARGS__


platform::Render::CommandListBase::CommandListBase()
	:Context(nullptr),MemManager()
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

static CommandListImmediate::RIFenceTypeRef PresentFences[2];
static int PresentFenceIndex = 0;

void platform::Render::CommandList::Present(Display* display)
{
	InsertCommand([=](CommandListBase& CmdList) {
		display->SwapBuffers();
	});

	//get event
	if (IsRunningCommandInThread())
	{
		PresentFences[PresentFenceIndex] = static_cast<CommandListImmediate*>(this)->ThreadFence();
	}

	CommandListExecutor::GetImmediateCommandList().ImmediateFlush();

	//wait prev
	if (IsRunningCommandInThread())
	{
		uint32 PreviousFrameFenceIndex = 1 - PresentFenceIndex;
		auto& LastFrameFence = PresentFences[PreviousFrameFenceIndex];
		if(LastFrameFence)
			LastFrameFence->wait();
		PresentFences[PreviousFrameFenceIndex] = nullptr;
		PresentFenceIndex = PreviousFrameFenceIndex;
	}

	GRenderIF->AdvanceDisplayBuffer();
}


UnorderedAccessView* CommandListBase::CreateUAV(GraphicsBuffer* Buffer, const BufferUAVCreateInfo& CreateInfo)
{
	return GDevice->CreateUnorderedAccessView(Buffer, CreateInfo);
}

// Finds a SRV matching the descriptor in the cache or creates a new one and updates the cache.
ShaderResourceView* CommandListBase::CreateSRV(GraphicsBuffer* Buffer, const BufferSRVCreateInfo& CreateInfo)
{
	return GDevice->CreateShaderResourceView(Buffer, CreateInfo);
}

ConstantBuffer* CommandListBase::CreateConstantBuffer(uint32 size_in_byte, const void* init_data, const char* Name, Buffer::Usage usage)
{
	return GDevice->CreateConstantBuffer(size_in_byte, init_data, Name, usage);
}


using RIFenceTypeRef = std::shared_ptr<CommandListImmediate::RIFenceType>;

struct ThreadFenceCommand :TCommand< ThreadFenceCommand>
{
	RIFenceTypeRef Fence;

	ThreadFenceCommand()
		:Fence(std::make_shared<CommandListImmediate::RIFenceType>())
	{
	}

	void Execute(CommandListBase& CmdList)
	{
		Fence->set();
		Fence.reset();
	}
};

CommandListImmediate::RIFenceTypeRef platform::Render::CommandListImmediate::ThreadFence()
{
	if (IsRunningCommandInThread())
	{
		auto Cmd = ALLOC_COMMAND(ThreadFenceCommand)();

		return Cmd->Fence;
	}

	return {};
}

bool platform::Render::IsInRenderCommandThread()
{
	return white::has_anyflags(white::threading::ActiveTag, white::threading::TaskTag::RenderThread);
}
