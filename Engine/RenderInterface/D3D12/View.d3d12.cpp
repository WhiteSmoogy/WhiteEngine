#include "View.h"
#include "GraphicsBuffer.hpp"

using namespace platform_ex::Windows::D3D12;

ShaderResourceView::ShaderResourceView(GraphicsBuffer* InBuffer, const D3D12_SHADER_RESOURCE_VIEW_DESC& InDesc, uint32 InStride)
	:TView(InBuffer->GetParentDevice(), ViewSubresourceSubsetFlags_None)
{
	Initialize(InDesc, InBuffer, InBuffer->Location, InStride);
}
