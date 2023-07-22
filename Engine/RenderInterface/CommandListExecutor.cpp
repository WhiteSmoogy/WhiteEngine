#include "ICommandList.h"
#include "Core/Coroutine/ThreadScheduler.h"
#include "System/SystemEnvironment.h"
#include <spdlog/spdlog.h>
#include <coroutine>

extern thread_local white::coroutine::ThreadScheduler* thread_local_scheduler;
namespace white::threading {
	void SetThreadDescription(void* hThread, const wchar_t* lpThreadDescription);
}
namespace white::coroutine
{
	class RenderThreadScheduler::thread_state
	{
		alignas(std::hardware_destructive_interference_size) std::atomic<schedule_operation*> queue_tail;
		alignas(std::hardware_destructive_interference_size) std::atomic< schedule_operation*> queue_head;

	public:
		void push(schedule_operation* operation)
		{
			auto tail = queue_tail.load(std::memory_order_relaxed);

			do
			{
				operation->next_oper = tail;
			} while (!queue_tail.compare_exchange_weak(tail, operation, std::memory_order_seq_cst,
				std::memory_order_relaxed));
		}

		schedule_operation* pop()
		{
			auto head = queue_head.load(std::memory_order_relaxed);

			if (head == nullptr)
			{

				if (queue_tail.load(std::memory_order_seq_cst) == nullptr)
					return nullptr;

				auto tail = queue_tail.exchange(nullptr, std::memory_order_acquire);

				if (tail == nullptr)
					return nullptr;

				do
				{
					auto next = std::exchange(tail->next_oper, head);
					head = std::exchange(tail, next);
				} while (tail != nullptr);
			}

			queue_head = head->next_oper;

			return head;
		}
	};

	RenderThreadScheduler::RenderThreadScheduler()
		:current_state(new thread_state())
	{
		std::thread fire_forget(
			[this] {
				thread_local_scheduler = this;
				this->run();
			}
		);
		native_handle = fire_forget.native_handle();

		white::threading::SetThreadDescription(native_handle, L"Scheduler Render");

		fire_forget.detach();
	}

	bool RenderThreadScheduler::schedule_impl(schedule_operation* operation) noexcept
	{
		current_state->push(operation);
		return true;
	}

	void RenderThreadScheduler::run() noexcept
	{
		while (true)
		{
			auto operation = current_state->pop();

			if (operation == nullptr)
				continue;

			operation->continuation_handle.resume();
		}
	}
}

using namespace platform::Render;

SyncPoint::~SyncPoint() = default;

class RenderTaskArray
{
public:
	RenderTaskArray()
		:tasks{ RenderTask(nullptr),RenderTask(nullptr),RenderTask(nullptr) }
	{}

	void assign(RenderTask&& t)
	{
		for (size_t i = 0; i != white::arrlen(tasks); ++i)
		{
			if (tasks[i].is_ready())
			{
				tasks[i].assign(std::move(t));
				return;
			}
		}

		WAssert(false, "out of max render_tasks");
	}

	RenderTask tasks[3];
};

RenderTaskArray SwapTasks;
RenderTask CommandTask { nullptr };

void CommandListExecutor::ExecuteList(CommandListBase& CmdList)
{
	ExecuteInner(CmdList);
}

void CommandListExecutor::ExecuteInner(CommandListBase& CmdList)
{
	if (IsRunningCommandInThread())
	{
		CommandListBase* SwapCmdList = new CommandList();

		static_assert(sizeof(CommandList) == sizeof(CommandListImmediate));

		SwapCmdList->ExchangeCmdList(CmdList);
		CmdList.CopyContext(*SwapCmdList);
		CmdList.PSOContext = SwapCmdList->PSOContext;

		SwapTasks.assign(std::move(CommandTask));
		CommandTask.swap([](CommandListBase* cmdlist)->RenderTask
		{
			Execute(*cmdlist);

			delete cmdlist;

			co_return;
		}(SwapCmdList));
		return;
	}

	Execute(CmdList);
}

#ifndef NDEBUG
class CommandListDebugIterator :public CommandListIterator
{
public:
	CommandListDebugIterator(CommandListBase& CmdList)
		:CommandListIterator(CmdList)
	{
	}

	CommandListIterator& operator++()
	{
		PrevCmdPtr = CmdPtr;

		CommandListIterator::operator++();

		return *this;
	}
	
private:
	CommandBase* PrevCmdPtr;
};
#else
using CommandListDebugIterator = CommandListIterator;
#endif

void CommandListExecutor::Execute(CommandListBase& CmdList)
{
	CmdList.bExecuting = true;

	CommandListContext Context;
	CommandListDebugIterator Iter(CmdList);

	while (Iter)
	{
		Iter->ExecuteAndDestruct(CmdList, Context);
		++Iter;
	}

	CmdList.Reset();
}

void CommandListBase::Flush()
{
	GCommandList.ExecuteList(*this);
}

void CommandListImmediate::ImmediateFlush()
{
	GCommandList.ExecuteList(*this);
}