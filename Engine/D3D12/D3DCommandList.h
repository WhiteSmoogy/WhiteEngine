#pragma once

#include "Utility.h"
#include "Common.h"
#include "ResourceHolder.h"
#include <atomic>
#include <queue>
#include <mutex>

#if ENABLE_AFTER_MATH
#include <Aftermath/GFSDK_Aftermath.h>
#endif

namespace platform_ex::Windows::D3D12 {
	class UnorderedAccessView;
	class ShaderResourceView;
	class RenderTargetView;
	class DepthStencilView;
	class ResourceHolder;

	class CommandContext;

	enum class QueueType;

	class CommandAllocator
	{
	public:
		friend class NodeDevice;
		friend class CommandList;

		CommandAllocator(CommandAllocator const&) = delete;

		explicit CommandAllocator(NodeDevice* InDevice, QueueType InType);
		~CommandAllocator();

		void Reset() {CheckHResult(D3DCommandAllocator->Reset()); }

		operator ID3D12CommandAllocator* () { return D3DCommandAllocator.Get(); }
	private:
		NodeDevice* const Device;
		QueueType Type;
		COMPtr<ID3D12CommandAllocator> D3DCommandAllocator;
	};

	//
	// Wraps a D3D command list object. Includes additional data required by the command context and submission thread.
	// Command lists are obtained from the parent device, and recycled in that device's object pool.
	class CommandList final
	{
	private:
		friend NodeDevice;
		friend CommandContext;
		friend ResourceBarrierBatcher;
		friend class ContextCommon;

		CommandList(CommandList const&) = delete;
		CommandList(CommandList&&) = delete;

		CommandList(CommandAllocator* CommandAllocator);

	public:
		~CommandList();

		void Reset(CommandAllocator* CommandAllocator);
		void Close();

		bool IsOpen() const { return !State.IsClosed; }

		bool IsClosed() const { return State.IsClosed; }

		uint32 GetNumCommands() const { return State.NumCommands; }

		NodeDevice* const Device;
		QueueType const Type;

		CResourceState& GetResourceState_OnCommandList(ResourceHolder* Resource);
	private:
		struct Interfaces
		{
			COMPtr<ID3D12CommandList> CommandList;
			COMPtr<ID3D12CopyCommandList> CopyCommandList;
			COMPtr<ID3D12GraphicsCommandList> GraphicsCommandList;

			template<typename T,size_t N>
			struct CommandListInterface :white::conditional_t< D3D12_MAX_COMMANDLIST_INTERFACE >= N, COMPtr<T>, COMPtr<IUnknown>>
			{};

			CommandListInterface<ID3D12GraphicsCommandList1, 1> GraphicsCommandList1;
			CommandListInterface<ID3D12GraphicsCommandList2, 2> GraphicsCommandList2;
			CommandListInterface<ID3D12GraphicsCommandList3, 3> GraphicsCommandList3;
			CommandListInterface<ID3D12GraphicsCommandList4, 4> GraphicsCommandList4;
			CommandListInterface<ID3D12GraphicsCommandList5, 5> GraphicsCommandList5;
			CommandListInterface<ID3D12GraphicsCommandList6, 6> GraphicsCommandList6;
			CommandListInterface<ID3D12GraphicsCommandList7, 7> GraphicsCommandList7;
			CommandListInterface<ID3D12GraphicsCommandList8, 8> GraphicsCommandList8;
			CommandListInterface<ID3D12GraphicsCommandList9, 9> GraphicsCommandList9;

#ifndef NDEBUG
			COMPtr<ID3D12DebugCommandList> DebugCommandList;
#endif

#if ENABLE_AFTER_MATH
			GFSDK_Aftermath_ContextHandle AftermathHandle = nullptr;
#endif
		} Interfaces;
	public:
		template <typename T>
		class TRValuePtr
		{
			friend CommandList;

			CommandList& CmdList;
			T* Ptr;

			TRValuePtr(CommandList& CommandList, T* Ptr)
				: CmdList(CommandList)
				, Ptr(Ptr)
			{}

		public:
			TRValuePtr() = delete;

			TRValuePtr(TRValuePtr const&) = delete;
			TRValuePtr(TRValuePtr&&) = delete;

			TRValuePtr& operator= (TRValuePtr const&) = delete;
			TRValuePtr& operator= (TRValuePtr&&) = delete;

			operator bool() const&& { return !!Ptr; }
			bool operator!() const&& { return  !Ptr; }

			// These accessor functions count useful work on command lists
			T* operator ->  ()&& { CmdList.State.NumCommands++; return Ptr; }
			T* Get()&& { CmdList.State.NumCommands++; return Ptr; }

			T* GetNoRefCount()&& { return Ptr; }
		};
	private:
		template <typename FInterfaceType>
		auto BuildRValuePtr(COMPtr<FInterfaceType> Interfaces::* Member)
		{
			return TRValuePtr<FInterfaceType>(*this, (Interfaces.*Member).Get());
		}

		template <typename FInterfaceType, size_t N>
		auto BuildRValuePtr(Interfaces::CommandListInterface<FInterfaceType,N> Interfaces::* Member)
		{
			return TRValuePtr<FInterfaceType>(*this, (Interfaces.*Member).Get());
		}

		auto BaseCommandList() { return BuildRValuePtr(&Interfaces::CommandList); }
		auto CopyCommandList() { return BuildRValuePtr(&Interfaces::CopyCommandList); }
		auto GraphicsCommandList() { return BuildRValuePtr(&Interfaces::GraphicsCommandList); }
		auto GraphicsCommandList1() { return BuildRValuePtr(&Interfaces::GraphicsCommandList1); }

#ifndef NDEBUG
		auto DebugCommandList() { return BuildRValuePtr(&Interfaces::DebugCommandList); }
#endif

		auto RayTracingCommandList() { return BuildRValuePtr(&Interfaces::GraphicsCommandList4); }

	private:
		struct ListState
		{
			static int64 NextCommandListID;

			ListState(CommandAllocator* CommandAllocator);

			// The allocator currently assigned to this command list.
			CommandAllocator* CmdAllocator;

			// Array of resources whose state needs to be synced between submits.
			std::vector<PendingResourceBarrier> PendingResourceBarriers;

			// A map of all D3D resources, and their states, that were state transitioned with tracking.
			std::unordered_map<ResourceHolder*, CResourceState> TrackedResourceState;

			// Unique ID of this command list used to avoid costly redundant operations, such as resource residency updates.
			// This value is updated every time the command list is reset, so it is safe to use even when command list object is recycled.
			// Value should be only used for identity, not for synchronization. Valid values are guaranteed to be > 0.
			uint64 CommandListID;

			uint32 NumCommands = 0;

			bool IsClosed = false;
		} State;
	};
}
