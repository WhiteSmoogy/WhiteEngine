#pragma once

#include "RenderInterface/ICommandContext.h"
#include "RenderInterface/Color_T.hpp"
#include "ContextStateCache.h"
#include "Submission.h"
#include "D3DCommandList.h"

namespace platform_ex::Windows::D3D12 {
	class CommandListManager;

	class Texture;

	enum class D3DFlushFlags
	{
		None = 0,

		// Block the calling thread until the submission thread has dispatched all work.
		WaitForSubmission = 1,

		// Both the calling thread until the GPU has signaled completion of all dispatched work.
		WaitForCompletion = 2
	};

	//
	// Base class that manages the recording of FD3D12FinalizedCommands instances.
	// Manages the logic for creating and recycling command lists and allocators.
	//
	class ContextCommon
	{
	public:
		ContextCommon(NodeDevice* InParent, QueueType Type, bool bIsDefaultContext);

		virtual ~ContextCommon();

		bool IsDefaultContext() const { return bIsDefaultContext; }
		bool IsAsyncComputeContext() const { return Type == QueueType::Async; }

		enum class ClearStateMode
		{
			TransientOnly,
			All
		};

		virtual void ClearState(ClearStateMode ClearStateMode = ClearStateMode::All) {}

		// Inserts a command to signal the specified sync point
		void SignalSyncPoint(SyncPoint* SyncPoint);

		// Inserts a command that blocks the GPU queue until the specified sync point is signaled.
		void WaitSyncPoint(SyncPoint* SyncPoint);

		// Inserts a command that signals the specified D3D12 fence object.
		void SignalManualFence(ID3D12Fence* Fence, uint64 Value);

		// Inserts a command that waits the specified D3D12 fence object.
		void WaitManualFence(ID3D12Fence* Fence, uint64 Value);

		// Complete recording of the current command list set, and appends the resulting
		// payloads to the given array. Resets the context so new commands can be recorded.
		void Finalize(std::vector<D3D12Payload*>& OutPayloads);

		bool IsOpen() const { return CmdList != nullptr; }

		// Returns unique identity that can be used to distinguish between command lists even after they were recycled.
		uint64 GetCommandListID() { return GetCommandList().State.CommandListID; }

		// Returns the current command list (or creates a new one if the command list was not open).
		CommandList& GetCommandList()
		{
			if (!CmdList)
			{
				OpenCommandList();
			}

			return *CmdList;
		}

		void TransitionResource(UnorderedAccessView* View, D3D12_RESOURCE_STATES after);

		void TransitionResource(ShaderResourceView* View, D3D12_RESOURCE_STATES after);

		bool TransitionResource(ResourceHolder* Resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after, const CViewSubset& subresourceSubset);
		bool TransitionResource(ResourceHolder* Resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after, uint32 subresource);

		void TransitionResource(DepthStencilView* View);


		void TransitionResource(DepthStencilView* pView, D3D12_RESOURCE_STATES after);

		void TransitionResource(RenderTargetView* pView, D3D12_RESOURCE_STATES after);

		bool TransitionResource(ResourceHolder* InResource, CResourceState& ResourceState_OnCommandList, uint32 InSubresourceIndex, D3D12_RESOURCE_STATES InBeforeState, D3D12_RESOURCE_STATES InAfterState, bool bInForceAfterState);

		void AddAliasingBarrier(ID3D12Resource* InResourceBefore, ID3D12Resource* InResourceAfter);
		void AddPendingResourceBarrier(ResourceHolder* Resource, D3D12_RESOURCE_STATES After, uint32 SubResource, CResourceState& ResourceState_OnCommandList);
		void AddTransitionBarrier(ResourceHolder* pResource, D3D12_RESOURCE_STATES Before, D3D12_RESOURCE_STATES After, uint32 Subresource, CResourceState* ResourceState_OnCommandList = nullptr);
		void AddUAVBarrier();

		void FlushResourceBarriers();

		SyncPoint* GetContextSyncPoint()
		{
			if (!ContextSyncPoint)
			{
				ContextSyncPoint = SyncPoint::Create(SyncPointType::GPUAndCPU);
				BatchedSyncPoints.ToSignal.emplace_back(ContextSyncPoint);
			}

			return ContextSyncPoint.Get();
		}

		// Flushes any pending commands in this context to the GPU.
		void FlushCommands(D3DFlushFlags FlushFlags = D3DFlushFlags::None);

		// Closes the current command list if the number of enqueued commands exceeds
		// the threshold defined by the "D3D12.MaxCommandsPerCommandList" cvar.
		void ConditionalSplitCommandList();

		auto BaseCommandList() { return GetCommandList().BaseCommandList(); }
		auto CopyCommandList() { return GetCommandList().CopyCommandList(); }
		auto GraphicsCommandList() { return GetCommandList().GraphicsCommandList(); }
		auto RayTracingCommandList() { return GetCommandList().RayTracingCommandList(); }

		auto GraphicsCommandList1() { return GetCommandList().GraphicsCommandList1(); }

	protected:
		virtual void OpenCommandList();
		virtual void CloseCommandList();

		void WriteGPUEventStackToBreadCrumbData(const char* Name, int32 CRC);
		void WriteGPUEventToBreadCrumbData(BreadcrumbStack* Breadcrumbs, uint32 MarkerIndex, bool bBeginEvent);
		[[nodiscard]] bool InitPayloadBreadcrumbs();
		void PopGPUEventStackFromBreadCrumbData();

		enum class Phase
		{
			Wait,
			Execute,
			Signal
		} CurrentPhase = Phase::Wait;

		D3D12Payload* GetPayload(Phase Phase)
		{
			if (Payloads.size() == 0 || Phase < CurrentPhase)
				Payloads.emplace_back(new D3D12Payload(Device, Type));

			CurrentPhase = Phase;
			return Payloads.back();
		}

		uint32 ActiveQueries = 0;
	protected:
		NodeDevice* const Device;
		QueueType const Type;

		const bool bIsDefaultContext;

		// Sync points which are waited at the start / signaled at the end 
		// of the whole batch of command lists this context recorded.
		struct
		{
			std::vector<SyncPointRef> ToWait;
			std::vector<SyncPointRef> ToSignal;
		} BatchedSyncPoints;

		// The allocator is reused for each new command list until the context is finalized.
		CommandAllocator* CommandAllocator = nullptr;
		CommandList* CmdList = nullptr;
		SyncPointRef ContextSyncPoint;

		// The array of recorded payloads the submission thread will process.
		// These are returned when the context is finalized.
		std::vector<D3D12Payload*> Payloads;

		//std::shared_ptr<BreadcrumbStack> BreadcrumbStack;

		ResourceBarrierBatcher ResourceBarrierBatcher;
	};

	class CommandContextBase : public platform::Render::CommandContext, public AdapterChild
	{
	public:
		CommandContextBase(D3D12Adapter* InParent, GPUMaskType InGpuMask);

	protected:
		GPUMaskType GPUMask;
	};

	class CommandContext final :public ContextCommon,public DeviceChild,public CommandContextBase
	{
	public:
		CommandContext(NodeDevice* InParent, QueueType Type, bool InIsDefaultContext);
		CommandContext(const CommandContext&) = delete;
	public:
		virtual void SetAsyncComputeBudgetInternal(platform::Render::AsyncComputeBudget Budget) {}

		void DispatchComputeShader(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ) override;

		void SetShaderTexture(const platform::Render::ComputeHWShader* Shader, uint32 TextureIndex, platform::Render::Texture* Texture) override;

		void SetShaderSampler(const platform::Render::ComputeHWShader* Shader, uint32 SamplerIndex, const platform::Render::TextureSampleDesc& Desc) override;

		void SetUAVParameter(const platform::Render::ComputeHWShader* Shader, uint32 UAVIndex, platform::Render::UnorderedAccessView* UAV) override;

		void SetUAVParameter(const platform::Render::ComputeHWShader* Shader, uint32 UAVIndex, platform::Render::UnorderedAccessView* UAV, uint32 InitialCount) override;

		void SetShaderParameter(const platform::Render::ComputeHWShader* Shader, uint32 BufferIndex, uint32 BaseIndex, uint32 NumBytes, const void* NewValue) override;

		void SetShaderResourceView(const platform::Render::ComputeHWShader* Shader, uint32 TextureIndex, platform::Render::ShaderResourceView* SRV) override;

		void SetShaderConstantBuffer(const platform::Render::ComputeHWShader* Shader, uint32 BaseIndex, platform::Render::ConstantBuffer* Buffer) override;

		void SetComputePipelineState(platform::Render::ComputePipelineState* ComputeState) override;

		void BeginRenderPass(const platform::Render::RenderPassInfo& Info, const char* Name) override;

		void SetViewport(uint32 MinX, uint32 MinY, float MinZ, uint32 MaxX, uint32 MaxY, float MaxZ) override;

		void SetScissorRect(bool bEnable, uint32 MinX, uint32 MinY, uint32 MaxX, uint32 MaxY) override;

		void SetVertexBuffer(uint32 slot, platform::Render::GraphicsBuffer* VertexBuffer) override;

		void SetGraphicsPipelineState(platform::Render::GraphicsPipelineState* pso) override;

		void SetShaderSampler(platform::Render::VertexHWShader* Shader, uint32 SamplerIndex, const platform::Render::TextureSampleDesc& Desc) override;
		void SetShaderSampler(platform::Render::PixelHWShader* Shader, uint32 SamplerIndex, const platform::Render::TextureSampleDesc& Desc) override;

		void SetShaderTexture(platform::Render::VertexHWShader* Shader, uint32 TextureIndex, platform::Render::Texture* Texture) override;
		void SetShaderTexture(platform::Render::PixelHWShader* Shader, uint32 TextureIndex, platform::Render::Texture* Texture) override;

		void SetShaderResourceView(platform::Render::VertexHWShader* Shader, uint32 TextureIndex, platform::Render::ShaderResourceView* SRV) override;
		void SetShaderResourceView(platform::Render::PixelHWShader* Shader, uint32 TextureIndex, platform::Render::ShaderResourceView* SRV) override;

		void SetShaderConstantBuffer(platform::Render::VertexHWShader* Shader, uint32 BaseIndex, platform::Render::ConstantBuffer* Buffer) override;
		void SetShaderConstantBuffer(platform::Render::PixelHWShader* Shader, uint32 BaseIndex, platform::Render::ConstantBuffer* Buffer) override;

		void SetShaderParameter(platform::Render::VertexHWShader* Shader, uint32 BufferIndex, uint32 BaseIndex, uint32 NumBytes, const void* NewValue) override;
		void SetShaderParameter(platform::Render::PixelHWShader* Shader, uint32 BufferIndex, uint32 BaseIndex, uint32 NumBytes, const void* NewValue) override;

		void DrawIndexedPrimitive(platform::Render::GraphicsBuffer* IndexBuffer, int32 BaseVertexIndex, uint32 FirstInstance, uint32 NumVertices, uint32 StartIndex, uint32 NumPrimitives, uint32 NumInstances) override;

		void DrawPrimitive(uint32 BaseVertexIndex, uint32 FirstInstance, uint32 NumPrimitives, uint32 NumInstances) override;

		void PushEvent(const char16_t* Name, platform::FColor Color) override;

		void PopEvent() override;

		void SetRenderTargets(
			uint32 NewNumSimultaneousRenderTargets,
			const platform::Render::RenderTarget* NewRenderTargets, const platform::Render::DepthRenderTarget* NewDepthStencilTarget
		);

		void ClearMRT(bool bClearColor, int32 NumClearColors, const white::math::float4* ColorArray, bool bClearDepth, float Depth, bool bClearStencil, uint32 Stencil);

		void SetRenderTargetsAndClear(const platform::Render::RenderTargetsInfo& RenderTargetsInfo);

		void OpenCommandList();
		void CloseCommandList();

		void BeginFrame();

		void EndFrame();
	private:
		void ClearState();

		void CommitGraphicsResourceTables();
		void CommitNonComputeShaderConstants();

		void CommitComputeShaderConstants();
		void CommitComputeResourceTables(const ComputeHWShader* ComputeShader);
	public:
		FastConstantAllocator ConstantsAllocator;

		CommandContextStateCache StateCache;

		// Tracks the currently set state blocks.
		RenderTargetView* CurrentRenderTargets[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
		DepthStencilView* CurrentDepthStencilTarget;
		Texture* CurrentDepthTexture;
		uint32 NumSimultaneousRenderTargets;

		/** Constant buffers for Set*ShaderParameter calls. */
		FastConstantBuffer VSConstantBuffer;
		FastConstantBuffer HSConstantBuffer;
		FastConstantBuffer DSConstantBuffer;
		FastConstantBuffer PSConstantBuffer;
		FastConstantBuffer GSConstantBuffer;
		FastConstantBuffer CSConstantBuffer;

		/** Track the currently bound constant buffers. */
		ConstantBuffer* BoundConstantBuffers[ShaderType::NumStandardType][MAX_CBS];

		/** Bit array to track which uniform buffers have changed since the last draw call. */
		uint16 DirtyConstantBuffers[ShaderType::NumStandardType];
	private:
		bool bUsingTessellation = false;
		bool bDiscardSharedConstants = false;
	};
}