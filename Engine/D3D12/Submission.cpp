#include "Submission.h"
#include "NodeQueue.h"
#include "NodeDevice.h"
#include "Context.h"

#define D3D12_PLATFORM_SUPPORTS_BLOCKING_FENCES 1

#define D3D12_USE_SUBMISSION_THREAD (1)
#define D3D12_USE_INTERRUPT_THREAD  (1 && D3D12_PLATFORM_SUPPORTS_BLOCKING_FENCES)

namespace white::threading {
	void SetThreadDescription(void* hThread, const wchar_t* lpThreadDescription);
}

namespace platform_ex::Windows::D3D12 {
	const std::chrono::seconds SubmissionTimeOutInSeconds{ 5 };

	class D3D12Thread
	{
	public:
		typedef Context::ProcessResult(Context::* QueueFunc)();

		static DWORD StartAddress(void* param)
		{
			return reinterpret_cast<D3D12Thread*>(param)->Run();
		}

		D3D12Thread(const wchar_t* Name, Context* Context, QueueFunc InFunc)
			:Ctx(Context),Func(InFunc)
			,Event(CreateEventW(nullptr,false,false ,nullptr))
			,Thread(CreateThread(nullptr,128*1024, StartAddress,this,0,nullptr))
		{
			SetThreadDescription(Thread, Name);
		}

		~D3D12Thread()
		{
			bExit = true;
			SetEvent(Event);

			WaitForSingleObject(Thread, INFINITE);
			CloseHandle(Thread);

			CloseHandle(Event);
		}

		void Kick()
		{
			SetEvent(Event);
		}

		DWORD Run()
		{
			while (!bExit)
			{
				// Process the queue until no more progress is made
				Context::ProcessResult Result;
				do { Result = (Ctx->*Func)(); } while (white::has_anyflags(Result.Status, Context::QueueStatus::Processed));

				WaitForSingleObject(Event, Result.WaitTimeout);
			}

			return 0;
		}

	private:
		Context* Ctx;
		QueueFunc Func;
		bool bExit = false;

	public:
		HANDLE const Event;

		HANDLE Thread;
	};

	void Context::InitializeSubmissionPipe()
	{
#if D3D12_USE_INTERRUPT_THREAD
		InterruptThread = new D3D12Thread(L"D3DInterruptThread", this, &Context::ProcessInterruptQueue);
#endif

#if D3D12_USE_SUBMISSION_THREAD
		SubmissionThread = new D3D12Thread(L"D3DSubmissionThread",this, &Context::ProcessSubmissionQueue);
#endif
	}

	void Context::ShutdownSubmissionPipe()
	{
		delete SubmissionThread;
		delete InterruptThread;

	}

	PayloadBase::PayloadBase(NodeDevice* Device, QueueType QueueType)
		:Queue(Device->GetQueue(QueueType))
	{
	}

	PayloadBase::~PayloadBase()
	{
		for (auto* Allocator : AllocatorsToRelease)
		{
			Queue.Device->ReleaseCommandAllocator(Allocator);
		}
	}

	void SyncPoint::Wait() const
	{
		if (CompletionEvent->ready())
		{
			Context::Instance().ProcessInterruptQueueUntil(CompletionEvent);
		}
	}

	void Context::ProcessInterruptQueueUntil(SubmissionEventRef Event)
	{
		if (InterruptThread)
		{
			if (Event && !Event->ready())
			{
				Event->wait();
			}
		}
		else
		{
			if (!Event || !Event->ready())
			{
			Retry:
				if (InterruptCS.try_lock())
				{
					ProcessResult Result;
					do { Result = ProcessInterruptQueue(); } while (Event ?
						!Event->ready() : white::has_anyflags(Result.Status, QueueStatus::Processed));

					InterruptCS.unlock();
				}
				else if (Event && !Event->ready())
				{
					SwitchToThread();
					goto Retry;
				}
			}
		}
	}
	
	Context::ProcessResult Context::ProcessInterruptQueue()
	{
		ProcessResult Result;
		ForEachQueue([&](NodeQueue& CurrentQueue)
			{
				while (auto Payload = CurrentQueue.PendingInterrupt.Peek())
				{
					// Check for GPU completion
					uint64 CompletedFenceValue = CurrentQueue.Fence.GetFence()->GetCompletedValue();

					if (CompletedFenceValue == UINT64_MAX)
					{
						// If the GPU crashes or hangs, the driver will signal all fences to UINT64_MAX. If we get an error code here, we can't pass it directly to 
						// CheckHResult, because that expects DXGI_ERROR_DEVICE_REMOVED, DXGI_ERROR_DEVICE_RESET etc. and wants to obtain the reason code itself
						// by calling GetDeviceRemovedReason (again).
						HRESULT DeviceRemovedReason = CurrentQueue.Device->GetDevice()->GetDeviceRemovedReason();
						if (DeviceRemovedReason != S_OK)
						{
							spdlog::critical("CurrentQueue.Fence.D3DFence->GetCompletedValue()");
							CheckHResult(DXGI_ERROR_DEVICE_REMOVED);
						}
					}

					if (CompletedFenceValue < Payload->CompletionFenceValue)
					{
						// Command list batch has not yet completed on this queue.
						// Ask the driver to wake this thread again when the required value is reached.
						if (InterruptThread && !CurrentQueue.Fence.bInterruptAwaited)
						{
							CheckHResult(CurrentQueue.Fence.GetFence()->SetEventOnCompletion(Payload->CompletionFenceValue, InterruptThread->Event));
							CurrentQueue.Fence.bInterruptAwaited = true;
						}

						// Skip processing this queue and move on to the next.
						Result.Status |= QueueStatus::Pending;

						// Detect a hung GPU
						if (Payload->SubmissionTime.has_value())
						{
							static const uint64 TimeoutCycles = std::chrono::duration_cast<std::chrono::nanoseconds>(SubmissionTimeOutInSeconds).count();

							uint64 ElapsedCycles = platform::GetHighResolutionTicks() - Payload->SubmissionTime.value();

							if (ElapsedCycles > TimeoutCycles)
							{
								// The last submission on this pipe did not complete within the timeout period. Assume the GPU has hung.
								HandleGpuTimeout(Payload, white::Timers::HighTicksToSecond<double>(ElapsedCycles));
								Payload->SubmissionTime.reset();
							}
							else
							{
								// Adjust the event wait timeout to cause the interrupt thread to wake automatically when
								// the timeout for this payload is reached, assuming it hasn't been woken by the GPU already.
								uint64 RemainingCycles = TimeoutCycles - ElapsedCycles;
								uint32 RemainingMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::nanoseconds(RemainingCycles)).count();
								Result.WaitTimeout = std::min(Result.WaitTimeout, RemainingMilliseconds);
							}
						}

						return;
					}

					// At this point, the current command list has completed on the GPU.
					CurrentQueue.Fence.bInterruptAwaited = false;
					CurrentQueue.PendingInterrupt.Pop();
					Result.Status |= QueueStatus::Processed;

					// Signal the CPU events of all sync points associated with this batch.
					for (auto& SyncPoint : Payload->SyncPointsToSignal)
					{
						if (SyncPoint->CompletionEvent)
						{
							SyncPoint->CompletionEvent->set();
						}
					}

					// We're done with this payload now.

					// GPU resources the payload is holding a reference to will be cleaned up here.
					// E.g. command list allocators, which get recycled on the parent device.
					delete Payload;
				}
			});

		return Result;
	}

	Context::ProcessResult Context::ProcessSubmissionQueue()
	{
		ProcessResult Result;

		//
		// Fence values for SyncPoint are determined on the submission thread,
		// where each queue has a monotonically incrementing fence value.
		//
		// We might receive work that waits on a sync point which has not yet been submitted
		// to the queue that will signal it, so we need to delay processing of those
		// payloads until the fence value is known.
		//

		// Process all queues (across all devices and adapters) to flush work.
		// Any sync point waits where the fence value is unknown will be left in the 
		// appropriate queue, to be processed the next time commands are submitted.

		std::vector<D3D12Payload*> PayloadsToHandDown;

		ForEachQueue([&](NodeQueue& CurrentQueue)
			{
				while (true)
				{
					std::vector<NodeQueue*> QueuesWithPayloads;
					{
						auto Payload = CurrentQueue.PendingSubmission.Peek();
						if (!Payload)
							break;

						// Accumulate the list of fences to await, and their maximum values
						while (Payload->SyncPointsToWait.Index < Payload->SyncPointsToWait.size())
						{
							auto& SyncPoint = Payload->SyncPointsToWait[Payload->SyncPointsToWait.Index];
							if (!SyncPoint->ResolvedFence.has_value())
							{
								// Need to wait on a sync point, but the fence value has not been resolved yet
								// (no other payloads have signaled the sync point yet).

								// Skip processing this queue, and move on to the next. We will retry later when
								// further work is submitted, which may contain the sync point we need.
								Result.Status |= QueueStatus::Pending;
								return;
							}

							CurrentQueue.EnqueueFenceWait(
								SyncPoint->ResolvedFence->Fence,
								SyncPoint->ResolvedFence->Value
							);

							Payload->SyncPointsToWait.Index++;
						}

						// All necessary sync points have been resolved.
						Payload->SyncPointsToWait = {};
						CurrentQueue.PendingSubmission.Pop();

						CurrentQueue.PayloadToSubmit = Payload;
						QueuesWithPayloads.emplace_back(&CurrentQueue);
						Result.Status |= QueueStatus::Processed;

						//
						// Now we generate any required barrier command lists. These may require
						// executing on a different queue (e.g. graphics-only transitions required 
						// before async compute work), so we gather potential work across all
						// queues for this device.
						//
						const uint32 MaxBatchSize = GetMaxExecuteBatchSize();

						auto AccumulateQueries = [&](CommandList* CommandList)
						{
							auto& TargetQueue = CommandList->Device->GetQueue(CommandList->Type);

							if (TargetQueue.NumCommandListsInBatch >= MaxBatchSize)
							{

								if (TargetQueue.NumCommandListsInBatch++ == 0)
								{
								}

								// Start a new batch
								TargetQueue.NumCommandListsInBatch = 0;
							}
						};

						for (uint32 Index = 0; Index < Payload->CommandListsToExecute.size(); Index++)
						{
							CommandList* CurrentCommandList = Payload->CommandListsToExecute[Index];

							//

							AccumulateQueries(CurrentCommandList);
						}
					}

					// Prepare the command lists from each payload for submission
					for (auto Queue : QueuesWithPayloads)
					{
						auto* Payload = Queue->PayloadToSubmit;

						Queue->NumCommandListsInBatch = 0;

						if (Queue->BarrierAllocator)
						{
							Payload->AllocatorsToRelease.emplace_back(Queue->BarrierAllocator);
							Queue->BarrierAllocator = nullptr;
						}

						// Hand payload down to the interrupt thread.
						PayloadsToHandDown.emplace_back(Payload);
					}

					// Queues with work to submit other than the current one (CurrentQueue) are performing barrier operations.
					// Submit this work first, followed by a fence signal + enqueued wait.
					for (auto* OtherQueue : QueuesWithPayloads)
					{
						if (OtherQueue != &CurrentQueue)
						{
							uint64 ValueSignaled = OtherQueue->ExecutePayload();
							CurrentQueue.EnqueueFenceWait(&OtherQueue->Fence, ValueSignaled);
						}
					}

					// Wait on the previous sync point and barrier command list fences.
					CurrentQueue.FlushFenceWaits();

					// Execute the command lists + signal for completion
					CurrentQueue.ExecutePayload();
				}
			}
		);

		for (auto* Payload : PayloadsToHandDown)
		{
			Payload->Queue.PendingInterrupt.Enqueue(Payload);
		}

		if (InterruptThread && white::has_anyflags(Result.Status, QueueStatus::Processed))
		{
			InterruptThread->Kick();
		}

		return Result;
	}

	void Context::SubmitPayloads(white::span<D3D12Payload*> Payloads)
	{
		// Push all payloads into the ordered per-device, per-pipe pending queues
		for (auto* Payload : Payloads)
		{
			Payload->Queue.PendingSubmission.Enqueue(Payload);
		}

		if (SubmissionThread)
		{
			SubmissionThread->Kick();
		}
		else
		{
			// Since we're processing directly on the calling thread, we need to take a scope lock.
			// Multiple engine threads might be calling Submit().
			{
				std::unique_lock Lock(SubmissionCS);

				// Process the submission queue until no further progress is being made.
				while (white::has_anyflags(ProcessSubmissionQueue().Status, QueueStatus::Processed)) {}
			}
		}
	}

	static int32 GetMaxExecuteBatchSize()
	{
		return
#ifndef NDEBUG
			platform::Render::Caps.IsDebugLayerEnabled ? 1 :
#endif
			std::numeric_limits<int32>::max();
	}

	uint64 NodeQueue::ExecutePayload()
	{
		// Wait for manual fences.
		for (auto& [ManualFence, Value] : PayloadToSubmit->FencesToWait)
		{
			CheckHResult(D3DCommandQueue->Wait(ManualFence.Get(), Value));
		}

		PayloadToSubmit->PreExecute();

		if (const int32 NumCommandLists = static_cast<uint32>(PayloadToSubmit->CommandListsToExecute.size()))
		{
			// Build SOA layout needed to call ExecuteCommandLists().
			std::vector<ID3D12CommandList*> D3DCommandLists;

			D3DCommandLists.reserve(NumCommandLists);

			for (auto* CommandList : PayloadToSubmit->CommandListsToExecute)
			{
				wassume(CommandList->IsClosed());
				D3DCommandLists.emplace_back(CommandList->Interfaces.CommandList.Get());
			}

			const int32 MaxBatchSize = GetMaxExecuteBatchSize();

			for (int32 DispatchNum, Offset = 0; Offset < NumCommandLists; Offset += DispatchNum)
			{
				DispatchNum = std::min(NumCommandLists - Offset, MaxBatchSize);

				extern int32 GD3D12MaxCommandsPerCommandList;
				if (GD3D12MaxCommandsPerCommandList > 0)
				{
					// Limit the dispatch group based on the total number of commands each command list contains, so that we
					// don't submit more than approx "GD3D12MaxCommandsPerCommandList" commands per call to ExecuteCommandLists().
					int32 Index = 0;
					for (int32 NumCommands = 0; Index < DispatchNum && NumCommands < GD3D12MaxCommandsPerCommandList; ++Index)
					{
						NumCommands += PayloadToSubmit->CommandListsToExecute[Offset + Index]->State.NumCommands;
					}

					DispatchNum = Index;
				}

				{
					D3DCommandQueue->ExecuteCommandLists(
						DispatchNum,
						&D3DCommandLists[Offset]
					);
				}
			}

			// Release the CommandList instance back to the parent device object pool.
			for (auto* CommandList : PayloadToSubmit->CommandListsToExecute)
				CommandList->Device->ReleaseCommandList(CommandList);

			PayloadToSubmit->CommandListsToExecute.clear();

			// We've executed command lists on the queue, so any 
			// future sync points need a new signaled fence value.
			bRequiresSignal = true;
		}

		bRequiresSignal |= PayloadToSubmit->bAlwaysSignal;

		// Keep the latest fence value in the submitted payload.
		// The interrupt thread uses this to determine when work has completed.
		uint64 FenceValue = SignalFence();
		PayloadToSubmit->CompletionFenceValue = FenceValue;

		// Signal any manual fences
		for (auto& [ManualFence, Value] : PayloadToSubmit->FencesToSignal)
		{
			CheckHResult(D3DCommandQueue->Signal(ManualFence.Get(), Value));
		}

		// Set the fence/value pair into any sync points we need to signal.
		for (auto& SyncPoint : PayloadToSubmit->SyncPointsToSignal)
		{
			wassume(!SyncPoint->ResolvedFence.has_value());
			SyncPoint->ResolvedFence.emplace(&Fence, PayloadToSubmit->CompletionFenceValue);
		}

		// Submission of this payload is completed. Signal the event if one was provided.
		if (PayloadToSubmit->SubmissionEvent)
		{
			PayloadToSubmit->SubmissionEvent->set();
		}

		PayloadToSubmit->SubmissionTime = platform::GetHighResolutionTicks();

		PayloadToSubmit = nullptr;

		return 0;
	}
	
}
