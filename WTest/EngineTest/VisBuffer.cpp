#include "VisBuffer.h"
#include "RenderInterface/BuiltinShader.h"
#include "Runtime/RenderCore/ShaderTextureTraits.hpp"
#include "Runtime/RenderCore/UnifiedBuffer.h"
#include "Runtime/RenderCore/Dispatch.h"
#include "Runtime/Renderer/SceneInfo.h"

using namespace platform::Render;

struct Matrixs
{
	wm::float4x4 mvp;
};

struct ViewArgs
{
	Matrixs matrixs;
};

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

		uint ClusterId;

		uint Pad0;
		uint Pad1;
	};



	struct UncompactedDrawArguments
	{
		uint startIndex;
		uint numIndices;
	};

	BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
		SHADER_PARAMETER_CBUFFER(FilterDispatchArgs[4096], DispatchArgs)
		SHADER_PARAMETER_CBUFFER(ViewArgs, View)
		SHADER_PARAMETER_SRV(ByteAddressBuffer, IndexBuffer)
		SHADER_PARAMETER_SRV(ByteAddressBuffer, PositionBuffer)
		SHADER_PARAMETER_UAV(RWByteAddressBuffer, FliteredIndexBuffer)
		SHADER_PARAMETER_UAV(RWStructuredBuffer<UncompactedDrawArguments>, UncompactedDrawArgs)
		END_SHADER_PARAMETER_STRUCT()

};

IMPLEMENT_BUILTIN_SHADER(FilterTriangleCS, "FilterTriangle.hlsl", "FilterTriangleCS", platform::Render::ComputeShader);


class BatchCompactionCS : public BuiltInShader
{
public:
	EXPORTED_BUILTIN_SHADER(BatchCompactionCS);

	constexpr static const uint32 DrawIndexArgumentNumElements = sizeof(DrawIndexArguments) / sizeof(uint32);

	class DrawArgumentDim : SHADER_PERMUTATION_SPARSE_INT("INDIRECT_DRAW_ARGUMENTS_STRUCT_NUM_ELEMENTS", DrawIndexArgumentNumElements);

	using PermutationDomain = TShaderPermutationDomain<DrawArgumentDim>;


	static bool ShouldCompilePermutation(const FBuiltInShaderPermutationParameters& Parameters)
	{
		PermutationDomain PermutationVector(Parameters.PermutationId);

		return true;
	}

	BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
		SHADER_PARAMETER_SRV(StructuredBuffer<FilterTriangleCS::UncompactedDrawArguments>, UncompactedDrawArgs)
		SHADER_PARAMETER_UAV(RWStructuredBuffer<uint32>, IndrectDrawArgsBuffer)
		SHADER_PARAMETER(uint32, MaxDraws)
		END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_BUILTIN_SHADER(BatchCompactionCS, "BatchCompaction.hlsl", "BatchCompactionCS", platform::Render::ComputeShader);

class VisTriangleVS :public BuiltInShader
{
	EXPORTED_BUILTIN_SHADER(VisTriangleVS);

	BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
		SHADER_PARAMETER_CBUFFER(ViewArgs, View)
		END_SHADER_PARAMETER_STRUCT()
};
IMPLEMENT_BUILTIN_SHADER(VisTriangleVS, "VisTriangle.hlsl", "VisTriangleVS", platform::Render::VertexShader);

class VisTrianglePS :public BuiltInShader
{
	EXPORTED_BUILTIN_SHADER(VisTrianglePS);
};
IMPLEMENT_BUILTIN_SHADER(VisTrianglePS, "VisTriangle.hlsl", "VisTrianglePS", platform::Render::PixelShader);


void VisBufferTest::OnGUI()
{
	if (sponza_trinf->State != Trinf::Resources::StreamingState::Resident)
		ImGui::Text("Loading...");
	else
		ImGui::Text("Loaded");
}

using RenderGraph::RGBufferDesc;
using RenderGraph::RGBufferUAVDesc;
using RenderGraph::RGBufferSRVDesc;
using RenderGraph::RGEventName;
using RenderGraph::ERGPassFlags;

void VisBufferTest::RenderTrinf(RenderGraph::RGBuilder& Builder)
{
	auto& device = Context::Instance().GetDevice();

	if (sponza_trinf->State != Trinf::Resources::StreamingState::Resident)
		return;

	auto ViewCB = Builder.CreateCBuffer<ViewArgs>();
	ViewCB->matrixs.mvp = wm::transpose(camera.GetViewMatrix() * projMatrix);

	auto RWStructAccess = EAccessHint::GPUReadWrite | EAccessHint::GPUStructured | EAccessHint::GPUUnordered;

	auto FliteredIndexBuffer = Builder.CreateBuffer(
		RGBufferDesc::CreateByteAddressDesc(
			Trinf::Scene.Index.Allocator.GetMaxSize() * Trinf::Scene.Index.kPageSize),
		"FliteredIndexBuffer");

	auto UncompactedDrawArgsDesc = RGBufferDesc::CreateStructuredDesc(
		sizeof(FilterTriangleCS::UncompactedDrawArguments), sponza_trinf->Metadata->TrinfsCount
	);

	auto UncompactedDrawArgsSize = white::Align(sizeof(FilterTriangleCS::UncompactedDrawArguments) * sponza_trinf->Metadata->TrinfsCount, 16);
	auto UncompactedDrawArgs = Builder.CreateBuffer(UncompactedDrawArgsDesc, "UncompactedDrawArgs");

	auto UncompactedDrawArgsUAV = Builder.CreateUAV(RGBufferUAVDesc{ UncompactedDrawArgs });
	auto UncompactedDrawArgsSRV = Builder.CreateSRV(RGBufferSRVDesc{ UncompactedDrawArgs });

	MemsetResourceParams Params;
	Params.Count = UncompactedDrawArgsDesc.GetSize();
	Params.DstOffset = 0;
	Params.Value = 0;
	MemsetResource(Builder, UncompactedDrawArgsUAV, Params);

	auto Parameters = Builder.AllocParameters<FilterTriangleCS::Parameters>();

	auto IndexBuffer = Builder.RegisterExternal(Trinf::Scene.Index.DataBuffer, RenderGraph::ERGBufferFlags::MultiFrame);
	auto PositionBuffer = Builder.RegisterExternal(Trinf::Scene.Position.DataBuffer, RenderGraph::ERGBufferFlags::MultiFrame);

	Parameters->View = ViewCB.Get();
	Parameters->IndexBuffer = Builder.CreateSRV({ .Buffer = IndexBuffer });
	Parameters->PositionBuffer = Builder.CreateSRV({ .Buffer = PositionBuffer });
	Parameters->FliteredIndexBuffer = Builder.CreateUAV({ .Buffer = FliteredIndexBuffer });
	Parameters->UncompactedDrawArgs = UncompactedDrawArgsUAV;

	const int MaxDispatchCount = 2048;
	auto BatchTrinfArgs = Builder.AllocParameters<FilterTriangleCS::FilterDispatchArgs>(MaxDispatchCount);

	FilterTriangleCS::PermutationDomain PermutationVector;
	PermutationVector.Set<FilterTriangleCS::CullBackFaceDim >(true);
	PermutationVector.Set<FilterTriangleCS::CullFrustumFaceDim>(true);
	auto ComputeShader = GetBuiltInShaderMap()->GetShader<FilterTriangleCS>(PermutationVector);

	int dispatchCount = 0;
	auto DipstachBatch = [&]()
		{
			if (dispatchCount > 0)
			{
				auto ArgCB = Builder.CreateCBuffer(BatchTrinfArgs);

				Parameters->DispatchArgs = ArgCB;

				ComputeShaderUtils::AddPass(
					Builder,
					RGEventName("DipstachBatch(Count:{})", dispatchCount),
					ComputeShader,
					Parameters,
					white::math::int3(dispatchCount, 1, 1));

				dispatchCount = 0;
			}
		};

	auto AddBatch = [&](int drawId, const platform_ex::DSFileFormat::TriinfMetadata& trinf)
		{
			FilterTriangleCS::FilterDispatchArgs args;

			auto& cluster_compact = trinf.ClusterCompacts[0];
			auto& last_compact = trinf.ClusterCompacts[trinf.ClusterCount - 1];

			args.DrawId = drawId;
			args.IndexStart = cluster_compact.ClusterStart;
			auto MaxTriangle = (trinf.ClusterCount - 1) * 64 + last_compact.TriangleCount;
			args.IndexEnd = args.IndexStart + MaxTriangle * 3;

			args.VertexStart = 0;
			args.OutpuIndexOffset = args.IndexStart;

			for (uint32 clusterIndex = 0; clusterIndex < trinf.ClusterCount; ++clusterIndex)
			{
				args.ClusterId = clusterIndex;
				BatchTrinfArgs[dispatchCount] = args;

				if (dispatchCount >= MaxDispatchCount)
					DipstachBatch();
			}
		};

	for (uint32 i = 0; i < sponza_trinf->Metadata->TrinfsCount; ++i)
	{
		auto& trinf = sponza_trinf->Metadata->Trinfs[i];

		if (BatchTrinfArgs.size() + trinf.ClusterCount > MaxDispatchCount)
		{
			DipstachBatch();
			AddBatch(i, trinf);
		}
		else
		{
			AddBatch(i, trinf);
		}
	}

	DipstachBatch();

	auto CompactedDrawArgsSize = white::Align(sizeof(DrawIndexArguments) * (sponza_trinf->Metadata->TrinfsCount + 1), 16);
	auto CompactedDrawArgs = Builder.CreateBuffer(RGBufferDesc::CreateStructIndirectDesc(sizeof(DrawIndexArguments), sponza_trinf->Metadata->TrinfsCount + 1), "CompactedDrawArgs");
	auto CompactedDrawArgsUAV = Builder.CreateUAV({ .Buffer = CompactedDrawArgs });
	Params.Count = white::Align(sizeof(DrawIndexArguments), 16);
	Params.DstOffset = 0;
	Params.Value = 0;
	MemsetResource(Builder, CompactedDrawArgsUAV, Params);

	{
		auto compactionPars = Builder.AllocParameters<BatchCompactionCS::Parameters>();
		compactionPars->IndrectDrawArgsBuffer = CompactedDrawArgsUAV;
		compactionPars->MaxDraws = sponza_trinf->Metadata->TrinfsCount;
		compactionPars->UncompactedDrawArgs = UncompactedDrawArgsSRV;

		BatchCompactionCS::PermutationDomain compactPV;

		compactPV.Set<BatchCompactionCS::DrawArgumentDim >(BatchCompactionCS::DrawIndexArgumentNumElements);
		auto compactCS = GetBuiltInShaderMap()->GetShader<BatchCompactionCS>(compactPV);

		int dispatchCount = static_cast<int>((sponza_trinf->Metadata->TrinfsCount + 255) / 256);

		ComputeShaderUtils::AddPass(Builder,
			RGEventName("Compact(Count:{})", dispatchCount),
			compactCS,
			compactionPars,
			white::math::int3(dispatchCount, 1, 1));
	}

	auto& screen_frame = render::Context::Instance().GetScreenFrame();



	auto depth_tex = static_cast<render::Texture2D*>(screen_frame->Attached(render::FrameBuffer::DepthStencil));

	auto VSParas = Builder.AllocParameters<VisTriangleVS::Parameters>();

	Builder.AddPass(
		RGEventName("VisBuffer"),
		VSParas,
		white::enum_or(ERGPassFlags::Raster, ERGPassFlags::NeverCull),
		[=](CommandList& CmdList)
		{
			render::RenderPassInfo visPass(
				vis_buffer.get(), render::Clear_Store,
				depth_tex, render::DepthStencilTargetActions::ClearDepthStencil_StoreDepthStencil
			);

			CmdList.BeginRenderPass(visPass, "VisBuffer");

			auto VisTriVS = GetBuiltInShaderMap()->GetShader<VisTriangleVS>();
			VSParas->View = ViewCB.Get();

			auto VisTriPS = GetBuiltInShaderMap()->GetShader<VisTrianglePS>();

			// Setup pipelinestate
			render::GraphicsPipelineStateInitializer VisPso{};
			CmdList.FillRenderTargetsInfo(VisPso);

			VisPso.ShaderPass.VertexShader = VisTriVS.GetVertexShader();
			VisPso.ShaderPass.VertexDeclaration.push_back(
				CtorVertexElement(0, 0, Vertex::Usage::Position, 0, EF_BGR32F, sizeof(wm::float3)));
			VisPso.ShaderPass.PixelShader = VisTriPS.GetPixelShader();

			SetGraphicsPipelineState(CmdList, VisPso);
			render::SetShaderParameters(CmdList, VisTriVS, VisTriVS.GetVertexShader(), *VSParas);

			CmdList.SetVertexBuffer(0, Trinf::Scene.Position.DataBuffer->GetRObject());

			CmdList.SetIndexBuffer(FliteredIndexBuffer->GetRObject());
			CmdList.DrawIndirect(draw_visidSig.get(), sponza_trinf->Metadata->TrinfsCount, CompactedDrawArgs->GetRObject(), sizeof(DrawIndexArguments), CompactedDrawArgs->GetRObject(), 0);
		});
}

class FullScreenVS : public BuiltInShader
{
public:
	EXPORTED_BUILTIN_SHADER(FullScreenVS);
};

IMPLEMENT_BUILTIN_SHADER(FullScreenVS, "Debug.hlsl", "FullScreenVS", platform::Render::VertexShader);

BEGIN_SHADER_PARAMETER_STRUCT(DebugParameters)
SHADER_PARAMETER_TEXTURE(Texture2D, DepthTexture)
SHADER_PARAMETER(white::math::float4, InvDeviceZToWorldZTransform)
SHADER_PARAMETER(white::math::float2, NearFar)
SHADER_PARAMETER_SAMPLER(TextureSampleDesc, DepthSampler)
END_SHADER_PARAMETER_STRUCT()

class LinearDepthPS : public BuiltInShader
{
public:
	using Parameters = DebugParameters;
	EXPORTED_BUILTIN_SHADER(LinearDepthPS);
};

IMPLEMENT_BUILTIN_SHADER(LinearDepthPS, "Debug.hlsl", "LinearDepthPS", platform::Render::PixelShader);


void VisBufferTest::DrawDetph(render::CommandList& CmdList, render::Texture* screenTex, render::Texture* depthTex)
{
	auto PixelShader = render::GetBuiltInShaderMap()->GetShader<LinearDepthPS>();

	platform::Render::RenderPassInfo passInfo(screenTex, render::RenderTargetActions::Load_Store);
	CmdList.BeginRenderPass(passInfo, "DrawDetph");

	render::GraphicsPipelineStateInitializer GraphicsPSOInit{};
	render::PixelShaderUtils::InitFullscreenPipelineState(CmdList, PixelShader, GraphicsPSOInit);

	auto VertexShader = render::GetBuiltInShaderMap()->GetShader<FullScreenVS>();
	GraphicsPSOInit.ShaderPass.VertexShader = VertexShader.GetVertexShader();

	SetGraphicsPipelineState(CmdList, GraphicsPSOInit);

	DebugParameters Parameters;
	Parameters.DepthTexture = static_cast<Texture2D*>(depthTex);
	Parameters.DepthSampler.filtering = render::TexFilterOp::Min_Mag_Mip_Point;

	Parameters.InvDeviceZToWorldZTransform = WhiteEngine::CreateInvDeviceZToWorldZTransform(projMatrix);
	Parameters.NearFar.x = 1;
	Parameters.NearFar.y = 1 / (1000.0f - 1);

	SetShaderParameters(CmdList, PixelShader, PixelShader.GetPixelShader(), Parameters);

	PixelShaderUtils::DrawFullscreenTriangle(CmdList);
}