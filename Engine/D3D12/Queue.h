#pragma once

#include "d3d12_dxgi.h"

namespace platform_ex::Windows::D3D12 {
	enum class QueueType
	{
		Direct = 0,
		Copy,
		Async,

		Count,
	};

	static constexpr uint32 GD3D12MaxNumQueues = MAX_NUM_GPUS * (uint32)QueueType::Count;

	inline const char* GetD3DCommandQueueTypeName(QueueType QueueType)
	{
		switch (QueueType)
		{
		default: throw white::unsupported(); // fallthrough
		case QueueType::Direct: return "3D";
		case QueueType::Async:  return "Compute";
		case QueueType::Copy:   return "Copy";
		}
	}

	inline D3D12_COMMAND_LIST_TYPE GetD3DCommandListType(QueueType QueueType)
	{
		switch (QueueType)
		{
		default: throw white::unsupported(); // fallthrough
		case QueueType::Direct: return D3D12_COMMAND_LIST_TYPE_DIRECT;
		case QueueType::Copy:   return D3D12_COMMAND_LIST_TYPE_COPY;
		case QueueType::Async:  return D3D12_COMMAND_LIST_TYPE_COMPUTE;
		}
	}

	class NodeQueue
	{

	};

}