
#include "Asset/MeshX.h"
#include "Developer/Nanite/Cluster.h"
#include "Developer/Nanite/NaniteBuilder.h"

using namespace platform::Render;

void TestNanite()
{
	auto asset = platform::X::LoadMeshAsset("sponza_crytek/sponza_276.asset");

	std::vector<Nanite::FStaticMeshBuildVertex> Vertices;
	Vertices.resize(asset->GetVertexCount());

	white::byte empty_element[64] = {};
	auto get_element = [&](Vertex::Usage usage, uint8 index = 0)
		{
			auto& elements = asset->GetVertexElements();
			for (size_t i = 0; i != elements.size(); ++i)
			{
				if (elements[i].usage == usage && elements[i].usage_index == index)
				{
					return std::make_pair(asset->GetVertexStreams()[i].get(), elements[i].format);
				}
			}
			return std::make_pair(empty_element, EF_Unknown);
		};
	{
		auto pos_stream = get_element(Vertex::Usage::Position);
		wconstraint(IsFloatFormat(pos_stream.second));
		auto fill_pos = [pointer = pos_stream.first, format = pos_stream.second, stride = NumFormatBytes(pos_stream.second), &Vertices](Nanite::FStaticMeshBuildVertex& Vertex)
			{
				auto i = &Vertex - &Vertices[0];
				std::memcpy(&Vertex.Position, pointer + stride * i, sizeof(wm::float3));
			};
		std::for_each_n(Vertices.begin(), Vertices.size(), fill_pos);
	}
	{
		auto tangent_stream = get_element(Vertex::Usage::Tangent);
		wconstraint(tangent_stream.second == EF_ABGR8);
		auto fill_tangent = [pointer = (uint32*)tangent_stream.first, &Vertices](Nanite::FStaticMeshBuildVertex& Vertex)
			{
				auto i = &Vertex - &Vertices[0];

				auto w = (pointer[i] >> 24) / 255.f;
				auto z = ((pointer[i] >> 16) & 0XFF) / 255.f;
				auto y = ((pointer[i] >> 8) & 0XFF) / 255.f;
				auto x = (pointer[i] & 0XFF) / 255.f;
				auto tangent = wm::float4(x, y, z, w);

				tangent = tangent * 2 - wm::float4(1.f, 1.f, 1.f, 1.f);

				wm::quaternion Tangent_Quat{ tangent.x,tangent.y,tangent.z,tangent.w };

				Vertex.TangentX = wm::transformquat(wm::float3(1, 0, 0), Tangent_Quat);
				Vertex.TangentY = wm::transformquat(wm::float3(0, 1, 0), Tangent_Quat);
				Vertex.TangentZ = wm::transformquat(wm::float3(0, 0, 1), Tangent_Quat);
			};
		std::for_each_n(Vertices.begin(), Vertices.size(), fill_tangent);
	}
	uint32 NumTexCoords = 0;
	{
		for (uint8 i = 0; i != 8; ++i)
		{
			auto uv_stream = get_element(Vertex::Usage::TextureCoord, i);
			if (uv_stream.second == EF_Unknown)
				break;
			wconstraint(uv_stream.second == EF_GR32F);
			auto fill_uv = [pointer = (wm::float2*)uv_stream.first, &Vertices, &i](Nanite::FStaticMeshBuildVertex& Vertex)
				{
					auto index = &Vertex - &Vertices[0];
					Vertex.UVs[i] = pointer[index];
				};
			std::for_each_n(Vertices.begin(), Vertices.size(), fill_uv);
			++NumTexCoords;
		}
	}

	std::vector<uint32> Indexs;
	{
		Indexs.resize(asset->GetIndexCount());
		auto index_stream = asset->GetIndexStreams().get();
		auto fill_index = [&](size_t i)
			{
				if (asset->GetIndexFormat() == EF_R16UI)
				{
					Indexs[i] = ((uint16*)index_stream)[i];
				}
				else
					Indexs[i] = ((uint32*)index_stream)[i];
			};
		for (size_t i = 0; i != Indexs.size(); ++i)
			fill_index(i);
	}

	std::vector<int32> MaterialIndices;
	std::vector<uint32> MeshTriangleCounts;

	MaterialIndices.reserve(Indexs.size() / 3);
	for (auto& desc : asset->GetSubMeshDesces())
	{
		MaterialIndices.insert(MaterialIndices.end(), desc.LodsDescription[0].IndexNum / 3, desc.MaterialIndex);
		MeshTriangleCounts.emplace_back(desc.LodsDescription[0].VertexNum / 3);
		wconstraint(desc.LodsDescription.size() == 1);
	}

	Nanite::Resources Res;
	Nanite::Build(Res, Vertices, Indexs, MaterialIndices, MeshTriangleCounts, NumTexCoords, {});
}
