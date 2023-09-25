#pragma once

#include "d3d12_dxgi.h"
#include "Core/Container/vector.hpp"

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

	class NodeDevice;
	struct D3D12Payload;
	class ContextCommon;
	class CommandAllocator;
	class CommandList;

	template<typename T>
	struct D3DObjectPool
	{
		T* Pop()
		{
			std::unique_lock lock{ Mutex };

			if (Pools.empty())
				return nullptr;

			auto Result = Pools.front();
			Pools.pop_front();

			return Result;
		}

		void Push(T* Object)
		{
			std::unique_lock lock{ Mutex };
			Pools.emplace_back(Object);
		}

		D3DObjectPool(D3DObjectPool&& Pool)
			:Mutex(),Pools(std::move(Pool.Pools))
		{
		}

		~D3DObjectPool()
		{
		}

		D3DObjectPool()
		{}

		std::mutex Mutex;
		std::list<T*> Pools;
	};

	class NodeQueue final
	{
	public:
		NodeDevice* const Device;
		QueueType const Type;

		struct PayloadQueue : protected std::queue<D3D12Payload*>
		{
			D3D12Payload* Peek()
			{
				std::unique_lock lock{ Mutex };

				if (!empty())
				{
					auto Result = front();

					return Result;
				}

				return nullptr;
			}

			void Enqueue(D3D12Payload* Payload)
			{
				std::unique_lock lock{ Mutex };

				push(Payload);
			}

			bool Pop()
			{
				std::unique_lock lock{ Mutex };

				if (!empty())
				{
					pop();

					return true;
				}

				return false;
			}

			PayloadQueue(PayloadQueue&& Queue) noexcept
				:std::queue<D3D12Payload*>(Queue), Mutex()
			{
			}

			PayloadQueue() = default;

			std::mutex Mutex;
		} PendingSubmission, PendingInterrupt;

		D3D12Payload* PayloadToSubmit = nullptr;
		CommandAllocator* BarrierAllocator = nullptr;

		uint32 NumCommandListsInBatch = 0;

		// Executes the current payload, returning the latest fence value signaled for this queue.
		uint64 ExecutePayload();

		bool bRequiresSignal = false;

		COMPtr<ID3D12CommandQueue> D3DCommandQueue;

		FenceCore Fence;

		// Tracks what fence values this queue has awaited on other queues.
		struct RemoteFenceState
		{
			uint64 MaxValueAwaited = 0;
			uint64 NextValueToAwait = 0;
		};
		std::unordered_map<FenceCore*, RemoteFenceState> RemoteFenceStates;

		uint64 SignalFence()
		{
			if (bRequiresSignal)
			{
				bRequiresSignal = false;
				uint64 ValueToSignal = ++Fence.FenceValueAvailableAt;
				CheckHResult(D3DCommandQueue->Signal(
					Fence.GetFence(),
					ValueToSignal
				));

				return ValueToSignal;
			}
			else
			{
				return Fence.FenceValueAvailableAt;
			}
		}

		std::vector<FenceCore*> FencesToAwait;
		void EnqueueFenceWait(FenceCore* RemoteFence, uint64 Value)
		{
			uint64& NextValueToAwait = RemoteFenceStates[RemoteFence].NextValueToAwait;
			NextValueToAwait = std::max(NextValueToAwait, Value);
			white::add_unique(FencesToAwait, RemoteFence);
		}

		void FlushFenceWaits()
		{
			for (auto* FenceToAwait : FencesToAwait)
			{
				wassume(RemoteFenceStates.count(FenceToAwait) != 0);
				auto& RemoteFenceState = RemoteFenceStates[FenceToAwait];

				// Skip issuing the fence wait if we've previously awaited the same fence with a higher value.
				if (RemoteFenceState.NextValueToAwait > RemoteFenceState.MaxValueAwaited)
				{
					CheckHResult(D3DCommandQueue->Wait(
						FenceToAwait->GetFence(),
						RemoteFenceState.NextValueToAwait
					));

					RemoteFenceState.MaxValueAwaited = std::max(
						RemoteFenceState.MaxValueAwaited,
						RemoteFenceState.NextValueToAwait
					);
				}
			}

			FencesToAwait.clear();
		}

		// A pool of reusable command list/allocator/context objects
		struct
		{
			D3DObjectPool<ContextCommon> Contexts;
			D3DObjectPool<CommandAllocator> Allocators;
			D3DObjectPool<CommandList> Lists;
		} ObjectPool;

		uint64 CumulativeIdleTicks = 0;
		uint64 LastEndTime = 0;

		NodeQueue(NodeDevice* Device, QueueType QueueType);
		~NodeQueue();

		NodeQueue(NodeQueue&&) = default;

		void SetupAfterDeviceCreation();
	};

}