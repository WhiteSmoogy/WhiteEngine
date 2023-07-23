#pragma once

#include "CoreTypes.h"
#include "Asset/TrinfAsset.h"
#include "RenderInterface/ICommandList.h"

namespace Trinf
{
	class Resources
	{
	public:
		enum class StreamingState
		{
			None,
			Streaming,
			Resident,
		};

		uint64 StreamingKey;
		StreamingState State;

		platform_ex::DSFileFormat::TrinfHeader Header;
	};

	class StreamingScene
	{
	public:
		void Init();

		void AddResource(std::shared_ptr<Resources> pResource);

		void BeginAsyncUpdate(platform::Render::CommandList& cmdList);

		void EndAsyncUpdate(platform::Render::CommandList& cmdList);
	private:

	};

	extern std::unique_ptr<StreamingScene> Scene;
}
