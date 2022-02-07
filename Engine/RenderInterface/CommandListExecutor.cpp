#include "ICommandList.h"
#include "Runtime/Core/Coroutine/ThreadScheduler.h"
#include "System/SystemEnvironment.h"
#include <coroutine>

class render_task;

using schedule_operation = white::coroutine::ThreadScheduler::schedule_operation;

class render_promise :public schedule_operation
{
public:
	render_promise()
		:schedule_operation(Environment->Scheduler->schedule_render())
	{}

	auto initial_suspend() noexcept
	{
		return *this;
	}

	auto final_suspend() noexcept
	{
		return std::suspend_never{};
	}

	render_task get_return_object() noexcept;

	void unhandled_exception() {
		exception = std::current_exception();
	}

	void return_void() noexcept
	{
		if (exception)
		{
			std::rethrow_exception(exception);
		}
	}
private:
	std::exception_ptr exception;
};

class render_task
{
public:
	using promise_type = render_promise;

	explicit render_task(std::coroutine_handle<promise_type> coroutine)
		: coroutine_handle(coroutine)
	{}

	explicit render_task(std::nullptr_t)
		:coroutine_handle(nullptr)
	{}

	render_task(render_task&& t) noexcept
		: coroutine_handle(t.coroutine_handle)
	{
		t.coroutine_handle = nullptr;
	}

	~render_task()
	{
		if (coroutine_handle)
		{
			coroutine_handle.destroy();
		}
		coroutine_handle = nullptr;
	}

	bool is_ready() const noexcept
	{
		return !coroutine_handle || coroutine_handle.done();
	}

	void swap(render_task&& t) noexcept
	{
		std::swap(coroutine_handle, t.coroutine_handle);
	}

	void assign(render_task&& t) noexcept
	{
		this->~render_task();
		swap(std::move(t));
	}

private:
	std::coroutine_handle<promise_type> coroutine_handle;
};

render_task render_promise::get_return_object() noexcept
{
	return render_task{ std::coroutine_handle<render_promise>::from_promise(*this) };
}

using namespace platform::Render;

class RenderTaskArray
{
public:
	RenderTaskArray()
		:tasks{ render_task(nullptr),render_task(nullptr),render_task(nullptr) }
	{}

	void assign(render_task&& t)
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

	render_task tasks[3];
};

RenderTaskArray SwapTasks;
render_task CommandTask { nullptr };

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
		CommandTask.swap([](CommandListBase* cmdlist)->render_task
		{
			Execute(*cmdlist);

			delete cmdlist;

			co_return;
		}(SwapCmdList));
		return;
	}

	Execute(CmdList);
}

void CommandListExecutor::Execute(CommandListBase& CmdList)
{
	CommandListContext Context;
	CommandListIterator Iter(CmdList);

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