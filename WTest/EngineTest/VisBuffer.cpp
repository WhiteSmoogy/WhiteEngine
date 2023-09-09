#include "VisBuffer.h"
#include "RenderInterface/BuiltinShader.h"
#include "Runtime/RenderCore/ShaderTextureTraits.hpp"
#include "Runtime/RenderCore/UnifiedBuffer.h"
#include "Runtime/RenderCore/Dispatch.h"

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

		uint ClusterId;

		uint Pad0;
		uint Pad1;
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
		SHADER_PARAMETER_CBUFFER(FilterDispatchArgs[4096], DispatchArgs)
		SHADER_PARAMETER_CBUFFER(ViewArgs, View)
		SHADER_PARAMETER_SRV(ByteAddressBuffer, IndexBuffer)
		SHADER_PARAMETER_SRV(ByteAddressBuffer, PositionBuffer)
		SHADER_PARAMETER_UAV(RWByteAddressBuffer, FliteredIndexBuffer)
		SHADER_PARAMETER_UAV(RWStructuredBuffer<UncompactedDrawArguments>, UncompactedDrawArgs)
		END_SHADER_PARAMETER_STRUCT()

};

IMPLEMENT_BUILTIN_SHADER(FilterTriangleCS, "FilterTriangle.wsl", "FilterTriangleCS", platform::Render::ComputeShader);

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

	FilterTriangleCS::ViewArgs view;

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
	Parameters.UncompactedDrawArgs = CmdList.CreateUnorderedAccessView(UncompactedDrawArgs.get()).get();

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
}