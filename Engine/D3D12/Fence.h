/*! \file Engine\Render\D3D12\Fence.h
\ingroup Render
\brief D3D12渲染同步简易包装。
*/

#ifndef WE_RENDER_D3D12_Fence_h
#define WE_RENDER_D3D12_Fence_h 1

#include <WBase/winttype.hpp>
#include <atomic>
#include "d3d12_dxgi.h"
#include "Common.h"
#include "RenderInterface/SyncPoint.h"
#include "System/Win32/NTHandle.h"


namespace platform_ex::Windows::D3D12 {
	enum class QueueType;

	class FenceCore : public AdapterChild
	{
	public:
		FenceCore(D3D12Adapter* InParent, uint64 InitialValue, uint32 GPUIndex);
		~FenceCore();

		inline ID3D12Fence* GetFence() const { return Fence.Get(); }
		inline HANDLE GetCompletionEvent() const { return hFenceCompleteEvent; }
		inline bool IsAvailable() const { return FenceValueAvailableAt <= Fence->GetCompletedValue(); }
		inline uint32 GetGPUIndex() const { return GPUIndex; }

		uint64 FenceValueAvailableAt;
		bool bInterruptAwaited = false;
	private:
		uint32 GPUIndex;

		COMPtr<ID3D12Fence> Fence;
		HANDLE hFenceCompleteEvent;
	};

	//todo:remove
	class Fence : public AdapterChild,public MultiNodeGPUObject {
	public:
		Fence(D3D12Adapter* InParent,GPUMaskType InGpuMask,const std::string& InName="<unnamed fence>");
		~Fence();

		void CreateFence();

		uint64 Signal(QueueType type);
		void GpuWait(uint32 DeviceIndex, QueueType InQueueType, uint64 FenceValue, uint32 FenceGPUIndex);

		void GpuWait(QueueType InQueueType, uint64 FenceValue)
		{
			GpuWait(GetGPUMask().ToIndex(), InQueueType, FenceValue, GetGPUMask().ToIndex());
		}

		void WaitForFence(uint64 FenceValue);
		bool IsFenceComplete(uint64 FenceValue);

		bool IsFenceCompleteFast(uint64 FenceValue) const { return FenceValue <= last_completed_val; }

		uint64 UpdateLastCompletedFence();

		uint64 GetLastCompletedFenceFast() const { return last_completed_val; };

		uint64 GetCurrentFence() const { return fence_val; }

		void Destroy();
	protected:
		void InternalSignal(QueueType InQueueType, uint64 FenceToSignal);

	protected:
		uint64 last_completed_val;
		std::atomic<uint64> fence_val;

		FenceCore* FenceCores[MAX_NUM_GPUNODES];
	};

	class ManualFence : public Fence
	{
	public:
		explicit ManualFence(D3D12Adapter* InParent, GPUMaskType InGPUMask, const std::string& InName = "<unnamed>")
			: Fence(InParent, InGPUMask, InName)
		{}

		// Signals the specified fence value.
		uint64 Signal(QueueType InQueueType, uint64 FenceToSignal);

		// Increments the current fence and returns the previous value.
		inline uint64 IncrementCurrentFence()
		{
			return fence_val++;
		}
	};

	class AwaitableSyncPoint :public platform::Render::SyncPoint
	{
	public:
		AwaitableSyncPoint(D3D12Adapter* InParent, uint64 InitialValue, uint32 GPUIndex);

		bool IsReady() const override
		{
			return fence->IsAvailable();
		}

		void Wait()
		{
			WaitForSingleObject(fence->GetCompletionEvent(), INFINITE);
			if(waited_callback)
				waited_callback();
		}

		void SetEventOnCompletion(uint64 signal_value)
		{
			fence->FenceValueAvailableAt = signal_value;
			fence->GetFence()->SetEventOnCompletion(signal_value, fence->GetCompletionEvent());
		}

		void AwaitSuspend(std::coroutine_handle<> handle);

		FenceCore& GetFence() { return *fence; }

		template<typename Fn>
		void SetOnWaited(Fn&& fn)
		{
			waited_callback = std::forward<Fn>(fn);
		}
	private:
		std::unique_ptr<FenceCore> fence;

		platform::Render::RenderTask task;

		std::function<void()> waited_callback;
	};
}

#endif