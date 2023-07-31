#include "ShadowRendering.h"
#include "RenderInterface/BuiltInShader.h"
#include "Runtime/RenderCore/ShaderParamterTraits.hpp"
#include "Runtime/RenderCore/ShaderParameterStruct.h"
#include "Runtime/RenderCore/ShaderTextureTraits.hpp"
#include "RenderInterface/DrawEvent.h"
#include "RenderInterface/ITexture.hpp"
#include "RenderInterface/IContext.h"
#include "RenderInterface/PipelineStateUtility.h"
#include "Runtime/Renderer/ScreenRendering.h"
#include "Core/Math/TranslationMatrix.h"
#include "Core/Math/RotationMatrix.h"
#include "Core/Math/ScaleMatrix.h"
#include "Core/Math/ShadowProjectionMatrix.h"
#include "Core/Math/TranslationMatrix.h"
#include <WBase/smart_ptr.hpp>

using namespace WhiteEngine;
using namespace platform;

class ShadowDepthVS : public Render::BuiltInShader
{
public:
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
		SHADER_PARAMETER(wm::float4x4, ViewMatrix)
		SHADER_PARAMETER(wm::float4x4, ProjectionMatrix)
		SHADER_PARAMETER(wm::float4, ShadowParams)
		END_SHADER_PARAMETER_STRUCT();

	EXPORTED_BUILTIN_SHADER(ShadowDepthVS);
};


class ShadowDepthPS : public Render::BuiltInShader
{
	EXPORTED_BUILTIN_SHADER(ShadowDepthPS);
};

IMPLEMENT_BUILTIN_SHADER(ShadowDepthVS, "ShadowDepthVertexShader.wsl", "Main", platform::Render::VertexShader);
IMPLEMENT_BUILTIN_SHADER(ShadowDepthPS, "ShadowDepthPixelShader.wsl", "Main", platform::Render::PixelShader);

constexpr float CSMShadowDepthBias = 0.1f;

void ProjectedShadowInfo::SetupWholeSceneProjection(const SceneInfo& scene, const WholeSceneProjectedShadowInitializer& initializer, uint32 InResolutionX, uint32 InResoultionY, uint32 InBorderSize)
{
	PreShadowTranslation = initializer.ShadowTranslation;

	ResolutionX = InResolutionX;
	ResolutionY = InResoultionY;
	BorderSize = InBorderSize;
	CascadeSettings = initializer.CascadeSettings;

	const auto WorldToLightScaled = initializer.WorldToLight * ScaleMatrix(initializer.Scales);

	MaxSubjectZ = wm::transformpoint(initializer.SubjectBounds.Origin, WorldToLightScaled).z + initializer.SubjectBounds.Radius;

	MinSubjectZ = (MaxSubjectZ - initializer.SubjectBounds.Radius * 2);

	const float DepthRange = 200;
	float MinRange = white::math::length(scene.AABBMax - scene.AABBMin);
	MinRange = std::min(DepthRange, MinRange);
	MaxSubjectZ = std::max(MaxSubjectZ, MinRange);
	MinSubjectZ = std::min(MinSubjectZ, -MinRange);

	SubjectAndReceiverMatrix = WorldToLightScaled * ShadowProjectionMatrix(MinSubjectZ, MaxSubjectZ, initializer.WAxis);

	auto FarZPoint = wm::transform(wm::float4(0, 0, 1, 0), wm::inverse(WorldToLightScaled)) * MaxSubjectZ;
	float MaxSubjectDepth = wm::transformpoint(
		initializer.SubjectBounds.Origin.yzx+FarZPoint.xyz
		, SubjectAndReceiverMatrix
	).z;

	InvMaxSubjectDepth = 1.0f / MaxSubjectDepth;

	ShadowBounds = Sphere(-initializer.ShadowTranslation, initializer.SubjectBounds.Radius);

	ShadowViewMatrix = initializer.WorldToLight;

	//ShadowDepthBias
	float DepthBias = 0;
	float SlopeSclaedDepthBias = 1;

	DepthBias = CSMShadowDepthBias / (MaxSubjectZ - MinSubjectZ);
	const float WorldSpaceTexelSize = ShadowBounds.W / ResolutionX;

	DepthBias = wm::lerp(DepthBias, DepthBias * WorldSpaceTexelSize, CascadeSettings.ShadowCascadeBiasDistribution);
	DepthBias *= 0.5f;

	SlopeSclaedDepthBias = WorldSpaceTexelSize;
	SlopeSclaedDepthBias *= 0.5f;

	ShaderDepthBias = DepthBias;
	ShaderSlopeDepthBias = std::max(SlopeSclaedDepthBias, 0.0f);
	ShaderMaxSlopeDepthBias = 1.f;
}


wr::GraphicsPipelineStateInitializer ProjectedShadowInfo::SetupShadowDepthPass(wr::CommandList& CmdList,wr::Texture2D* Target)
{
	DepthTarget = Target;

	wr::GraphicsPipelineStateInitializer psoInit;

	CmdList.FillRenderTargetsInfo(psoInit);

	CmdList.SetViewport(
		X + BorderSize, 
		Y + BorderSize,
		0, 
		X + BorderSize +ResolutionX,
		Y + BorderSize +ResolutionY,
		1);

	auto VertexShader = Render::GetBuiltInShaderMap()->GetShader<ShadowDepthVS>();
	auto PixelShader = Render::GetBuiltInShaderMap()->GetShader<ShadowDepthPS>();

	ShadowDepthVS::Parameters Parameters;

	Parameters.ProjectionMatrix =wm::transpose(TranslationMatrix(PreShadowTranslation) * SubjectAndReceiverMatrix);
	Parameters.ViewMatrix = wm::transpose(ShadowViewMatrix);
	Parameters.ShadowParams = wm::float4(
		ShaderDepthBias, ShaderSlopeDepthBias, ShaderMaxSlopeDepthBias,InvMaxSubjectDepth
	);

	auto ShadowDepthPassUniformBuffer = Render::CreateGraphicsBuffeImmediate(Parameters, Render::Buffer::Usage::SingleFrame);
	CmdList.SetShaderConstantBuffer(VertexShader.GetVertexShader(), 0, ShadowDepthPassUniformBuffer.Get());

	psoInit.ShaderPass.VertexShader = VertexShader.GetVertexShader();
	psoInit.ShaderPass.PixelShader = PixelShader.GetPixelShader();

	// Disable color writes
	psoInit.BlendState.color_write_mask[0] = 0;

	return psoInit;;
}

wm::vector2<int> ProjectedShadowInfo::GetShadowBufferResolution() const
{
	return { DepthTarget->GetWidth(0),DepthTarget->GetHeight(0) };
}

wm::float4x4 ProjectedShadowInfo::GetScreenToShadowMatrix(const SceneInfo& scene, uint32 TileOffsetX, uint32 TileOffsetY, uint32 TileResolutionX, uint32 TileResolutionY) const
{
	auto ShadowBufferRes = GetShadowBufferResolution();

	const auto InvResX = 1.0f / ShadowBufferRes.x;
	const auto InvResY = 1.0f / ShadowBufferRes.y;

	//pixel coord -> texel coord
	//[-1,1] -> [0,1]
	const auto ResFractionX = 0.5f * InvResX * TileResolutionX;
	const auto ResFractionY = 0.5f * InvResY * TileResolutionY;

	wm::float4x4 ViewDependeTransform =
		//Z of the position being transformed is actually view space Z
		wm::float4x4(
			wm::float4(1, 0, 0, 0),
			wm::float4(0, 1, 0, 0),
			wm::float4(0, 0, scene.Matrices.GetProjectionMatrix()[2][2], 1),
			wm::float4(0, 0, scene.Matrices.GetProjectionMatrix()[3][2], 0)
		) *
		scene.Matrices.GetInvViewProjectionMatrix();

	wm::float4x4 ShadowMapDependentTransform =
		// Translate to the origin of the shadow's translated world space
		TranslationMatrix(PreShadowTranslation)
		* SubjectAndReceiverMatrix*
		// Scale and translate x and y to be texture coordinates into the ShadowInfo's rectangle in the shadow depth buffer
		wm::float4x4(
			wm::float4(ResFractionX, 0, 0, 0),
			wm::float4(0, -ResFractionY, 0, 0),
			wm::float4(0, 0,1, 0),
			wm::float4(
				(TileOffsetX + BorderSize) * InvResX + ResFractionX,
				(TileOffsetY + BorderSize) * InvResY + ResFractionY,
				0, 1)
		)
		;

	return ViewDependeTransform * ShadowMapDependentTransform;
}

class ShadowProjectionVS : public Render::BuiltInShader
{
	EXPORTED_BUILTIN_SHADER(ShadowProjectionVS);
};


class ShadowProjectionPS : public Render::BuiltInShader
{
public:
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
		SHADER_PARAMETER(wm::float4x4, ScreenToShadow)
		SHADER_PARAMETER(wm::float4, ProjectionDepthBiasParameters)
		SHADER_PARAMETER(float, FadePlaneOffset)
		SHADER_PARAMETER(float, InvFadePlaneLength)
		SHADER_PARAMETER_TEXTURE(wr::Texture2D, ShadowDepthTexture)
		SHADER_PARAMETER_SAMPLER(wr::TextureSampleDesc, ShadowDepthTextureSampler)

		SHADER_PARAMETER(wm::float4, InvDeviceZToWorldZTransform)

		SHADER_PARAMETER_STRUCT_INCLUDE(SceneParameters, SceneParameters)
		END_SHADER_PARAMETER_STRUCT();
	EXPORTED_BUILTIN_SHADER(ShadowProjectionPS);

	class FFadePlane : SHADER_PERMUTATION_BOOL("USE_FADE_PLANE");

	using PermutationDomain = Render::TShaderPermutationDomain<FFadePlane>;

	void Set(wr::CommandList& CmdList, wr::ShaderRef<ShadowProjectionPS> This, const SceneInfo& scene, const ProjectedShadowInfo* ShadowInfo)
	{
		Parameters Parameters;

		Parameters.ScreenToShadow = wm::transpose(ShadowInfo->GetScreenToShadowMatrix(scene));

		Parameters.ShadowDepthTexture = ShadowInfo->DepthTarget;

		Parameters.ShadowDepthTextureSampler.address_mode_u = Parameters.ShadowDepthTextureSampler.address_mode_v = wr::TexAddressingMode::Clamp;
		Parameters.ProjectionDepthBiasParameters = wm::float4(
			ShadowInfo->GetShaderDepthBias(), ShadowInfo->GetShaderSlopeDepthBias(), ShadowInfo->GetShaderReceiverDepthBias(), ShadowInfo->MaxSubjectZ - ShadowInfo->MinSubjectZ
		);

		Parameters.FadePlaneOffset = ShadowInfo->CascadeSettings.FadePlaneOffset;
		if(ShadowInfo->CascadeSettings.FadePlaneLength > 0)
			Parameters.InvFadePlaneLength = 1.0f /ShadowInfo->CascadeSettings.FadePlaneLength;

		Parameters.SceneParameters = scene.GetParameters();
		Parameters.InvDeviceZToWorldZTransform = CreateInvDeviceZToWorldZTransform(scene.Matrices.GetProjectionMatrix());

		wr::SetShaderParameters(CmdList, This, This.GetPixelShader(), Parameters);
	}
};

IMPLEMENT_BUILTIN_SHADER(ShadowProjectionVS, "ShadowProjectionVertexShader.wsl", "Main", platform::Render::VertexShader);
IMPLEMENT_BUILTIN_SHADER(ShadowProjectionPS, "ShadowProjectionPixelShader.wsl", "Main", platform::Render::PixelShader);

void ProjectedShadowInfo::RenderProjection(wr::CommandList& CmdList, const SceneInfo& scene)
{
	SCOPED_GPU_EVENTF(CmdList, "WholeScene split%d",CascadeSettings.ShadowSplitIndex);

	wr::GraphicsPipelineStateInitializer psoInit;

	CmdList.FillRenderTargetsInfo(psoInit);

	psoInit.DepthStencilState.depth_enable = false;
	psoInit.DepthStencilState.depth_func = wr::CompareOp::Pass;

	psoInit.BlendState.blend_enable[0] = true;
	psoInit.BlendState.src_blend[0] = wr::BlendFactor::Src_Alpha;
	psoInit.BlendState.dst_blend[0] = wr::BlendFactor::Inv_Src_Alpha;

	psoInit.Primitive = wr::PrimtivteType::TriangleStrip;
	psoInit.ShaderPass.VertexDeclaration =wr::VertexDeclarationElements { 
		wr::CtorVertexElement(0,0,wr::Vertex::Usage::Position,0,wr::EF_ABGR32F,sizeof(wm::float4))
	};

	//BindShadowProjectionShaders
	ShadowProjectionPS::PermutationDomain PermutationVector;
	PermutationVector.Set < ShadowProjectionPS::FFadePlane>(CascadeSettings.FadePlaneLength > 0);

	auto PixelShader = wr::GetBuiltInShaderMap()->GetShader< ShadowProjectionPS>(PermutationVector);
	auto VertexShader = wr::GetBuiltInShaderMap()->GetShader< ShadowProjectionVS>();
	psoInit.ShaderPass.VertexShader = VertexShader.GetVertexShader();
	psoInit.ShaderPass.PixelShader = PixelShader.GetPixelShader();

	wr::SetGraphicsPipelineState(CmdList, psoInit);

	PixelShader->Set(CmdList, PixelShader, scene, this);

	CmdList.SetVertexBuffer(0, GFullScreenVertexBuffer().get());
	CmdList.DrawPrimitive(0, 0, 2, 1);
}
