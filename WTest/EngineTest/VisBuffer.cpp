#include "VisBuffer.h"
#include "RenderInterface/BuiltinShader.h"
#include "Runtime/RenderCore/ShaderTextureTraits.hpp"

using namespace platform::Render;

class FilterTriangleCS :public BuiltInShader
{
public:
	EXPORTED_BUILTIN_SHADER(FilterTriangleCS);

	class CullBackFaceDim : SHADER_PERMUTATION_BOOL("CULL_BACKFACE");
	class CullFrustumFaceDim : SHADER_PERMUTATION_BOOL("CULL_FRUSTUM");

	using PermutationDomain = TShaderPermutationDomain<CullBackFaceDim, CullFrustumFaceDim>;

	static bool ShouldCompilePermutation(const FBuiltInShaderPermutationParameters& Parameters)
	{
		PermutationDomain PermutationVector(Parameters.PermutationId);

		if (!PermutationVector.Get<CullBackFaceDim>() && !PermutationVector.Get<CullFrustumFaceDim>())
		{
			return false;
		}

		return true;
	}

	using uint = white::uint32;

	struct FilterDispatchArgs
	{
		uint IndexStart;
		uint VertexStart;

		uint DrawId;
		uint IndexEnd;
		uint OutpuIndexOffset;
	};

	struct Matrixs
	{
		wm::float4x4 mvp;
	};

	struct ViewArgs
	{
		Matrixs matrixs;
	};

	struct UncompactedDrawArguments
	{
		uint startIndex;
		uint numIndices;
	};

	BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
		SHADER_PARAMETER_CBUFFER(FilterDispatchArgs, Args)
		SHADER_PARAMETER_CBUFFER(ViewArgs, View)
		SHADER_PARAMETER_SRV(ByteAddressBuffer, IndexBuffer)
		SHADER_PARAMETER_SRV(ByteAddressBuffer, PositionBuffer)
		SHADER_PARAMETER_UAV(RWByteAddressBuffer, PositionBuffer)
		SHADER_PARAMETER_UAV(RWStructuredBuffer, UncompactedDrawArgs)
		END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_BUILTIN_SHADER(FilterTriangleCS, "FilterTriangle.wsl", "FilterTriangleCS", platform::Render::ComputeShader);


void VisBufferTest::RenderTrinf(CommandList& CmdList)
{

}