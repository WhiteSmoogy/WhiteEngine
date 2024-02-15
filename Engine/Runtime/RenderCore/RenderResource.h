#pragma once
#include <concepts>
#include "RenderInterface/ICommandList.h"
#include "Dispatch.h"
#include "Core/Container/vector.hpp"

namespace platform::Render
{
	class RenderResource
	{
	public:
		~RenderResource() = default;

		virtual void InitRenderResource(CommandListBase& CmdList) {}
		virtual void ReleaseRenderResource() {}

		void InitResource(CommandListBase& CmdList);
		void ReleaseResource();

		static CommandList& GetCommandList();
		static void InitResources();

		std::atomic<int32> ListIndex = white::INDEX_NONE;
	};

	void BeginInitResource(RenderResource* Resource);

	template<class ResourceType> requires std::derived_from< ResourceType, RenderResource>
	class GlobalResource : public ResourceType
	{
	public:
		GlobalResource()
		{
			InitGlobalResource();
		}

		template<typename... Args>
		explicit GlobalResource(Args... InArgs)
			: ResourceType(InArgs...)
		{
			InitGlobalResource();
		}

	private:
		void InitGlobalResource()
		{
			if (IsInRenderCommandThread())
			{
				// If the resource is constructed in the rendering thread, directly initialize it.
				((ResourceType*)this)->InitResource(RenderResource::GetCommandList());
			}
			else
			{
				// If the resource is constructed outside of the rendering thread, enqueue a command to initialize it.
				BeginInitResource((ResourceType*)this);
			}
		}

		void ReleaseGlobalResource()
		{
			((ResourceType*)this)->ReleaseResource();
		}
	};
}