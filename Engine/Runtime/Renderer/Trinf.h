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

		uint64 StreamingKey = -1;
		StreamingState State = StreamingState::None;

		std::shared_ptr<platform_ex::DStorageFile> File;
		std::shared_ptr<platform_ex::DStorageSyncPoint> IORequest;

		platform_ex::DSFileFormat::TrinfHeader Header;
		platform_ex::MemoryRegion<platform_ex::DSFileFormat::TrinfGridHeader> Metadata;
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
