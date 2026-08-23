#include "RenderResource.h"
#include <mutex>
using namespace platform::Render;

class RenderResourceList
{
public:
	static RenderResourceList& Get()
	{
		static RenderResourceList Instance;
		return Instance;
	}

	int32 Allocate(RenderResource* Resource)
	{
		std::unique_lock lock{ Mutex };
		if (Resource->ListIndex != white::INDEX_NONE)
		{
			return Resource->ListIndex;
		}

		int32 Index;
		if (FreeIndexList.empty())
		{
			Index = static_cast<int32>(ResourceList.size());
			ResourceList.emplace_back(Resource);
		}
		else
		{
			Index = white::pop(FreeIndexList);
			ResourceList[Index] = Resource;
		}
		Resource->ListIndex = Index;
		return Index;
	}

	void Deallocate(int32 Index, RenderResource* Resource)
	{
		std::unique_lock lock{ Mutex };
		if (Index < 0 || Index >= ResourceList.size() || ResourceList[Index] != Resource)
		{
			return;
		}

		FreeIndexList.emplace_back(Index);
		ResourceList[Index] = nullptr;
	}

	void Clear()
	{
		std::unique_lock lock{ Mutex };
		FreeIndexList.clear();
		ResourceList.clear();
	}

	std::vector<RenderResource*> Snapshot()
	{
		std::unique_lock lock{ Mutex };
		return ResourceList;
	}

	template<typename FunctionType>
	void ForEachReverse(const FunctionType& Function)
	{
		std::unique_lock lock{ Mutex };
		for (int32 Index = static_cast<int32>(ResourceList.size()) - 1; Index >= 0; --Index)
		{
			auto* Resource = ResourceList[Index];
			if (Resource)
			{
				Function(Resource);
			}
		}
	}

private:
	std::mutex Mutex;

	std::vector<int32> FreeIndexList;
	std::vector<RenderResource*> ResourceList;
};

void RenderResource::InitResources()
{
	auto& CmdList = GetCommandList();

	for (RenderResource* Resource : RenderResourceList::Get().Snapshot())
	{
		if (Resource)
		{
			Resource->InitResource(CmdList);
		}
	}
}

void RenderResource::RegisterResource()
{
	if (ListIndex == white::INDEX_NONE)
	{
		RenderResourceList::Get().Allocate(this);
	}
}

void RenderResource::InitResource(CommandListBase& CmdList)
{
	RegisterResource();

	bool Expected = false;
	if (Caps.IsInitialized && Initialized.compare_exchange_strong(Expected, true))
	{
		try
		{
			InitRenderResource(CmdList);
		}
		catch (...)
		{
			Initialized = false;
			throw;
		}
	}
}

void RenderResource::ReleaseResource()
{
	if (Initialized.exchange(false))
	{
		ReleaseRenderResource();
	}

	const int32 LocalListIndex = ListIndex.exchange(white::INDEX_NONE);
	if (LocalListIndex != white::INDEX_NONE)
	{
		RenderResourceList::Get().Deallocate(LocalListIndex, this);
	}
}

CommandList& RenderResource::GetCommandList()
{
	return CommandListExecutor::GetImmediateCommandList();
}

struct CommandRenderResource final : platform::Render::CommandBase
{
	RenderResource* Resource;

	CommandRenderResource(RenderResource* InResource)
		:Resource(InResource)
	{}

	void ExecuteAndDestruct(platform::Render::CommandListBase& CmdList, platform::Render::CommandListContext& Context) override
	{
		Resource->InitResource(CmdList);
	}
};

void platform::Render::BeginInitResource(RenderResource* Resource)
{
	Resource->RegisterResource();
	if (!Caps.IsInitialized)
	{
		return;
	}

	CL_ALLOC_COMMAND(RenderResource::GetCommandList(), CommandRenderResource)(Resource);
}
