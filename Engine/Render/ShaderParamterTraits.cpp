#include "ShaderParamterTraits.hpp"

using namespace white::math;
using namespace platform::Render::Shader;

namespace {
	
	BEGIN_SHADER_PARAMETER_STRUCT(A)
		SHADER_PARAMETER(float2, a)
		SHADER_PARAMETER(float3,b)
		END_SHADER_PARAMETER_STRUCT();

	MS_ALIGN(VariableBoundary) struct RefA
	{
		__declspec(align(4)) float2 a;
		__declspec(align(16)) float3 b;
	};

	static_assert(woffsetof(RefA, b) == woffsetof(A, b));

	BEGIN_SHADER_PARAMETER_STRUCT(B)
		SHADER_PARAMETER(float, a)
		SHADER_PARAMETER(float3, b)
		END_SHADER_PARAMETER_STRUCT();

	MS_ALIGN(VariableBoundary) struct RefB
	{
		__declspec(align(4)) float a;
		__declspec(align(4)) float3 b;
	};

	static_assert(woffsetof(RefB, b) == woffsetof(B, b));
}