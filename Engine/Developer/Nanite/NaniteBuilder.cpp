#include "NaniteBuilder.h"
#include "Bounds.h"
#include "Cluster.h"
#include "NaniteEncode.h"
#include "GraphPartitioner.h"
#include "Core/Container/BitArray.h"
#include "Core/Container/HashTable.h"
#include "Core/Container/DisjointSet.h"
#include "Core/ParallelFor.h"
#include "Developer/MeshSimplifier/MeshSimplify.h"

#define NumBitsPerDWORD ((int32)32)
#define NumBitsPerDWORDLogTwo ((int32)5)
#define LOG_CRC(x) (void)x

#pragma warning(disable:4267)

namespace Nanite
{
	using  white::TBitArray;
	using  white::FHashTable;
	using  white::FDisjointSet;
	struct MeshDescrption
	{
		white::uint8 MaterialIndex;
		white::uint32 VertexNum;
		white::uint32 VertexBase;
		white::uint32 IndexNum;
		white::uint32 IndexBase;
	};

	static void ClusterTriangles(
		const std::vector< FStaticMeshBuildVertex >& Verts,
		const white::span< const uint32 >& Indexes,
		const white::span< const int32 >& MaterialIndexes,
		std::vector< FCluster >& Clusters,	// Append
		const FBounds& MeshBounds,
		uint32 NumTexCoords,
		bool bHasColors)
	{
		spdlog::stopwatch sw;

		LOG_CRC(Verts);
		LOG_CRC(Indexes);

		uint32 NumTriangles = Indexes.size() / 3;

		std::vector< uint32 > SharedEdges;
		SharedEdges.resize(Indexes.size());

		TBitArray<> BoundaryEdges;
		BoundaryEdges.resize(Indexes.size(),false);

		FHashTable EdgeHash(1 << wm::FloorLog2(Indexes.size()), Indexes.size());

		ParallelFor(Indexes.size(),
			[&](int32 EdgeIndex)
			{

				uint32 VertIndex0 = Indexes[EdgeIndex];
				uint32 VertIndex1 = Indexes[Cycle3(EdgeIndex)];

				const auto& Position0 = Verts[VertIndex0].Position;
				const auto& Position1 = Verts[VertIndex1].Position;

				uint32 Hash0 = HashPosition(Position0);
				uint32 Hash1 = HashPosition(Position1);
				uint32 Hash = Murmur32({ Hash0, Hash1 });

				EdgeHash.Add_Concurrent(Hash, EdgeIndex);
			});

		const int32 NumDwords = wm::DivideAndRoundUp<int32>(BoundaryEdges.size(), NumBitsPerDWORD);

		ParallelFor(NumDwords,
			[&](int32 DwordIndex)
			{
				const int32 NumIndexes = Indexes.size();
				const int32 NumBits = std::min(NumBitsPerDWORD, NumIndexes - DwordIndex * NumBitsPerDWORD);

				uint32 Mask = 1;
				uint32 Dword = 0;
				for (int32 BitIndex = 0; BitIndex < NumBits; BitIndex++, Mask <<= 1)
				{
					int32 EdgeIndex = DwordIndex * NumBitsPerDWORD + BitIndex;

					uint32 VertIndex0 = Indexes[EdgeIndex];
					uint32 VertIndex1 = Indexes[Cycle3(EdgeIndex)];

					const auto& Position0 = Verts[VertIndex0].Position;
					const auto& Position1 = Verts[VertIndex1].Position;

					uint32 Hash0 = HashPosition(Position0);
					uint32 Hash1 = HashPosition(Position1);
					uint32 Hash = Murmur32({ Hash1, Hash0 });

					// Find edge with opposite direction that shares these 2 verts.
					/*
						  /\
						 /  \
						o-<<-o
						o->>-o
						 \  /
						  \/
					*/
					uint32 FoundEdge = ~0u;
					for (uint32 OtherEdgeIndex = EdgeHash.First(Hash); EdgeHash.IsValid(OtherEdgeIndex); OtherEdgeIndex = EdgeHash.Next(OtherEdgeIndex))
					{
						uint32 OtherVertIndex0 = Indexes[OtherEdgeIndex];
						uint32 OtherVertIndex1 = Indexes[Cycle3(OtherEdgeIndex)];

						if (Position0 == Verts[OtherVertIndex1].Position &&
							Position1 == Verts[OtherVertIndex0].Position)
						{
							// Found matching edge.
							// Hash table is not in deterministic order. Find stable match not just first.
							FoundEdge = std::min(FoundEdge, OtherEdgeIndex);
						}
					}
					SharedEdges[EdgeIndex] = FoundEdge;

					if (FoundEdge == ~0u)
					{
						Dword |= Mask;
					}
				}

				if (Dword)
				{
					BoundaryEdges.data()[DwordIndex] = Dword;
				}
			});

		FDisjointSet DisjointSet(NumTriangles);

		for (uint32 EdgeIndex = 0, Num = SharedEdges.size(); EdgeIndex < Num; EdgeIndex++)
		{
			uint32 OtherEdgeIndex = SharedEdges[EdgeIndex];
			if (OtherEdgeIndex != ~0u)
			{
				// OtherEdgeIndex is smallest that matches EdgeIndex
				// ThisEdgeIndex is smallest that matches OtherEdgeIndex

				uint32 ThisEdgeIndex = SharedEdges[OtherEdgeIndex];
				wconstraint(ThisEdgeIndex != ~0u);
				wconstraint(ThisEdgeIndex <= EdgeIndex);

				if (EdgeIndex > ThisEdgeIndex)
				{
					// Previous element points to OtherEdgeIndex
					SharedEdges[EdgeIndex] = ~0u;
				}
				else if (EdgeIndex > OtherEdgeIndex)
				{
					// Second time seeing this
					DisjointSet.UnionSequential(EdgeIndex / 3, OtherEdgeIndex / 3);
				}
			}
		}

		spdlog::info("Boundary [{}s], tris: {}, UVs {}{}", sw, Indexes.size() / 3, NumTexCoords, bHasColors ?", Color" : "");
		sw.reset();

		LOG_CRC(SharedEdges);

		FGraphPartitioner Partitioner(NumTriangles);

		{
			NANITE_TRACE(Build::PartitionGraph);

			auto GetCenter = [&Verts, &Indexes](uint32 TriIndex)
			{
				wm::float3 Center;
				Center = Verts[Indexes[TriIndex * 3 + 0]].Position;
				Center += Verts[Indexes[TriIndex * 3 + 1]].Position;
				Center += Verts[Indexes[TriIndex * 3 + 2]].Position;
				return Center * (1.0f / 3.0f);
			};
			Partitioner.BuildLocalityLinks(DisjointSet, MeshBounds, GetCenter);

			auto* wrestrict Graph = Partitioner.NewGraph(NumTriangles * 3);

			for (uint32 i = 0; i < NumTriangles; i++)
			{
				Graph->AdjacencyOffset[i] = Graph->Adjacency.size();

				uint32 TriIndex = Partitioner.Indexes[i];

				for (int k = 0; k < 3; k++)
				{
					uint32 EdgeIndex = SharedEdges[3 * TriIndex + k];
					if (EdgeIndex != ~0u)
					{
						Partitioner.AddAdjacency(Graph, EdgeIndex / 3, 4 * 65);
					}
				}

				Partitioner.AddLocalityLinks(Graph, TriIndex, 1);
			}
			Graph->AdjacencyOffset[NumTriangles] = Graph->Adjacency.size();

			Partitioner.PartitionStrict(Graph, FCluster::ClusterSize - 4, FCluster::ClusterSize, true);
			wconstraint(Partitioner.Ranges.size());

			LOG_CRC(Partitioner.Ranges);
		}

		const uint32 OptimalNumClusters = wm::DivideAndRoundUp< int32 >(Indexes.size(), FCluster::ClusterSize * 3);

		spdlog::info("Clustering [{}s]. Ratio: {}", sw, (float)Partitioner.Ranges.size() / OptimalNumClusters);
		sw.reset();

		const uint32 BaseCluster = Clusters.size();
		Clusters.resize(Clusters.size()+Partitioner.Ranges.size());

		const auto Flags = Partitioner.Ranges.size() > 32 ? ParallelForFlags::SingleThread: ParallelForFlags::None;
		{
			NANITE_TRACE(Build::BuildClusters);
			ParallelFor(Partitioner.Ranges.size(),
				[&](int32 Index)
				{
					auto& Range = Partitioner.Ranges[Index];


					Clusters[BaseCluster + Index] = FCluster(Verts,
						Indexes,
						MaterialIndexes,
						BoundaryEdges, Range.Begin, Range.End, Partitioner.Indexes, NumTexCoords, bHasColors);

					// Negative notes it's a leaf
					Clusters[BaseCluster + Index].EdgeLength *= -1.0f;
				}, Flags);
		}

		spdlog::info("Leaves [{}s]", sw);
	}

	static bool BuildNaniteData(
		Resources& Resources,
		std::vector< FStaticMeshBuildVertex >& Verts, // TODO: Do not require this vertex type for all users of Nanite
		std::vector< uint32 >& Indexes,
		std::vector< int32 >& MaterialIndexes,
		std::vector<uint32>& MeshTriangleCounts,
		std::vector< MeshDescrption>& Sections,
		uint32 NumTexCoords,
		const Settings& Settings
	)
	{
		NANITE_TRACE(BuildData);

		if (NumTexCoords > MAX_NANITE_UVS) NumTexCoords = MAX_NANITE_UVS;

		FBounds	VertexBounds;
		uint32 Channel = 255;
		for (auto& Vert : Verts)
		{
			VertexBounds += Vert.Position;

			Channel &= Vert.Color.R;
			Channel &= Vert.Color.G;
			Channel &= Vert.Color.B;
			Channel &= Vert.Color.A;
		}

		const uint32 NumMeshes = MeshTriangleCounts.size();

		// Don't trust any input. We only have color if it isn't all white.
		bool bHasColors = Channel != 255;

		std::vector< uint32 > ClusterCountPerMesh;
		std::vector< FCluster > Clusters;
		{
			uint32 BaseTriangle = 0;
			for (uint32 NumTriangles : MeshTriangleCounts)
			{
				uint32 NumClustersBefore = Clusters.size();
				if (NumTriangles)
				{
					ClusterTriangles(Verts, white::make_const_span(&Indexes[BaseTriangle * 3], NumTriangles * 3),
						white::make_const_span(&MaterialIndexes[BaseTriangle], NumTriangles),
						Clusters, VertexBounds, NumTexCoords, bHasColors);
				}
				ClusterCountPerMesh.emplace_back<uint32>(Clusters.size() - NumClustersBefore);
				BaseTriangle += NumTriangles;
			}
		}

		const int32 OldTriangleCount = Indexes.size() / 3;
		const int32 MinTriCount = 2000;
		

		spdlog::stopwatch build_sw;

		FBounds MeshBounds;
		std::vector<FClusterGroup> Groups;
		{
			NANITE_TRACE(Build::DAG.Reduce);

			uint32 ClusterStart = 0;
			for (uint32 MeshIndex = 0; MeshIndex < NumMeshes; MeshIndex++)
			{
				uint32 NumClusters = ClusterCountPerMesh[MeshIndex];
				BuildDAG(Groups, Clusters, ClusterStart, NumClusters, MeshIndex, MeshBounds);
				ClusterStart += NumClusters;
			}
		}

		spdlog::stopwatch encode_sw;

		Encode(Resources, Settings, Clusters, Groups, MeshBounds, NumMeshes, NumTexCoords, bHasColors);

		spdlog::info("Encode [{}s]", encode_sw);

		

		spdlog::info("Nanite build [{}s]", build_sw);

		return true;
	}

	void Build(
		Resources& Resources,
		std::vector<FStaticMeshBuildVertex>& Vertices, // TODO: Do not require this vertex type for all users of Nanite
		std::vector<uint32>& TriangleIndices,
		std::vector<int32>& MaterialIndices,
		std::vector<uint32>& MeshTriangleCounts,
		uint32 NumTexCoords,
		const Settings& Settings)
	{
		std::vector<MeshDescrption> sections;

		BuildNaniteData(
			Resources,
			Vertices,
			TriangleIndices,
			MaterialIndices,
			MeshTriangleCounts,
			sections,
			NumTexCoords,
			Settings
		);
	}
}