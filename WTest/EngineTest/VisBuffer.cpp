#include "VisBuffer.h"
#include "RenderInterface/BuiltinShader.h"
#include "Runtime/RenderCore/ShaderTextureTraits.hpp"
#include "Runtime/RenderCore/UnifiedBuffer.h"
#include "Runtime/RenderCore/Dispatch.h"

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

	class DrawArgumentDim : SHADER_PERMUTATION_SPARSE_INT("INDIRECT_DRAW_ARGUMENTS_STRUCT_NUM_ELEMENTS" , DrawIndexArgumentNumElements);

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

void VisBufferTest::RenderTrinf(CommandList& CmdList)
{
	auto& device = Context::Instance().GetDevice();

	if (sponza_trinf->State != Trinf::Resources::StreamingState::Resident)
		return;

	ViewArgs view;

	view.matrixs.mvp = wm::transpose(camera.GetViewMatrix() * projMatrix);

	auto ViewCB = CreateGraphicsBuffeImmediate(view, Buffer::Usage::SingleFrame);

	auto RWStructAccess = EAccessHint::EA_GPUReadWrite | EAccessHint::EA_GPUStructured  | EAccessHint::EA_GPUUnordered;

	auto FliteredIndexBuffer = shared_raw_robject(device.CreateBuffer(Buffer::Usage::SingleFrame,
		RWStructAccess | EAccessHint::EA_Raw,
		Trinf::Scene->Index.Allocator.GetMaxSize()* Trinf::Scene->Index.kPageSize, sizeof(uint32), nullptr));

	auto UncompactedDrawArgsSize = white::Align(sizeof(FilterTriangleCS::UncompactedDrawArguments) * sponza_trinf->Metadata->TrinfsCount,16);
	auto UncompactedDrawArgs = shared_raw_robject(device.CreateBuffer(Buffer::Usage::SingleFrame,
		RWStructAccess,
		UncompactedDrawArgsSize, sizeof(FilterTriangleCS::UncompactedDrawArguments), nullptr));
	auto UncompactedDrawArgsUAV = CmdList.CreateUnorderedAccessView(UncompactedDrawArgs.get());
	auto UncompactedDrawArgsSRV = CmdList.CreateShaderResourceView(UncompactedDrawArgs.get());

	MemsetResourceParams Params;
	Params.Count = UncompactedDrawArgsSize;
	Params.DstOffset = 0;
	Params.Value = 0;
	MemsetResource(CmdList, UncompactedDrawArgs, Params);

	FilterTriangleCS::Parameters Parameters;

	Parameters.View = ViewCB.Get();
	Parameters.IndexBuffer = CmdList.CreateShaderResourceView(Trinf::Scene->Index.DataBuffer.get()).get();
	Parameters.PositionBuffer = CmdList.CreateShaderResourceView(Trinf::Scene->Position.DataBuffer.get()).get();
	Parameters.FliteredIndexBuffer = CmdList.CreateUnorderedAccessView(FliteredIndexBuffer.get()).get();
	Parameters.UncompactedDrawArgs = UncompactedDrawArgsUAV.get();

	std::vector<FilterTriangleCS::FilterDispatchArgs> BatchTrinfArgs;
	BatchTrinfArgs.reserve(Caps.MaxDispatchThreadGroupsPerDimension.x);
	const int MaxDispatchCount = 2048;

	FilterTriangleCS::PermutationDomain PermutationVector;
	PermutationVector.Set<FilterTriangleCS::CullBackFaceDim >(true);
	PermutationVector.Set<FilterTriangleCS::CullFrustumFaceDim>(true);
	auto ComputeShader = GetBuiltInShaderMap()->GetShader<FilterTriangleCS>(PermutationVector);

	auto DipstachBatch = [&]()
		{
			int dispatchCount = static_cast<int>(BatchTrinfArgs.size());

			if (dispatchCount > 0)
			{
				auto ArgCB = CreateConstantBuffer(BatchTrinfArgs, Buffer::Usage::SingleFrame);

				Parameters.DispatchArgs = ArgCB.get();

				platform::ComputeShaderUtils::Dispatch(CmdList, ComputeShader, Parameters, white::math::int3(dispatchCount, 1, 1));

				BatchTrinfArgs.clear();
			}
		};

	auto AddBatch = [&](int drawId, const platform_ex::DSFileFormat::TriinfMetadata& trinf)
		{
			FilterTriangleCS::FilterDispatchArgs args;

			auto& cluster_compact = trinf.ClusterCompacts[0];
			auto& last_compact = trinf.ClusterCompacts[trinf.ClusterCount - 1];

			args.DrawId = drawId;
			args.IndexStart = cluster_compact.ClusterStart;
			args.IndexEnd = args.IndexStart + (trinf.ClusterCount - 1) * 64 + last_compact.TriangleCount * 3;

			args.VertexStart = 0;
			args.OutpuIndexOffset = args.IndexStart;

			for (uint32 clusterIndex = 0; clusterIndex < trinf.ClusterCount; ++clusterIndex)
			{
				args.ClusterId = clusterIndex;
				BatchTrinfArgs.push_back(args);

				if (BatchTrinfArgs.size() >= MaxDispatchCount)
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
	auto CompactedDrawArgs = shared_raw_robject(device.CreateBuffer(Buffer::Usage::SingleFrame,
		RWStructAccess | EAccessHint::EA_DrawIndirect | EAccessHint::EA_Raw,
		CompactedDrawArgsSize, sizeof(uint), nullptr));
	Params.Count = white::Align(sizeof(DrawIndexArguments),16);
	Params.DstOffset = 0;
	Params.Value = 0;
	MemsetResource(CmdList, CompactedDrawArgs, Params);

	auto CompactedDrawArgsUAV = CmdList.CreateUnorderedAccessView(CompactedDrawArgs.get());

	{
		BatchCompactionCS::Parameters compactionPars{};
		compactionPars.IndrectDrawArgsBuffer = CompactedDrawArgsUAV.get();
		compactionPars.MaxDraws = sponza_trinf->Metadata->TrinfsCount;
		compactionPars.UncompactedDrawArgs = UncompactedDrawArgsSRV.get();

		BatchCompactionCS::PermutationDomain compactPV;

		compactPV.Set<BatchCompactionCS::DrawArgumentDim >(BatchCompactionCS::DrawIndexArgumentNumElements);
		auto compactCS = GetBuiltInShaderMap()->GetShader<BatchCompactionCS>(compactPV);

		int dispatchCount = static_cast<int>((sponza_trinf->Metadata->TrinfsCount + 255) / 256);

		platform::ComputeShaderUtils::Dispatch(CmdList, compactCS, compactionPars, white::math::int3(dispatchCount, 1, 1));
	}

	auto& screen_frame = render::Context::Instance().GetScreenFrame();

	auto depth_tex = static_cast<render::Texture2D*>(screen_frame->Attached(render::FrameBuffer::DepthStencil));
	render::RenderPassInfo visPass(
		vis_buffer.get(), render::Clear_Store,
		depth_tex,render::DepthStencilTargetActions::ClearDepthStencil_StoreDepthStencil
	);

	CmdList.BeginRenderPass(visPass, "VisBuffer");

	auto VisTriVS = GetBuiltInShaderMap()->GetShader<VisTriangleVS>();
	VisTriangleVS::Parameters VSParas;
	VSParas.View = ViewCB.Get();

	auto VisTriPS = GetBuiltInShaderMap()->GetShader<VisTrianglePS>();

	// Setup pipelinestate
	render::GraphicsPipelineStateInitializer VisPso{};
	CmdList.FillRenderTargetsInfo(VisPso);

	VisPso.ShaderPass.VertexShader = VisTriVS.GetVertexShader();
	VisPso.ShaderPass.VertexDeclaration.push_back(
		CtorVertexElement(0, 0, Vertex::Usage::Position, 0, EF_ABGR32F, sizeof(wm::float3)));
	VisPso.ShaderPass.PixelShader = VisTriPS.GetPixelShader();

	SetGraphicsPipelineState(CmdList, VisPso);
	render::SetShaderParameters(CmdList, VisTriVS, VisTriVS.GetVertexShader(), VSParas);

	CmdList.SetVertexBuffer(0, Trinf::Scene->Position.DataBuffer.get());
	CmdList.SetIndexBuffer(FliteredIndexBuffer.get());

	CmdList.DrawIndirect(draw_visidSig.get(), sponza_trinf->Metadata->TrinfsCount, CompactedDrawArgs.get(), sizeof(DrawIndexArguments), CompactedDrawArgs.get(), 0);
}