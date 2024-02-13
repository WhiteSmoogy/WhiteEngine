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
		return Index;
	}

	void Deallocate(int32 Index)
	{
		std::unique_lock lock{ Mutex };
		FreeIndexList.emplace_back(Index);
		ResourceList[Index] = nullptr;
	}

	void Clear()
	{
		std::unique_lock lock{ Mutex };
		FreeIndexList.clear();
		ResourceList.clear();
	}

	template<typename FunctionType>
	void ForEach(const FunctionType& Function)
	{
		std::unique_lock lock{ Mutex };
		for (int32 Index = 0; Index < ResourceList.size(); ++Index)
		{
			auto* Resource = ResourceList[Index];
			if (Resource)
			{
				Function(Resource);
			}
		}
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

	auto& ResourceList = RenderResourceList::Get();

	const auto Lambda = [&](RenderResource* Resource) { Resource->InitResource(CmdList); };

	ResourceList.ForEach(Lambda);
}

void RenderResource::InitResource(CommandListBase& CmdList)
{
	if (ListIndex == white::INDEX_NONE)
	{
		int LocalListIndex = white::INDEX_NONE;

		if (!Caps.IsInitialized)
		{
			LocalListIndex = RenderResourceList::Get().Allocate(this);
		}
		else
		{
			LocalListIndex = 0;
		}

		if (Caps.IsInitialized)
		{
			InitRenderResource(CmdList);
		}

		ListIndex = LocalListIndex;
	}
}


void RenderResource::ReleaseResource()
{
	if (ListIndex != white::INDEX_NONE)
	{
		if (Caps.IsInitialized)
		{
			ReleaseRenderResource();
		}

		RenderResourceList::Get().Deallocate(ListIndex);

		ListIndex = white::INDEX_NONE;
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
	CL_ALLOC_COMMAND(RenderResource::GetCommandList(), CommandRenderResource)(Resource);
}
