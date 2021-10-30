#include "Shader.h"

using namespace platform::Render;

HardwareShader* Shader::ShaderMapResource::CreateShader(int32 ShaderIndex)
{
	wassume(!HWShaders[ShaderIndex].load(std::memory_order_acquire));

	auto HWShader = CreateHWShader(ShaderIndex);

#if D3D_RAYTRACING
	//AddToRayTracingLibrary
#endif

	return HWShader;
}