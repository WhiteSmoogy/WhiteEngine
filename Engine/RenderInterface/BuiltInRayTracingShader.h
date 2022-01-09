#pragma once

#include "BuiltInShader.h"

#define PR_NAMESPACE_BEGIN  namespace platform::Render {
#define PR_NAMESPACE_END }

PR_NAMESPACE_BEGIN
class RayTracingShader;

inline namespace Shader
{
	bool IsRayTracingShader(platform::Render::ShaderType type);


	class BuiltInRayTracingShader :public BuiltInShader
	{
	public:
		using DerivedType = BuiltInRayTracingShader;
	public:
		BuiltInRayTracingShader(const ShaderMetaType::CompiledShaderInitializer& Initializer)
			:BuiltInShader(Initializer)
		{}

		BuiltInRayTracingShader(){}
	private:
	};

	template<typename ShaderContentType>
	requires std::is_base_of_v<BuiltInRayTracingShader, ShaderContentType>
	class RayTracingShaderRef : public ShaderRefBase<ShaderContentType>
	{
	public:
		using base = ShaderRefBase<ShaderContentType>;
		using base::GetShader;

		RayTracingShaderRef(const base& Rhs)
			:base(Rhs.GetShader(),Rhs.GetShaderMap())
		{}

		inline RayTracingShader* GetRayTracingShader() const
		{
			return base::template GetShaderBase<RayTracingShader>(GetShader()->GetShaderType());
		}
	};
}

#define EXPORTED_RAYTRACING_SHADER(ShaderClass) EXPORTED_SHADER_TYPE(ShaderClass,BuiltIn) \
	static inline const ShaderParametersMetadata* GetRootParametersMetadata() {return platform::Render::ParametersMetadata<ShaderClass>();}\
	static constexpr bool RootParameterStruct = ShaderParametersType<ShaderClass>::HasParameters;

PR_NAMESPACE_END

#undef PR_NAMESPACE_BEGIN
#undef PR_NAMESPACE_END