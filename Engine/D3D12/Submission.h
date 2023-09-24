#pragma once

#include "Fence.h"
#include "Utility.h"
#include "Queue.h"
#include "Core/Threading/ManualResetEvent.h"

namespace platform_ex::Windows::D3D12 {
	enum class SyncPointType
	{
		// Sync points of this type do not include an FGraphEvent, so cannot
		// report completion to the CPU (via either IsComplete() or Wait())
		GPUOnly,

		// Sync points of this type include an FGraphEvent. The IsComplete() and Wait() functions
		// can be used to poll for completion from the CPU, or block the CPU, respectively.
		GPUAndCPU,
	};

	struct ResolveFence
	{
		std::shared_ptr<FenceCore> Fence;
		uint64 Value = 0;

		ResolveFence() = default;
		ResolveFence(std::shared_ptr<FenceCore> InFence, uint64 InValue)
			:Fence(InFence), Value(InValue)
		{}
	};

	class SyncPoint;
	class NodeQueue;
	class CommandList;
	class CommandAllocator;

	using SyncPointRef = COMPtr<SyncPoint>;

	using SubmissionEvent = white::threading::manual_reset_event;

	using SubmissionEventRef = std::shared_ptr<SubmissionEvent>;

	class SyncPoint :public RefCountBase
	{
		SyncPoint(SyncPoint const&) = delete;
		SyncPoint(SyncPoint&&) = delete;

		std::optional<ResolveFence> ResolvedFence;
		SubmissionEventRef CompletionEvent;

		SyncPoint(SyncPointType Type)
		{
			if (Type == SyncPointType::GPUAndCPU)
			{
				CompletionEvent.reset();
			}
		}
	public:
		static SyncPointRef Create(SyncPointType Type)
		{
			return new SyncPoint(Type);
		}

		void Wait() const
		{
			CompletionEvent->wait();
		}

		bool IsComplete()
		{
			return CompletionEvent->ready();
		}
	};

	//GPU breadcrumbs for crash debugging
	struct BreadcrumbStack
	{
		struct FScope
		{
			uint32 NameCRC;
			uint32 MarkerIndex;
			uint32 Child;
			uint32 Sibling;
		};

		NodeQueue* Queue = nullptr;
		uint32 NextIdx{ 0 };
		int32 ContextId;
		uint32 MaxMarkers{ 0 };
		D3D12_GPU_VIRTUAL_ADDRESS WriteAddress;
		void* CPUAddress;

		std::vector<FScope> Scopes;
		std::vector<uint32> ScopeStack;
		bool bTopIsOpen{ false };

		BreadcrumbStack();
		~BreadcrumbStack();
	};

	// A single unit of work (specific to a single GPU node and queue type) to be processed by the submission thread.
	struct PayloadBase
	{
		// Used to signal FD3D12ManualFence instances on the submission thread.
		struct FManualFence
		{
			// The D3D fence to signal
			COMPtr<ID3D12Fence> Fence;

			// The value to signal the fence with.
			uint64 Value;

			FManualFence() = default;
			FManualFence(COMPtr<ID3D12Fence>&& Fence, uint64 Value)
				: Fence(Fence)
				, Value(Value)
			{}
		};

		// Constants
		NodeQueue& Queue;

		// Wait
		struct : public std::vector<SyncPointRef>
		{
			// Used to pause / resume iteration of the sync point array on the
			// submission thread when we find a sync point that is unresolved.
			int32 Index = 0;

		} SyncPointsToWait;

		virtual void PreExecute() {}

		// Wait
		std::vector<FManualFence> FencesToWait;

		// Execute
		std::vector<CommandList*> CommandListsToExecute;

		// Signal
		std::vector<FManualFence> FencesToSignal;

		std::vector<SyncPointRef> SyncPointsToSignal;
		uint64 CompletionFenceValue = 0;
		SubmissionEventRef SubmissionEvent;
		std::optional<uint64> SubmissionTime;

		// Flags.
		bool bAlwaysSignal = false;

		// Cleanup
		std::vector<CommandAllocator*> AllocatorsToRelease;

		// GPU crash breadcrumbs stack
		std::vector<std::shared_ptr<BreadcrumbStack>> BreadcrumbStacks;

		virtual ~PayloadBase();

	protected:
		PayloadBase(NodeDevice* Device, QueueType QueueType);
	};

#if WFL_Win64
	struct D3D12Payload final : public PayloadBase
	{
		D3D12Payload(NodeDevice* Device, QueueType QueueType)
			: PayloadBase(Device, QueueType)
		{
		}
	};
#endif
}