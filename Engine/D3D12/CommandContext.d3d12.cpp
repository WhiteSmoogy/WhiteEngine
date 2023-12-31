#include "CommandContext.h"
#include "Texture.h"
#include "NodeDevice.h"
#include "Adapter.h"
#include "Context.h"

#if USE_PIX
#include <WinPixEventRuntime/pix3.h>
#if WFL_Win64
#pragma comment(lib,"WinPixEventRuntime/lib/x64/WinPixEventRuntime.lib")
#endif
#endif

using namespace platform_ex::Windows::D3D12;

constexpr auto MaxSimultaneousRenderTargets = platform::Render::MaxSimultaneousRenderTargets;

int32 MaxInitialResourceCopiesPerCommandList = 10000;
int32 GD3D12MaxCommandsPerCommandList = 10000;


ContextCommon::ContextCommon(NodeDevice* InParent, QueueType Type, bool bIsDefaultContext)
	:Device(InParent)
	, Type(Type)
	, bIsDefaultContext(bIsDefaultContext)
{
}

CommandContextBase::CommandContextBase(D3D12Adapter* InParent, GPUMaskType InGpuMask)
	:AdapterChild(InParent)
	, GPUMask(InGpuMask)
{
}

CommandContext::CommandContext(NodeDevice* InParent, QueueType Type, bool InIsDefaultContext)
	:
	ContextCommon(InParent, Type, InIsDefaultContext),
	DeviceChild(InParent),
	CommandContextBase(InParent->GetParentAdapter(), InParent->GetGPUMask()),
	ConstantsAllocator(InParent, InParent->GetGPUMask()),
	VSConstantBuffer(InParent, ConstantsAllocator),
	HSConstantBuffer(InParent, ConstantsAllocator),
	DSConstantBuffer(InParent, ConstantsAllocator),
	PSConstantBuffer(InParent, ConstantsAllocator),
	GSConstantBuffer(InParent, ConstantsAllocator),
	CSConstantBuffer(InParent, ConstantsAllocator),
	StateCache(*this, InParent->GetGPUMask())
{
}

void CommandContext::BeginRenderPass(const platform::Render::RenderPassInfo& Info, const char* Name)
{
	platform::Render::RenderTargetsInfo RTInfo(Info);

	SetRenderTargetsAndClear(RTInfo);
}

void CommandContext::SetRenderTargetsAndClear(const platform::Render::RenderTargetsInfo& RenderTargetsInfo)
{
	SetRenderTargets(RenderTargetsInfo.NumColorRenderTargets,
		RenderTargetsInfo.ColorRenderTarget,
		&RenderTargetsInfo.DepthStencilRenderTarget);

	white::math::float4 ClearColors[MaxSimultaneousRenderTargets];

	if (RenderTargetsInfo.bClearColor)
	{
		for (int Index = 0; Index < RenderTargetsInfo.NumColorRenderTargets; ++Index)
		{
			if (RenderTargetsInfo.ColorRenderTarget[Index].Texture != nullptr)
			{
				auto& ClearValue = RenderTargetsInfo.ColorRenderTarget[Index].Texture->GetClearBinding();
				memcpy(ClearColors[Index], ClearValue.Value.Color);
			}
			else
				ClearColors[Index] = white::math::float4();
		}
	}

	float DepthClear = 1.0;
	uint32 StencilClear = 0;

	if (RenderTargetsInfo.bClearDepth || RenderTargetsInfo.bClearStencil)
	{
		//TODO clear value binding
		auto& ClearValue = RenderTargetsInfo.DepthStencilRenderTarget.Texture->GetClearBinding();

		DepthClear = ClearValue.Value.DSValue.Depth;
		StencilClear = ClearValue.Value.DSValue.Stencil;
	}

	ClearMRT(RenderTargetsInfo.bClearColor, RenderTargetsInfo.NumColorRenderTargets, ClearColors, RenderTargetsInfo.bClearDepth, DepthClear, RenderTargetsInfo.bClearStencil, StencilClear);
}

void CommandContext::SetViewport(uint32 MinX, uint32 MinY, float MinZ, uint32 MaxX, uint32 MaxY, float MaxZ)
{
	// These are the maximum viewport extents for D3D12. Exceeding them leads to badness.
	wconstraint(MinX <= (uint32)D3D12_VIEWPORT_BOUNDS_MAX);
	wconstraint(MinY <= (uint32)D3D12_VIEWPORT_BOUNDS_MAX);
	wconstraint(MaxX <= (uint32)D3D12_VIEWPORT_BOUNDS_MAX);
	wconstraint(MaxY <= (uint32)D3D12_VIEWPORT_BOUNDS_MAX);

	D3D12_VIEWPORT Viewport = { (float)MinX, (float)MinY, (float)(MaxX - MinX), (float)(MaxY - MinY), MinZ, MaxZ };

	if (Viewport.Width > 0 && Viewport.Height > 0)
	{
		StateCache.SetViewport(Viewport);
		SetScissorRect(true, MinX, MinY, MaxX, MaxY);
	}
}

void CommandContext::SetScissorRect(bool bEnable, uint32 MinX, uint32 MinY, uint32 MaxX, uint32 MaxY)
{
	if (bEnable)
	{
		const CD3DX12_RECT ScissorRect(MinX, MinY, MaxX, MaxY);
		StateCache.SetScissorRect(ScissorRect);
	}
	else
	{
		const D3D12_VIEWPORT& Viewport = StateCache.GetViewport();
		const CD3DX12_RECT ScissorRect((LONG)Viewport.TopLeftX, (LONG)Viewport.TopLeftY, (LONG)Viewport.TopLeftX + (LONG)Viewport.Width, (LONG)Viewport.TopLeftY + (LONG)Viewport.Height);
		StateCache.SetScissorRect(ScissorRect);
	}
}

void CommandContext::SetVertexBuffer(uint32 slot, platform::Render::GraphicsBuffer* IVertexBuffer)
{
	auto VertexBuffer = static_cast<GraphicsBuffer*>(IVertexBuffer);

	StateCache.SetStreamSource(VertexBuffer, slot, 0);
}

void CommandContext::SetGraphicsPipelineState(platform::Render::GraphicsPipelineState* pso)
{
	auto PipelineState = static_cast<GraphicsPipelineState*>(pso);

	const bool bWasUsingTessellation = bUsingTessellation;
	bUsingTessellation = PipelineState->GetHullShader() && PipelineState->GetDomainShader();

	// Ensure the command buffers are reset to reduce the amount of data that needs to be versioned.
	VSConstantBuffer.Reset();
	PSConstantBuffer.Reset();
	HSConstantBuffer.Reset();
	DSConstantBuffer.Reset();
	GSConstantBuffer.Reset();
	// Should this be here or in RHISetComputeShader? Might need a new bDiscardSharedConstants for CS.
	CSConstantBuffer.Reset();

	//really should only discard the constants if the shader state has actually changed.
	bDiscardSharedConstants = true;

	if (!false)
	{
		StateCache.SetDepthBounds(0.0f, 1.0f);
	}

	StateCache.SetGraphicsPipelineState(PipelineState, bUsingTessellation != bWasUsingTessellation);
	StateCache.SetStencilRef(0);
}

void CommandContext::SetShaderSampler(platform::Render::VertexHWShader* Shader, uint32 SamplerIndex, const platform::Render::TextureSampleDesc& Desc)
{
	auto pSampler = GetParentDevice()->CreateSampler(Convert(Desc));

	StateCache.SetSamplerState<ShaderType::VertexShader>(pSampler.get(), SamplerIndex);
}

void CommandContext::SetShaderSampler(platform::Render::PixelHWShader* Shader, uint32 SamplerIndex, const platform::Render::TextureSampleDesc& Desc)
{
	auto pSampler = GetParentDevice()->CreateSampler(Convert(Desc));

	StateCache.SetSamplerState<ShaderType::PixelShader>(pSampler.get(), SamplerIndex);
}

void CommandContext::SetShaderTexture(platform::Render::VertexHWShader* Shader, uint32 TextureIndex, platform::Render::Texture* ITexture)
{
	auto SRV = dynamic_cast_texture(ITexture)->RetriveShaderResourceView();

	StateCache.SetShaderResourceView<ShaderType::VertexShader>(SRV, TextureIndex);
}

void CommandContext::SetShaderTexture(platform::Render::PixelHWShader* Shader, uint32 TextureIndex, platform::Render::Texture* ITexture)
{
	auto SRV = dynamic_cast_texture(ITexture)->RetriveShaderResourceView();

	StateCache.SetShaderResourceView<ShaderType::PixelShader>(SRV, TextureIndex);
}

void CommandContext::SetShaderResourceView(platform::Render::VertexHWShader* Shader, uint32 TextureIndex, platform::Render::ShaderResourceView* SRV)
{
	StateCache.SetShaderResourceView<ShaderType::VertexShader>(static_cast<ShaderResourceView*>(SRV), TextureIndex);
}

void CommandContext::SetShaderResourceView(platform::Render::PixelHWShader* Shader, uint32 TextureIndex, platform::Render::ShaderResourceView* SRV)
{
	StateCache.SetShaderResourceView<ShaderType::PixelShader>(static_cast<ShaderResourceView*>(SRV), TextureIndex);
}

void CommandContext::SetShaderResourceView(const platform::Render::ComputeHWShader* Shader, uint32 TextureIndex, platform::Render::ShaderResourceView* SRV)
{
	StateCache.SetShaderResourceView<ShaderType::ComputeShader>(static_cast<ShaderResourceView*>(SRV), TextureIndex);
}

void CommandContext::SetShaderConstantBuffer(platform::Render::VertexHWShader* Shader, uint32 BaseIndex, platform::Render::ConstantBuffer* IBuffer)
{
	auto Buffer = static_cast<ConstantBuffer*>(IBuffer);

	StateCache.SetConstantsBuffer<ShaderType::VertexShader>(BaseIndex, Buffer);

	BoundConstantBuffers[ShaderType::VertexShader][BaseIndex] = Buffer;
	DirtyConstantBuffers[ShaderType::VertexShader] |= (1 << BaseIndex);
}

void CommandContext::SetShaderConstantBuffer(platform::Render::PixelHWShader* Shader, uint32 BaseIndex, platform::Render::ConstantBuffer* IBuffer)
{
	auto Buffer = static_cast<ConstantBuffer*>(IBuffer);

	StateCache.SetConstantsBuffer<ShaderType::PixelShader>(BaseIndex, Buffer);

	BoundConstantBuffers[ShaderType::PixelShader][BaseIndex] = Buffer;
	DirtyConstantBuffers[ShaderType::PixelShader] |= (1 << BaseIndex);
}

void CommandContext::SetShaderConstantBuffer(const platform::Render::ComputeHWShader* Shader, uint32 BaseIndex, platform::Render::ConstantBuffer* IBuffer)
{
	auto Buffer = static_cast<ConstantBuffer*>(IBuffer);

	StateCache.SetConstantsBuffer<ShaderType::ComputeShader>(BaseIndex, Buffer);

	BoundConstantBuffers[ShaderType::ComputeShader][BaseIndex] = Buffer;
	DirtyConstantBuffers[ShaderType::ComputeShader] |= (1 << BaseIndex);
}

void CommandContext::SetShaderParameter(platform::Render::VertexHWShader* Shader, uint32 BufferIndex, uint32 BaseIndex, uint32 NumBytes, const void* NewValue)
{
	wconstraint(BufferIndex == 0);
	VSConstantBuffer.UpdateConstant(reinterpret_cast<const uint8*>(NewValue), BaseIndex, NumBytes);
}

void CommandContext::SetShaderParameter(platform::Render::PixelHWShader* Shader, uint32 BufferIndex, uint32 BaseIndex, uint32 NumBytes, const void* NewValue)
{
	wconstraint(BufferIndex == 0);
	PSConstantBuffer.UpdateConstant(reinterpret_cast<const uint8*>(NewValue), BaseIndex, NumBytes);
}

void CommandContext::SetShaderParameter(const platform::Render::ComputeHWShader* Shader, uint32 BufferIndex, uint32 BaseIndex, uint32 NumBytes, const void* NewValue)
{
	wconstraint(BufferIndex == 0);
	CSConstantBuffer.UpdateConstant(reinterpret_cast<const uint8*>(NewValue), BaseIndex, NumBytes);
}

void CommandContext::CommitGraphicsResourceTables()
{
	//don't support UE4 ShaderResourceTable
}

void CommandContext::CommitNonComputeShaderConstants()
{
	const auto* const  GraphicPSO = StateCache.GetGraphicsPipelineState();

	wconstraint(GraphicPSO);

	// Only set the constant buffer if this shader needs the global constant buffer bound
	// Otherwise we will overwrite a different constant buffer
	if (GraphicPSO->bShaderNeedsGlobalConstantBuffer[ShaderType::VertexShader])
	{
		StateCache.SetConstantBuffer<ShaderType::VertexShader>(VSConstantBuffer, bDiscardSharedConstants);
	}

	// Skip HS/DS CB updates in cases where tessellation isn't being used
	// Note that this is *potentially* unsafe because bDiscardSharedConstants is cleared at the
	// end of the function, however we're OK for now because bDiscardSharedConstants
	// is always reset whenever bUsingTessellation changes in SetBoundShaderState()
	if (bUsingTessellation)
	{
		if (GraphicPSO->bShaderNeedsGlobalConstantBuffer[ShaderType::HullShader])
		{
			StateCache.SetConstantBuffer<ShaderType::HullShader>(HSConstantBuffer, bDiscardSharedConstants);
		}

		if (GraphicPSO->bShaderNeedsGlobalConstantBuffer[ShaderType::DomainShader])
		{
			StateCache.SetConstantBuffer<ShaderType::DomainShader>(DSConstantBuffer, bDiscardSharedConstants);
		}
	}

	if (GraphicPSO->bShaderNeedsGlobalConstantBuffer[ShaderType::GeometryShader])
	{
		StateCache.SetConstantBuffer<ShaderType::GeometryShader>(GSConstantBuffer, bDiscardSharedConstants);
	}

	if (GraphicPSO->bShaderNeedsGlobalConstantBuffer[ShaderType::PixelShader])
	{
		StateCache.SetConstantBuffer<ShaderType::PixelShader>(PSConstantBuffer, bDiscardSharedConstants);
	}

	bDiscardSharedConstants = false;
}

void ContextCommon::OpenCommandList()
{
	WAssert(!IsOpen(), "Command list is already open.");

	if (CommandAllocator == nullptr)
	{
		// Obtain a command allocator if the context doesn't already have one.
		CommandAllocator = Device->ObtainCommandAllocator(Type);
	}

	// Get a new command list
	CmdList = Device->ObtainCommandList(CommandAllocator);
	GetPayload(Phase::Execute)->CommandListsToExecute.emplace_back(CmdList);
}

void CommandContext::OpenCommandList()
{
	ContextCommon::OpenCommandList();

	// Notify the descriptor cache about the new command list
	// This will set the descriptor cache's current heaps on the new command list.
	StateCache.GetDescriptorCache()->OpenCommandList();
}

void ContextCommon::CloseCommandList()
{
	WAssert(IsOpen(), "Command list is not open.");
	WAssert(Payloads.size() && CurrentPhase == Phase::Execute, "Expected the current payload to be in the execute phase.");

	WAssert(ActiveQueries == 0, "All queries must be completed before the command list is closed.");

	auto* Payload = GetPayload(Phase::Execute);

	// Do this before we insert the final timestamp to ensure we're timing all the work on the command list.
	FlushResourceBarriers();

	CmdList->Close();
	CmdList = nullptr;
}

void CommandContext::CloseCommandList()
{
	StateCache.GetDescriptorCache()->CloseCommandList();
	ContextCommon::CloseCommandList();
	// Mark state as dirty now, because ApplyState may be called before OpenCommandList(), and it needs to know that the state has
	// become invalid, so it can set it up again (which opens a new command list if necessary).
	StateCache.DirtyStateForNewCommandList();
}


void CommandContext::BeginFrame()
{
	//Device->GetDefaultBufferAllocator().BeginFrame();
}

void CommandContext::EndFrame()
{
	ParentAdapter->EndFrame();

	auto GPUIndex = 0;

	auto Device = ParentAdapter->GetNodeDevice(GPUIndex);
	auto& DefaultContext = Device->GetDefaultCommandContext();
	DefaultContext.FlushResourceBarriers();

	DefaultContext.ClearState();
	DefaultContext.FlushCommands();

	uint64 BufferPoolDeletionFrameLag = 2;
	Device->GetDefaultBufferAllocator().CleanUpAllocations(BufferPoolDeletionFrameLag);

	uint64 FastAllocatorDeletionFrameLag = 10;
	Device->GetDefaultFastAllocator().CleanupPages(FastAllocatorDeletionFrameLag);
}

void CommandContext::ClearState()
{
	StateCache.ClearState();

	bDiscardSharedConstants = false;

	std::memset(BoundConstantBuffers, 0, sizeof(BoundConstantBuffers));
	std::memset(DirtyConstantBuffers, 0, sizeof(DirtyConstantBuffers));
}

static uint32 GetIndexCount(platform::Render::PrimtivteType type, uint32 NumPrimitives)
{
	auto PrimitiveTypeFactor = platform::Render::GetPrimitiveTypeFactor(type);

	auto PrimitiveTypeOffset = type == platform::Render::PrimtivteType::TriangleStrip ? 2 : 0;

	return PrimitiveTypeFactor * NumPrimitives + PrimitiveTypeOffset;
}

void CommandContext::DrawIndexedPrimitive(platform::Render::GraphicsBuffer* IIndexBuffer, int32 BaseVertexIndex, uint32 FirstInstance, uint32 NumVertices, uint32 StartIndex, uint32 NumPrimitives, uint32 NumInstances)
{
	// called should make sure the input is valid, this avoid hidden bugs
	wconstraint(NumPrimitives > 0);

	NumInstances = std::max<uint32>(1, NumInstances);

	CommitGraphicsResourceTables();
	CommitNonComputeShaderConstants();

	uint32 IndexCount = GetIndexCount(StateCache.GetPrimtivteType(), NumPrimitives);
	auto IndexBuffer = static_cast<GraphicsBuffer*>(IIndexBuffer);

	const DXGI_FORMAT Format = IndexBuffer->GetFormat();
	StateCache.SetIndexBuffer(IndexBuffer->Location, Format, 0);
	StateCache.ApplyState<CPT_Graphics>();

	GraphicsCommandList()->DrawIndexedInstanced(IndexCount, NumInstances, StartIndex, BaseVertexIndex, FirstInstance);
}

void CommandContext::DrawPrimitive(uint32 BaseVertexIndex, uint32 FirstInstance, uint32 NumPrimitives, uint32 NumInstances)
{
	NumInstances = std::max<uint32>(1, NumInstances);

	CommitGraphicsResourceTables();
	CommitNonComputeShaderConstants();

	uint32 VertexCount = GetIndexCount(StateCache.GetPrimtivteType(), NumPrimitives);

	StateCache.ApplyState<CPT_Graphics>();
	GraphicsCommandList()->DrawInstanced(VertexCount, NumInstances, BaseVertexIndex, FirstInstance);
}

void CommandContext::SetIndexBuffer(platform::Render::GraphicsBuffer* IIndexBuffer)
{
	auto IndexBuffer = static_cast<GraphicsBuffer*>(IIndexBuffer);

	DXGI_FORMAT Format = IndexBuffer->GetFormat();

	if (Format == DXGI_FORMAT_UNKNOWN)
	{
		switch (IndexBuffer->GetStride())
		{
		case 4:
			Format = DXGI_FORMAT_R32_UINT;
			break;
		case 2:
			Format = DXGI_FORMAT_R16_UINT;
		default:
			WAssert(false, "Invalid IndexBuffer");
		}
	}

	StateCache.SetIndexBuffer(IndexBuffer->Location, Format, 0);
}


void CommandContext::DrawIndirect(platform::Render::CommandSignature* Sig, uint32 MaxCmdCount, platform::Render::GraphicsBuffer* IndirectBuffer, uint32 BufferOffset, platform::Render::GraphicsBuffer* CountBuffer, uint32 CountBufferOffset)
{
	CommitGraphicsResourceTables();
	CommitNonComputeShaderConstants();

	StateCache.ApplyState<CPT_Graphics>();

	auto Signature = static_cast<D3DCommandSignature*>(Sig);

	ID3D12Resource* CountBufferResource = nullptr;
	if (CountBuffer != nullptr)
	{
		auto& CounterLocation = static_cast<GraphicsBuffer*>(IndirectBuffer)->Location;

		CountBufferResource = CounterLocation.GetResource()->Resource();
		TransitionResource(CounterLocation.GetResource(), D3D12_RESOURCE_STATE_TBD, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
	}

	auto& Location = static_cast<GraphicsBuffer*>(IndirectBuffer)->Location;

	// Indirect args buffer can be a previously pending UAV, which becomes PS\Non-PS read. ApplyState will flush pending transitions, so enqueue the indirect
	// arg transition and flush here.
	TransitionResource(Location.GetResource(), D3D12_RESOURCE_STATE_TBD, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
	FlushResourceBarriers();	// Must flush so the desired state is actually set.

	GraphicsCommandList()->ExecuteIndirect(
		Signature->Get(),
		MaxCmdCount,
		Location.GetResource()->Resource(),
		BufferOffset,
		CountBufferResource,
		CountBufferOffset
	);
}

struct FRTVDesc
{
	uint32 Width;
	uint32 Height;
	DXGI_SAMPLE_DESC SampleDesc;
};

// Return an FRTVDesc structure whose
// Width and height dimensions are adjusted for the RTV's miplevel.
FRTVDesc GetRenderTargetViewDesc(RenderTargetView* RenderTargetView)
{
	const D3D12_RENDER_TARGET_VIEW_DESC& TargetDesc = RenderTargetView->GetDesc();

	auto BaseResource = RenderTargetView->GetResource();
	uint32 MipIndex = 0;
	FRTVDesc ret;
	memset(&ret, 0, sizeof(ret));

	switch (TargetDesc.ViewDimension)
	{
	case D3D12_RTV_DIMENSION_TEXTURE2D:
	case D3D12_RTV_DIMENSION_TEXTURE2DMS:
	case D3D12_RTV_DIMENSION_TEXTURE2DARRAY:
	case D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY:
	{
		D3D12_RESOURCE_DESC const& Desc = BaseResource->GetDesc();
		ret.Width = (uint32)Desc.Width;
		ret.Height = Desc.Height;
		ret.SampleDesc = Desc.SampleDesc;
		if (TargetDesc.ViewDimension == D3D12_RTV_DIMENSION_TEXTURE2D || TargetDesc.ViewDimension == D3D12_RTV_DIMENSION_TEXTURE2DARRAY)
		{
			// All the non-multisampled texture types have their mip-slice in the same position.
			MipIndex = TargetDesc.Texture2D.MipSlice;
		}
		break;
	}
	case D3D12_RTV_DIMENSION_TEXTURE3D:
	{
		D3D12_RESOURCE_DESC const& Desc = BaseResource->GetDesc();
		ret.Width = (uint32)Desc.Width;
		ret.Height = Desc.Height;
		ret.SampleDesc.Count = 1;
		ret.SampleDesc.Quality = 0;
		MipIndex = TargetDesc.Texture3D.MipSlice;
		break;
	}
	default:
	{
		// not expecting 1D targets.
		wconstraint(false);
	}
	}
	ret.Width >>= MipIndex;
	ret.Height >>= MipIndex;
	return ret;
}


void CommandContext::SetRenderTargets(uint32 NewNumSimultaneousRenderTargets, const platform::Render::RenderTarget* NewRenderTargets, const platform::Render::DepthRenderTarget* INewDepthStencilTarget)
{
	auto NewDepthStencilTarget = INewDepthStencilTarget ? dynamic_cast<Texture*>(INewDepthStencilTarget->Texture) : nullptr;

	bool bTargetChanged = false;

	DepthStencilView* DepthStencilView = nullptr;
	if (NewDepthStencilTarget)
	{
		DepthStencilView = NewDepthStencilTarget->GetDepthStencilView({});
	}

	// Check if the depth stencil target is different from the old state.
	if (CurrentDepthStencilTarget != DepthStencilView)
	{
		CurrentDepthTexture = NewDepthStencilTarget;
		CurrentDepthStencilTarget = DepthStencilView;
		bTargetChanged = true;
	}

	// Gather the render target views for the new render targets.
	RenderTargetView* NewRenderTargetViews[MaxSimultaneousRenderTargets];
	for (uint32 RenderTargetIndex = 0; RenderTargetIndex < MaxSimultaneousRenderTargets; ++RenderTargetIndex)
	{
		RenderTargetView* RenderTargetView = nullptr;

		if (RenderTargetIndex < NewNumSimultaneousRenderTargets && NewRenderTargets[RenderTargetIndex].Texture != nullptr)
		{
			int32 RTMipIndex = NewRenderTargets[RenderTargetIndex].MipIndex;
			int32 RTSliceIndex = NewRenderTargets[RenderTargetIndex].ArraySlice;

			auto NewRenderTarget = dynamic_cast<Texture*>(NewRenderTargets[RenderTargetIndex].Texture);

			RenderTargetView = NewRenderTarget->GetRenderTargetView(RTMipIndex, RTSliceIndex);

			WAssert(RenderTargetView, "Texture being set as render target has no RTV");
		}

		NewRenderTargetViews[RenderTargetIndex] = RenderTargetView;
		// Check if the render target is different from the old state.
		if (CurrentRenderTargets[RenderTargetIndex] != RenderTargetView)
		{
			CurrentRenderTargets[RenderTargetIndex] = RenderTargetView;
			bTargetChanged = true;
		}
	}

	if (NumSimultaneousRenderTargets != NewNumSimultaneousRenderTargets)
	{
		NumSimultaneousRenderTargets = NewNumSimultaneousRenderTargets;
		bTargetChanged = true;
	}

	if (bTargetChanged)
	{
		StateCache.SetRenderTargets(NewNumSimultaneousRenderTargets, CurrentRenderTargets, CurrentDepthStencilTarget);
		StateCache.ClearUAVs<ShaderType::PixelShader>();
	}

	// Set the viewport to the full size of render target 0.
	if (NewRenderTargetViews[0])
	{
		// check target 0 is valid
		wconstraint(0 < NewNumSimultaneousRenderTargets && NewRenderTargets[0].Texture != nullptr);
		FRTVDesc RTTDesc = GetRenderTargetViewDesc(NewRenderTargetViews[0]);
		SetViewport(0, 0, 0.0f, RTTDesc.Width, RTTDesc.Height, 1.0f);
	}
	else if (DepthStencilView)
	{
		auto DepthTargetTexture = DepthStencilView->GetResource();
		D3D12_RESOURCE_DESC const& DTTDesc = DepthTargetTexture->GetDesc();
		SetViewport(0, 0, 0.0f, static_cast<uint32>(DTTDesc.Width), DTTDesc.Height, 1.0f);
	}
}

void CommandContext::ClearMRT(bool bClearColor, int32 NumClearColors, const white::math::float4* ColorArray, bool bClearDepth, float Depth, bool bClearStencil, uint32 Stencil)
{
	const D3D12_VIEWPORT& Viewport = StateCache.GetViewport();
	const D3D12_RECT& ScissorRect = StateCache.GetScissorRect();

	if (ScissorRect.left >= ScissorRect.right || ScissorRect.top >= ScissorRect.bottom)
	{
		return;
	}

	RenderTargetView* RenderTargetViews[MaxSimultaneousRenderTargets];
	DepthStencilView* DSView = nullptr;
	uint32 NumSimultaneousRTs = 0;
	StateCache.GetRenderTargets(RenderTargetViews, &NumSimultaneousRTs, &DSView);

	const LONG Width = static_cast<LONG>(Viewport.Width);
	const LONG Height = static_cast<LONG>(Viewport.Height);

	bool bClearCoversEntireSurface = false;
	if (ScissorRect.left <= 0 && ScissorRect.top <= 0 &&
		ScissorRect.right >= Width && ScissorRect.bottom >= Height)
	{
		bClearCoversEntireSurface = true;
	}

	const bool bSupportsFastClear = true;
	uint32 ClearRectCount = 0;
	D3D12_RECT* pClearRects = nullptr;
	D3D12_RECT ClearRects[4];

	// Only pass a rect down to the driver if we specifically want to clear a sub-rect
	if (!bSupportsFastClear || !bClearCoversEntireSurface)
	{
		{
			ClearRects[ClearRectCount] = ScissorRect;
			ClearRectCount++;
		}

		pClearRects = ClearRects;
	}

	const bool ClearRTV = bClearColor && NumSimultaneousRTs > 0;
	const bool ClearDSV = (bClearDepth || bClearStencil) && DSView;

	if (ClearRTV)
	{
		for (uint32 TargetIndex = 0; TargetIndex < NumSimultaneousRTs; TargetIndex++)
		{
			auto View = RenderTargetViews[TargetIndex];

			if (View != nullptr)
			{
				TransitionResource(View, D3D12_RESOURCE_STATE_RENDER_TARGET);
			}
		}

	}

	uint32 ClearFlags = 0;

	if (ClearDSV)
	{
		if (bClearDepth && DSView->HasDepth())
		{
			ClearFlags |= D3D12_CLEAR_FLAG_DEPTH;
		}

		if (bClearStencil && DSView->HasStencil())
		{
			ClearFlags |= D3D12_CLEAR_FLAG_STENCIL;
		}

		if (bClearDepth && (!DSView->HasStencil() || bClearStencil))
		{
			// Transition the entire view (Both depth and stencil planes if applicable)
			// Some DSVs don't have stencil bits.
			TransitionResource(DSView, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		}
		else
		{
			if (bClearDepth)
			{
				// Transition just the depth plane
				TransitionResource(DSView->GetResource(), D3D12_RESOURCE_STATE_TBD, D3D12_RESOURCE_STATE_DEPTH_WRITE, DSView->GetDepthOnlySubset());
			}
			else
			{
				// Transition just the stencil plane
				TransitionResource(DSView->GetResource(), D3D12_RESOURCE_STATE_TBD, D3D12_RESOURCE_STATE_DEPTH_WRITE, DSView->GetStencilOnlySubset());
			}
		}
	}

	if (ClearRTV || ClearDSV)
	{
		FlushResourceBarriers();

		if (ClearRTV)
		{
			for (uint32 TargetIndex = 0; TargetIndex < NumSimultaneousRTs; ++TargetIndex)
			{
				auto RTView = RenderTargetViews[TargetIndex];

				if (RTView != nullptr)
				{
					GraphicsCommandList()->ClearRenderTargetView(RTView->GetOfflineCpuHandle(), (float*)&ColorArray[TargetIndex], ClearRectCount, pClearRects);
				}
			}
		}

		if (ClearDSV)
		{
			GraphicsCommandList()->ClearDepthStencilView(DSView->GetOfflineCpuHandle(), (D3D12_CLEAR_FLAGS)ClearFlags, Depth, Stencil, ClearRectCount, pClearRects);
		}
	}
}

void CommandContext::DispatchComputeShader(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ)
{
	const ComputeHWShader* ComputeShader = nullptr;
	StateCache.GetComputeShader(&ComputeShader);

	if (ComputeShader->bGlobalUniformBufferUsed)
	{
		CommitComputeShaderConstants();
	}

	CommitComputeResourceTables(ComputeShader);
	StateCache.ApplyState<CPT_Compute>();

	GraphicsCommandList()->Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
}

void CommandContext::SetShaderTexture(const platform::Render::ComputeHWShader* Shader, uint32 TextureIndex, platform::Render::Texture* ITexture)
{
	auto SRV = dynamic_cast_texture(ITexture)->RetriveShaderResourceView();

	StateCache.SetShaderResourceView<ShaderType::ComputeShader>(SRV, TextureIndex);
}

void CommandContext::SetShaderSampler(const platform::Render::ComputeHWShader* Shader, uint32 SamplerIndex, const platform::Render::TextureSampleDesc& Desc)
{
	auto pSampler = GetParentDevice()->CreateSampler(Convert(Desc));

	StateCache.SetSamplerState<ShaderType::ComputeShader>(pSampler.get(), SamplerIndex);
}

void CommandContext::SetUAVParameter(const platform::Render::ComputeHWShader* Shader, uint32 UAVIndex, platform::Render::UnorderedAccessView* IUAV)
{
	auto UAV = static_cast<UnorderedAccessView*>(IUAV);

	uint32 InitialCount = -1;

	StateCache.SetUAVs<ShaderType::ComputeShader>(UAVIndex, 1, &UAV, &InitialCount);
}

void CommandContext::SetUAVParameter(const platform::Render::ComputeHWShader* Shader, uint32 UAVIndex, platform::Render::UnorderedAccessView* IUAV, uint32 InitialCount)
{
	auto UAV = static_cast<UnorderedAccessView*>(IUAV);

	//Clear ShaderResource

	StateCache.SetUAVs<ShaderType::ComputeShader>(UAVIndex, 1, &UAV, &InitialCount);
}

void CommandContext::SetComputePipelineState(platform::Render::ComputePipelineState* IComputeState)
{
	auto ComputeState = static_cast<ComputePipelineState*>(IComputeState);

	StateCache.TransitionComputeState(CPT_Compute);

	StateCache.SetComputeShader(ComputeState->ComputeShader);
	StateCache.SetComputePipelineState(ComputeState);
}

void CommandContext::CommitComputeShaderConstants()
{
	StateCache.SetConstantBuffer<ShaderType::ComputeShader>(CSConstantBuffer, bDiscardSharedConstants);
}


void CommandContext::CommitComputeResourceTables(const ComputeHWShader* ComputeShader)
{
	//don't support Unreal ShaderResourceTable
}

void CommandContext::PushEvent(const char16_t* Name, platform::FColor Color)
{
#if USE_PIX
	PIXBeginEvent(GraphicsCommandList().GetNoRefCount(), PIX_COLOR(Color.R, Color.G, Color.B), reinterpret_cast<const wchar_t*>(Name));
#endif
}

void CommandContext::PopEvent()
{
#if USE_PIX
	PIXEndEvent(GraphicsCommandList().GetNoRefCount());
#endif
}

void ContextCommon::FlushCommands(D3DFlushFlags FlushFlags)
{
	wassume(IsDefaultContext());

	if (IsOpen())
	{
		CloseCommandList();
	}

	SyncPointRef SyncPoint;
	SubmissionEventRef SubmissionEvent;

	if (white::has_anyflags(FlushFlags, D3DFlushFlags::WaitForCompletion))
	{
		SyncPoint = SyncPoint::Create(SyncPointType::GPUAndCPU);
		SignalSyncPoint(SyncPoint.Get());
	}

	if (white::has_anyflags(FlushFlags, D3DFlushFlags::WaitForSubmission))
	{
		SubmissionEvent = std::make_shared<D3D12::SubmissionEvent>();
		GetPayload(Phase::Signal)->SubmissionEvent = SubmissionEvent;
	}

	{
		std::vector<D3D12Payload*> LocalPayloads;
		Finalize(LocalPayloads);
		Context::Instance().SubmitPayloads(white::make_span(LocalPayloads));
	}

	if (SyncPoint)
	{
		SyncPoint->Wait();
	}

	if (SubmissionEvent && !SubmissionEvent->ready())
	{
		SubmissionEvent->wait();
	}
}

ContextCommon::~ContextCommon() = default;

void ContextCommon::SignalSyncPoint(SyncPoint* SyncPoint)
{
	if (IsOpen())
	{
		CloseCommandList();
	}

	GetPayload(Phase::Signal)->SyncPointsToSignal.emplace_back(SyncPoint);
}

void ContextCommon::WaitSyncPoint(SyncPoint* SyncPoint)
{
	if (IsOpen())
	{
		CloseCommandList();
	}

	GetPayload(Phase::Wait)->SyncPointsToWait.emplace_back(SyncPoint);
}

void ContextCommon::SignalManualFence(ID3D12Fence* Fence, uint64 Value)
{
	if (IsOpen())
	{
		CloseCommandList();
	}

	GetPayload(Phase::Signal)->FencesToSignal.emplace_back(Fence, Value);
}

void ContextCommon::WaitManualFence(ID3D12Fence* Fence, uint64 Value)
{
	if (IsOpen())
	{
		CloseCommandList();
	}

	GetPayload(Phase::Wait)->FencesToWait.emplace_back(Fence, Value);
}

void ContextCommon::Finalize(std::vector<D3D12Payload*>& OutPayloads)
{
	if (IsOpen())
	{
		CloseCommandList();
	}

	// Collect the context's batch of sync points to wait/signal
	if (!BatchedSyncPoints.ToWait.empty())
	{
		auto* Payload = Payloads.size()
			? Payloads[0]
			: GetPayload(Phase::Wait);

		Payload->SyncPointsToWait.append_range(BatchedSyncPoints.ToWait);
		BatchedSyncPoints.ToWait.clear();
	}

	if (!BatchedSyncPoints.ToSignal.empty())
	{
		GetPayload(Phase::Signal)->SyncPointsToSignal.append_range(BatchedSyncPoints.ToSignal);
		BatchedSyncPoints.ToSignal.clear();
	}

	// Attach the command allocator and query heaps to the last payload.
	// The interrupt thread will release these back to the device object pool.
	if (CommandAllocator)
	{
		GetPayload(Phase::Signal)->AllocatorsToRelease.emplace_back(CommandAllocator);
		CommandAllocator = nullptr;
	}

	ContextSyncPoint = nullptr;

	// Move the list of payloads out of this context
	OutPayloads.append_range(Payloads);
	Payloads.resize(0);
}

void ContextCommon::ConditionalSplitCommandList()
{
	// Start a new command list if the total number of commands exceeds the threshold. Too many commands in a single command list can cause TDRs.
	if (IsOpen() && ActiveQueries == 0 && CmdList->State.NumCommands > (uint32)GD3D12MaxCommandsPerCommandList)
	{
		spdlog::info("Splitting command lists because too many commands have been enqueued already ({} commands)", CmdList->State.NumCommands);
		CloseCommandList();
	}
}

void ContextCommon::WriteGPUEventStackToBreadCrumbData(const char* Name, int32 CRC)
{
}
void ContextCommon::WriteGPUEventToBreadCrumbData(D3D12::BreadcrumbStack* Breadcrumbs, uint32 MarkerIndex, bool bBeginEvent)
{
}
bool ContextCommon::InitPayloadBreadcrumbs()
{
	return false;
}
void ContextCommon::PopGPUEventStackFromBreadCrumbData()
{
}