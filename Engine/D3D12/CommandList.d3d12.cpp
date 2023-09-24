#include "NodeDevice.h"
#include "D3DCommandList.h"
#include "CommandContext.h"
#include "NodeQueue.h"

using namespace platform_ex::Windows::D3D12;

static int32 GD3D12ExtraDepthTransitions = 0;
static int32 GD3D12BatchResourceBarriers = 1;

CommandList::ListState::ListState(CommandAllocator* InCommandAllocator)
	:CmdAllocator(InCommandAllocator)
{
}

void CommandList::Close()
{
	if (Interfaces.CopyCommandList)
	{
		CheckHResult(Interfaces.CopyCommandList->Close());
	}
	else
	{
		CheckHResult(Interfaces.GraphicsCommandList->Close());
	}

	State.IsClosed = true;
}

CommandAllocator::CommandAllocator(NodeDevice* InDevice, QueueType InType)
	:Device(InDevice),Type(InType)
{
	CheckHResult(Device->GetDevice()->CreateCommandAllocator(GetD3DCommandListType(Type), COMPtr_RefParam(D3DCommandAllocator, IID_ID3D12CommandAllocator)));
}

CommandAllocator::~CommandAllocator()
{
	D3DCommandAllocator = nullptr;
}

static inline bool IsTransitionNeeded(D3D12_RESOURCE_STATES Before, D3D12_RESOURCE_STATES& After)
{
	wassume(Before != D3D12_RESOURCE_STATE_CORRUPT && After != D3D12_RESOURCE_STATE_CORRUPT);
	wassume(Before != D3D12_RESOURCE_STATE_TBD && After != D3D12_RESOURCE_STATE_TBD);

	// Depth write is actually a suitable for read operations as a "normal" depth buffer.
	if (Before == D3D12_RESOURCE_STATE_DEPTH_WRITE && After == D3D12_RESOURCE_STATE_DEPTH_READ)
	{
		if (GD3D12ExtraDepthTransitions)
		{
			After = D3D12_RESOURCE_STATE_DEPTH_WRITE;
		}
		return false;
	}

	// COMMON is an oddball state that doesn't follow the RESOURE_STATE pattern of 
	// having exactly one bit set so we need to special case these
	if (After == D3D12_RESOURCE_STATE_COMMON)
	{
		// Before state should not have the common state otherwise it's invalid transition
		wassume(Before != D3D12_RESOURCE_STATE_COMMON);
		return true;
	}

	// We should avoid doing read-to-read state transitions. But when we do, we should avoid turning off already transitioned bits,
	// e.g. VERTEX_BUFFER -> SHADER_RESOURCE is turned into VERTEX_BUFFER -> VERTEX_BUFFER | SHADER_RESOURCE.
	// This reduces the number of resource transitions and ensures automatic states from resource bindings get properly combined.
	D3D12_RESOURCE_STATES Combined = Before | After;
	if ((Combined & (D3D12_RESOURCE_STATE_GENERIC_READ | D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT)) == Combined)
	{
		After = Combined;
	}

	return Before != After;
}

bool ContextCommon::TransitionResource(ResourceHolder* Resource, D3D12_RESOURCE_STATES Before, D3D12_RESOURCE_STATES After, const CViewSubset& ViewSubset)
{
	const bool bIsWholeResource = ViewSubset.IsWholeResource();

	CResourceState& ResourceState = GetCommandList().GetResourceState_OnCommandList(Resource);

	bool bRequireUAVBarrier = false;

	if (bIsWholeResource && ResourceState.AreAllSubresourcesSame())
	{
		// Fast path. Transition the entire resource from one state to another.
		bool bForceInAfterState = false;
		bRequireUAVBarrier = TransitionResource(Resource, ResourceState, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, Before, After, bForceInAfterState);
	}
	else
	{
		// Slower path. Either the subresources are in more than 1 state, or the view only partially covers the resource.
		// Either way, we'll need to loop over each subresource in the view...

		bool bWholeResourceWasTransitionedToSameState = bIsWholeResource;
		for (uint32 SubresourceIndex : ViewSubset)
		{
			bool bForceInAfterState = false;
			bRequireUAVBarrier |= TransitionResource(Resource, ResourceState, SubresourceIndex, Before, After, bForceInAfterState);

			// Subresource not in the same state, then whole resource is not in the same state anymore
			if (ResourceState.GetSubresourceState(SubresourceIndex) != After)
				bWholeResourceWasTransitionedToSameState = false;
		}

		// If we just transtioned every subresource to the same state, lets update it's tracking so it's on a per-resource level
		if (bWholeResourceWasTransitionedToSameState)
		{
			// Sanity check to make sure all subresources are really in the 'after' state
			wassume(ResourceState.CheckAllSubresourceSame());
			wassume(white::has_anyflags(ResourceState.GetSubresourceState(D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES), After));
		}
	}

	return bRequireUAVBarrier;
}

bool ContextCommon::TransitionResource(ResourceHolder* Resource, D3D12_RESOURCE_STATES Before, D3D12_RESOURCE_STATES After, uint32 Subresource)
{
	bool bRequireUAVBarrier = false;

	CResourceState& ResourceState = GetCommandList().GetResourceState_OnCommandList(Resource);
	if (Subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES && !ResourceState.AreAllSubresourcesSame())
	{
		// Slow path. Want to transition the entire resource (with multiple subresources). But they aren't in the same state.

		const uint16 SubresourceCount = Resource->GetSubresourceCount();
		for (uint32 SubresourceIndex = 0; SubresourceIndex < SubresourceCount; SubresourceIndex++)
		{
			bool bForceInAfterState = true;
			bRequireUAVBarrier |= TransitionResource(Resource, ResourceState, SubresourceIndex, Before, After, bForceInAfterState);
		}

		// The entire resource should now be in the after state on this command list (even if all barriers are pending)
		wassume(ResourceState.CheckAllSubresourceSame());
		wassume(white::has_anyflags(ResourceState.GetSubresourceState(D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES), After));
	}
	else
	{
		bool bForceInAfterState = false;
		bRequireUAVBarrier = TransitionResource(Resource, ResourceState, Subresource, Before, After, bForceInAfterState);
	}

	return bRequireUAVBarrier;
}

void ContextCommon::TransitionResource(DepthStencilView* View)
{
	// Determine the required subresource states from the view desc
	const D3D12_DEPTH_STENCIL_VIEW_DESC& DSVDesc = View->GetDesc();
	const bool bDSVDepthIsWritable = (DSVDesc.Flags & D3D12_DSV_FLAG_READ_ONLY_DEPTH) == 0;
	const bool bDSVStencilIsWritable = (DSVDesc.Flags & D3D12_DSV_FLAG_READ_ONLY_STENCIL) == 0;
	// TODO: Check if the PSO depth stencil is writable. When this is done, we need to transition in SetDepthStencilState too.

	// This code assumes that the DSV always contains the depth plane
	const bool bHasDepth = true;
	const bool bHasStencil = View->HasStencil();
	const bool bDepthIsWritable = bHasDepth && bDSVDepthIsWritable;
	const bool bStencilIsWritable = bHasStencil && bDSVStencilIsWritable;

	// DEPTH_WRITE is suitable for read operations when used as a normal depth/stencil buffer.
	auto pResource = View->GetResource();
	if (bDepthIsWritable)
	{
		TransitionResource(pResource, D3D12_RESOURCE_STATE_TBD, D3D12_RESOURCE_STATE_DEPTH_WRITE, View->GetDepthOnlySubset());
	}
	if (bStencilIsWritable)
	{
		TransitionResource(pResource, D3D12_RESOURCE_STATE_TBD, D3D12_RESOURCE_STATE_DEPTH_WRITE, View->GetStencilOnlySubset());
	}
}

void ContextCommon::TransitionResource(DepthStencilView* pView, D3D12_RESOURCE_STATES after)
{
	auto pResource = pView->GetResource();
	auto ResourceDesc = pResource->GetDesc();

	const D3D12_DEPTH_STENCIL_VIEW_DESC& desc = pView->GetDesc();
	switch (desc.ViewDimension)
	{
	case D3D12_DSV_DIMENSION_TEXTURE2D:
	case D3D12_DSV_DIMENSION_TEXTURE2DMS:
		if (GetPlaneCount(ResourceDesc.Format) > 1)
		{
			// Multiple subresources to transtion
			TransitionResource(pResource, D3D12_RESOURCE_STATE_TBD, after, pView->GetViewSubset());
			break;
		}
		else
		{
			// Only one subresource to transition
			wconstraint(GetPlaneCount(ResourceDesc.Format) == 1);
			TransitionResource(pResource, D3D12_RESOURCE_STATE_TBD, after, desc.Texture2D.MipSlice);
		}
		break;

	case D3D12_DSV_DIMENSION_TEXTURE2DARRAY:
	case D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY:
	{
		// Multiple subresources to transtion
		TransitionResource(pResource, D3D12_RESOURCE_STATE_TBD, after, pView->GetViewSubset());
		break;
	}

	default:
		wconstraint(false);	// Need to update this code to include the view type
		break;
	}
}

void ContextCommon::TransitionResource( RenderTargetView* pView, D3D12_RESOURCE_STATES after)
{
	auto pResource = pView->GetResource();

	const D3D12_RENDER_TARGET_VIEW_DESC& desc = pView->GetDesc();
	switch (desc.ViewDimension)
	{
	case D3D12_RTV_DIMENSION_TEXTURE3D:
		// Note: For volume (3D) textures, all slices for a given mipmap level are a single subresource index.
		// Fall-through
	case D3D12_RTV_DIMENSION_TEXTURE2D:
	case D3D12_RTV_DIMENSION_TEXTURE2DMS:
		// Only one subresource to transition
		TransitionResource(pResource, D3D12_RESOURCE_STATE_TBD,  after, desc.Texture2D.MipSlice);
		break;

	case D3D12_RTV_DIMENSION_TEXTURE2DARRAY:
	{
		// Multiple subresources to transition
		TransitionResource(pResource, D3D12_RESOURCE_STATE_TBD, after, pView->GetViewSubset());
		break;
	}

	default:
		wconstraint(false);	// Need to update this code to include the view type
		break;
	}
}

void ContextCommon::TransitionResource(UnorderedAccessView* View, D3D12_RESOURCE_STATES After)
{
	auto* Resource = View->GetResource();

	const D3D12_UNORDERED_ACCESS_VIEW_DESC& Desc = View->GetDesc();
	switch (Desc.ViewDimension)
	{
	case D3D12_UAV_DIMENSION_BUFFER:
		TransitionResource(Resource, D3D12_RESOURCE_STATE_TBD, After, 0);
		break;

	case D3D12_UAV_DIMENSION_TEXTURE2D:
		// Only one subresource to transition
		TransitionResource(Resource, D3D12_RESOURCE_STATE_TBD, After, Desc.Texture2D.MipSlice);
		break;

	case D3D12_UAV_DIMENSION_TEXTURE2DARRAY:
		// Multiple subresources to transtion
		TransitionResource(Resource, D3D12_RESOURCE_STATE_TBD, After, View->GetViewSubset());
		break;

	case D3D12_UAV_DIMENSION_TEXTURE3D:
		// Multiple subresources to transtion
		TransitionResource(Resource, D3D12_RESOURCE_STATE_TBD, After, View->GetViewSubset());
		break;

	default:
		wconstraint(false);
		break;
	}
}

void ContextCommon::TransitionResource(ShaderResourceView* View, D3D12_RESOURCE_STATES After)
{
	auto* Resource = View->GetResource();

	// Early out if we never need to do state tracking, the resource should always be in an SRV state.
	if (!Resource || !Resource->RequiresResourceStateTracking())
		return;

	const D3D12_RESOURCE_DESC& ResDesc = Resource->GetDesc();
	const auto& ViewSubset = View->GetViewSubset();

	const D3D12_SHADER_RESOURCE_VIEW_DESC& Desc = View->GetDesc();
	switch (Desc.ViewDimension)
	{
	default:
		// Transition the resource
		TransitionResource(Resource, D3D12_RESOURCE_STATE_TBD, After, ViewSubset);
		break;

	case D3D12_SRV_DIMENSION_BUFFER:
		if (Resource->GetHeapType() == D3D12_HEAP_TYPE_DEFAULT)
		{
			// Transition the resource
			TransitionResource(Resource, D3D12_RESOURCE_STATE_TBD, After, ViewSubset);
		}
		break;
	}
}

bool ContextCommon::TransitionResource(ResourceHolder* InResource, CResourceState& ResourceState_OnCommandList, uint32 InSubresourceIndex, D3D12_RESOURCE_STATES InBeforeState, D3D12_RESOURCE_STATES InAfterState, bool bInForceAfterState)
{
	// Try and get the correct D3D before state for the transition
	D3D12_RESOURCE_STATES const TrackedState = ResourceState_OnCommandList.GetSubresourceState(InSubresourceIndex);
	D3D12_RESOURCE_STATES BeforeState = TrackedState != D3D12_RESOURCE_STATE_TBD ? TrackedState : InBeforeState;

	// Make sure the before states match up or are unknown
	wassume(InBeforeState == D3D12_RESOURCE_STATE_TBD || BeforeState == InBeforeState);

	bool bRequireUAVBarrier = false;
	if (BeforeState != D3D12_RESOURCE_STATE_TBD)
	{
		bool bApplyTransitionBarrier = true;

		// Require UAV barrier when before and after are UAV
		if (BeforeState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS && InAfterState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
		{
			bRequireUAVBarrier = true;
		}
		// Special case for UAV access resources
		else if (InResource->GetUAVAccessResource() && white::has_anyflags(BeforeState | InAfterState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS))
		{
			// inject an aliasing barrier
			const bool bFromUAV = white::has_anyflags(BeforeState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			const bool bToUAV = white::has_anyflags(InAfterState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

			AddAliasingBarrier(
				bFromUAV ? InResource->GetUAVAccessResource() : InResource->Resource(),
				bToUAV ? InResource->GetUAVAccessResource() : InResource->Resource());

			if (bToUAV)
			{
				ResourceState_OnCommandList.SetUAVHiddenResourceState(BeforeState);
				bApplyTransitionBarrier = false;
			}
			else
			{
				D3D12_RESOURCE_STATES HiddenState = ResourceState_OnCommandList.GetUAVHiddenResourceState();

				// Still unknown in this command list?
				if (HiddenState == D3D12_RESOURCE_STATE_TBD)
				{
					AddPendingResourceBarrier(InResource, InAfterState, InSubresourceIndex, ResourceState_OnCommandList);
					bApplyTransitionBarrier = false;
				}
				else
				{
					// Use the hidden state as the before state on the resource
					BeforeState = HiddenState;
				}
			}
		}

		if (bApplyTransitionBarrier)
		{
			// We're not using IsTransitionNeeded() when bInForceAfterState is set because we do want to transition even if 'after' is a subset of 'before'
			// This is so that we can ensure all subresources are in the same state, simplifying future barriers
			// No state merging when using engine transitions - otherwise next before state might not match up anymore)
			if ((bInForceAfterState && BeforeState != InAfterState) || IsTransitionNeeded(BeforeState, InAfterState))
			{
				AddTransitionBarrier(InResource, BeforeState, InAfterState, InSubresourceIndex, &ResourceState_OnCommandList);
			}
			else
			{
				// Ensure the command list tracking is up to date, even if we skipped an unnecessary transition.
				ResourceState_OnCommandList.SetSubresourceState(InSubresourceIndex, InAfterState);
			}
		}
	}
	else
	{
		// BeforeState is TBD. We need a pending resource barrier.

		// Special handling for UAVAccessResource and transition to UAV - don't want to enqueue pending resource to UAV because the actual resource won't transition
		// Adding of patch up will only be done when transitioning to non UAV state
		if (InResource->GetUAVAccessResource() && white::has_anyflags(InAfterState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS))
		{
			AddAliasingBarrier(InResource->Resource(), InResource->GetUAVAccessResource());
			ResourceState_OnCommandList.SetUAVHiddenResourceState(D3D12_RESOURCE_STATE_TBD);
		}
		else
		{
			// We need a pending resource barrier so we can setup the state before this command list executes
			AddPendingResourceBarrier(InResource, InAfterState, InSubresourceIndex, ResourceState_OnCommandList);
		}
	}

	return bRequireUAVBarrier;
}

void ContextCommon::AddAliasingBarrier(ID3D12Resource* InResourceBefore, ID3D12Resource* InResourceAfter)
{
	ResourceBarrierBatcher.AddAliasingBarrier(InResourceBefore, InResourceAfter);

	if (!GD3D12BatchResourceBarriers)
	{
		FlushResourceBarriers();
	}
}

void ContextCommon::AddPendingResourceBarrier(ResourceHolder* Resource, D3D12_RESOURCE_STATES After, uint32 SubResource, CResourceState& ResourceState_OnCommandList)
{
	wassume(After != D3D12_RESOURCE_STATE_TBD);
	wassume(Resource->RequiresResourceStateTracking());
	wassume(&GetCommandList().GetResourceState_OnCommandList(Resource) == &ResourceState_OnCommandList);

	GetCommandList().State.PendingResourceBarriers.emplace_back(Resource, After, SubResource);
	ResourceState_OnCommandList.SetSubresourceState(SubResource, After);
}

void ContextCommon::AddTransitionBarrier(ResourceHolder* pResource, D3D12_RESOURCE_STATES Before, D3D12_RESOURCE_STATES After, uint32 Subresource, CResourceState* ResourceState_OnCommandList)
{
	if (Before != After)
	{
		ResourceBarrierBatcher.AddTransition(pResource, Before, After, Subresource);

		if (!GD3D12BatchResourceBarriers)
		{
			FlushResourceBarriers();
		}
	}
	else
	{
		WAssert(false, "AddTransitionBarrier: Before == After");
	}

	if (pResource->RequiresResourceStateTracking())
	{
		if (!ResourceState_OnCommandList)
		{
			ResourceState_OnCommandList = &GetCommandList().GetResourceState_OnCommandList(pResource);
		}
		else
		{
			// If a resource state was passed to avoid a repeat lookup, it must be the one we expected.
			wassume(&GetCommandList().GetResourceState_OnCommandList(pResource) == ResourceState_OnCommandList);
		}

		ResourceState_OnCommandList->SetSubresourceState(Subresource, After);
		ResourceState_OnCommandList->SetHasInternalTransition();
	}
}

void ContextCommon::AddUAVBarrier()
{
	ResourceBarrierBatcher.AddUAV();

	if (!GD3D12BatchResourceBarriers)
	{
		FlushResourceBarriers();
	}
}

void ContextCommon::FlushResourceBarriers()
{
	if (!ResourceBarrierBatcher.IsEmpty())
	{
		ResourceBarrierBatcher.Flush(GetCommandList());
	}
}

void ResourceBarrierBatcher::Flush(CommandList& CommandList)
{
	if (!Barriers.empty())
	{
		CommandList.GraphicsCommandList()->ResourceBarrier(static_cast<UINT>(Barriers.size()), Barriers.data());
		Reset();
	}
}