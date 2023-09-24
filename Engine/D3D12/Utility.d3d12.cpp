#include "Utility.h"
#include "Fence.h"
#include "HardwareShader.h"
#include "RootSignature.h"

using namespace platform_ex::Windows::D3D12;

void platform_ex::Windows::D3D12::QuantizeBoundShaderState(const D3D12_RESOURCE_BINDING_TIER& ResourceBindingTier, const ComputeHWShader* const ComputeShader, QuantizedBoundShaderState& OutQBSS)
{
    std::memset(&OutQBSS, 0, sizeof(OutQBSS));

    QuantizedBoundShaderState::InitShaderRegisterCounts(ResourceBindingTier, ComputeShader->ResourceCounts, OutQBSS.RegisterCounts[ShaderType::VisibilityAll], true);
}
