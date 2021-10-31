#pragma once

#include "../BuiltInRayTracingShader.h"
#include "../IRayContext.h"
#include "Engine/Render/ShaderParamterTraits.hpp"
#include "Engine/Render/ShaderTextureTraits.hpp"
#include "Engine/Render/ShaderParameterStruct.h"
#include <list>

namespace platform_ex::Windows::D3D12 {

	using namespace platform::Render::Shader;

	template<typename ShaderContentType>
	inline platform::Render::RayTracingShader* GetBuildInRayTracingShader()
	{
		auto ShaderMap = GetBuiltInShaderMap();

		RayTracingShaderRef<ShaderContentType> Shader{ ShaderMap->GetShader<ShaderContentType>() };

		auto RayTracingShader = Shader.GetRayTracingShader();

		return RayTracingShader;
	}

	class DefaultCHS :public BuiltInRayTracingShader
	{
		EXPORTED_RAYTRACING_SHADER(DefaultCHS);
	};

	class DefaultMS :public BuiltInRayTracingShader
	{
		EXPORTED_RAYTRACING_SHADER(DefaultMS);
	};

	class ShadowRG : public BuiltInRayTracingShader
	{
	public:
		using Parameters = platform::Render::ShadowRGParameters;

		EXPORTED_RAYTRACING_SHADER(ShadowRG);
	};
}
