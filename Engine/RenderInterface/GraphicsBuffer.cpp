#include "IGraphicsBuffer.hpp"
#include "ICommandList.h"

platform::Render::Buffer::Mapper::Mapper(GraphicsBuffer& InBuffer, Access ba)
	:CmdList(platform::Render::CommandListExecutor::GetImmediateCommandList()),buffer(InBuffer)
{
	data = buffer.Map(CmdList, ba);
}

platform::Render::GraphicsBuffer::GraphicsBuffer(Buffer::Usage usage_, white::uint32 access_, white::uint32 size_in_byte_)
	:usage(usage_),access(access_), size_in_byte(size_in_byte_)
{
}

platform::Render::GraphicsBuffer::~GraphicsBuffer() {
}

platform::Render::UnorderedAccessView* platform::Render::BufferViewCache::GetOrCreateUAV(CommandListBase& CmdList, GraphicsBuffer* Buffer, const BufferUAVCreateInfo& CreateInfo)
{
	for (const auto& KeyValue : UAVs)
	{
		if (KeyValue.first == CreateInfo)
		{
			return KeyValue.second.get();
		}
	}

	UAVRIRef View = CmdList.CreateUAV(Buffer, CreateInfo);

	UAVs.emplace_back(CreateInfo, View);

	return View.get();
}

platform::Render::ShaderResourceView* platform::Render::BufferViewCache::GetOrCreateSRV(CommandListBase& CmdList, GraphicsBuffer* Buffer, const BufferSRVCreateInfo& CreateInfo)
{
	for (const auto& KeyValue : SRVs)
	{
		if (KeyValue.first == CreateInfo)
		{
			return KeyValue.second.get();
		}
	}

	SRVRIRef View = CmdList.CreateSRV(Buffer, CreateInfo);

	SRVs.emplace_back(CreateInfo, View);

	return View.get();
}