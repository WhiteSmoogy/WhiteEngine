#include "BuiltInRayTracingShaders.h"
#include "RayTracingShader.h"
#include "../../Asset/ShaderAsset.h"
#include "../../Asset/D3DShaderCompiler.h"
#include "Context.h"
#include "RayDevice.h"

using namespace platform_ex::Windows::D3D12;


IMPLEMENT_BUILTIN_SHADER(DefaultCHS, "RayTracing/RayTracingBuiltInShaders.wsl", "DefaultCHS", platform::Render::RayHitGroup);

IMPLEMENT_BUILTIN_SHADER(DefaultMS, "RayTracing/RayTracingBuiltInShaders.wsl", "DefaultMS", platform::Render::RayMiss);

IMPLEMENT_BUILTIN_SHADER(ShadowRG, "RayTracing/RayTracingScreenSpaceShadow.wsl", "RayGen", platform::Render::RayGen);