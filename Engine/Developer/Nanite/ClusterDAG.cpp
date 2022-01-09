// Copyright Epic Games, Inc. All Rights Reserved.
#if defined(_MSC_VER) && !defined(__clang__)
#pragma warning(disable:4267)
#endif

#include "ClusterDAG.h"
#include "Runtime/Core/ParallelFor.h"
#include "GraphPartitioner.h"
#include "Developer/MeshSimplifier/MeshSimplify.h"
#include "Runtime/Core/Container/BinaryHeap.h"
#include <WBase/span.hpp>
#include <atomic>

#define LOG_CRC(x) (void)x

namespace Nanite
{

static const uint32 MinGroupSize = 8;
static const uint32 MaxGroupSize = 32;

static void DAGReduce( std::vector< FClusterGroup >& Groups, std::vector< FCluster >& Clusters, std::atomic< uint32 >& NumClusters, white::span< uint32 > Children, int32 GroupIndex, uint32 MeshIndex );

void BuildDAG(std::vector< FClusterGroup >& Groups, std::vector< FCluster >& Clusters, uint32 ClusterRangeStart, uint32 ClusterRangeNum, uint32 MeshIndex, FBounds& MeshBounds )
{
	uint32 LevelOffset	= ClusterRangeStart;
	
	std::atomic< uint32 >	NumClusters( Clusters.size() );
	uint32				NumExternalEdges = 0;

	bool bFirstLevel = true;

	while( true )
	{
		white::span< FCluster > LevelClusters( &Clusters[LevelOffset], bFirstLevel ? ClusterRangeNum : (Clusters.size() - LevelOffset) );
		bFirstLevel = false;
		
		for( FCluster& Cluster : LevelClusters )
		{
			NumExternalEdges	+= Cluster.NumExternalEdges;
			MeshBounds			+= Cluster.Bounds;
		}

		if( LevelClusters.size() < 2 )
			break;

		if( LevelClusters.size() <= MaxGroupSize )
		{
			std::vector<uint32> Children;

			uint32 MaxParents = 0;
			for( FCluster& Cluster : LevelClusters )
			{
				MaxParents += wm::DivideAndRoundUp< uint32 >( Cluster.Indexes.size(), FCluster::ClusterSize * 6 );
				Children.emplace_back( LevelOffset++ );
			}

			LevelOffset = Clusters.size();
			Clusters.resize(Clusters.size() + MaxParents,{});
			Groups.emplace_back();

			DAGReduce( Groups, Clusters, NumClusters,white::make_span(Children), Groups.size() - 1, MeshIndex );

			// Correct num to atomic count
			Clusters.resize( NumClusters);

			continue;
		}
		
		struct FExternalEdge
		{
			uint32	ClusterIndex;
			uint32	EdgeIndex;
		};
		std::vector< FExternalEdge >	ExternalEdges;
		white::FHashTable				ExternalEdgeHash;
		std::atomic< uint32 >		ExternalEdgeOffset(0);

		// We have a total count of NumExternalEdges so we can allocate a hash table without growing.
		ExternalEdges.resize(ExternalEdges.size()+NumExternalEdges );
		ExternalEdgeHash.Clear( 1 << wm::FloorLog2( NumExternalEdges ), NumExternalEdges );
		NumExternalEdges = 0;

		// Add edges to hash table
		ParallelFor( LevelClusters.size(),
			[&]( uint32 ClusterIndex )
			{
				FCluster& Cluster = LevelClusters[ ClusterIndex ];

				for( white::TConstSetBitIterator<> SetBit( Cluster.ExternalEdges ); SetBit; ++SetBit )
				{
					uint32 EdgeIndex = SetBit.GetIndex();

					uint32 VertIndex0 = Cluster.Indexes[ EdgeIndex ];
					uint32 VertIndex1 = Cluster.Indexes[ Cycle3( EdgeIndex ) ];
	
					const auto& Position0 = Cluster.GetPosition( VertIndex0 );
					const auto& Position1 = Cluster.GetPosition( VertIndex1 );

					uint32 Hash0 = HashPosition( Position0 );
					uint32 Hash1 = HashPosition( Position1 );
					uint32 Hash = Murmur32( { Hash0, Hash1 } );

					uint32 ExternalEdgeIndex = ExternalEdgeOffset++;
					ExternalEdges[ ExternalEdgeIndex ] = { ClusterIndex, EdgeIndex };
					ExternalEdgeHash.Add_Concurrent( Hash, ExternalEdgeIndex );
				}
			});

		wconstraint( ExternalEdgeOffset == ExternalEdges.size() );

		std::atomic< uint32 > NumAdjacency(0);

		// Find matching edge in other clusters
		ParallelFor( LevelClusters.size(),
			[&]( uint32 ClusterIndex )
			{
				FCluster& Cluster = LevelClusters[ ClusterIndex ];

				for(white::TConstSetBitIterator<> SetBit( Cluster.ExternalEdges ); SetBit; ++SetBit )
				{
					uint32 EdgeIndex = SetBit.GetIndex();

					uint32 VertIndex0 = Cluster.Indexes[ EdgeIndex ];
					uint32 VertIndex1 = Cluster.Indexes[ Cycle3( EdgeIndex ) ];
	
					const auto& Position0 = Cluster.GetPosition( VertIndex0 );
					const auto& Position1 = Cluster.GetPosition( VertIndex1 );

					uint32 Hash0 = HashPosition( Position0 );
					uint32 Hash1 = HashPosition( Position1 );
					uint32 Hash = Murmur32( { Hash1, Hash0 } );

					for( uint32 ExternalEdgeIndex = ExternalEdgeHash.First( Hash ); ExternalEdgeHash.IsValid( ExternalEdgeIndex ); ExternalEdgeIndex = ExternalEdgeHash.Next( ExternalEdgeIndex ) )
					{
						FExternalEdge ExternalEdge = ExternalEdges[ ExternalEdgeIndex ];

						FCluster& OtherCluster = LevelClusters[ ExternalEdge.ClusterIndex ];

						if( OtherCluster.ExternalEdges[ ExternalEdge.EdgeIndex ] )
						{
							uint32 OtherVertIndex0 = OtherCluster.Indexes[ ExternalEdge.EdgeIndex ];
							uint32 OtherVertIndex1 = OtherCluster.Indexes[ Cycle3( ExternalEdge.EdgeIndex ) ];
			
							if( Position0 == OtherCluster.GetPosition( OtherVertIndex1 ) &&
								Position1 == OtherCluster.GetPosition( OtherVertIndex0 ) )
							{
								// Found matching edge. Increase it's count.
								Cluster.AdjacentClusters.try_emplace( ExternalEdge.ClusterIndex, 0 ).first->second++;

								// Can't break or a triple edge might be non-deterministically connected.
								// Need to find all matching, not just first.
							}
						}
					}
				}
				NumAdjacency += Cluster.AdjacentClusters.size();

				// Force deterministic order of adjacency.
				{
					//Collect Keys
					std::vector<uint32> Keys;
					for (auto& pair : Cluster.AdjacentClusters)
					{
						Keys.emplace_back(pair.first);
					}
					std::sort(Keys.begin(), Keys.end(), [&LevelClusters](uint32 A, uint32 B)
						{
							return LevelClusters[A].GUID < LevelClusters[B].GUID;
						});
					std::unordered_map<uint32, uint32> KeySort;
					for (auto& key : Keys)
					{
						KeySort.emplace(key, Cluster.AdjacentClusters[key]);
					}
					std::swap(Cluster.AdjacentClusters, KeySort);
				}
			});

		white::FDisjointSet DisjointSet( LevelClusters.size() );

		for( uint32 ClusterIndex = 0; ClusterIndex < (uint32)LevelClusters.size(); ClusterIndex++ )
		{
			for( auto& Pair : LevelClusters[ ClusterIndex ].AdjacentClusters )
			{
				uint32 OtherClusterIndex = Pair.first;

				uint32 Count = LevelClusters[ OtherClusterIndex ].AdjacentClusters.at( ClusterIndex );
				wconstraint( Count == Pair.second );

				if( ClusterIndex > OtherClusterIndex )
				{
					DisjointSet.UnionSequential( ClusterIndex, OtherClusterIndex );
				}
			}
		}

		FGraphPartitioner Partitioner( LevelClusters.size() );

		// Sort to force deterministic order
		{
			std::vector< uint32 > SortedIndexes;
			SortedIndexes.resize( Partitioner.Indexes.size() );
			RadixSort32( SortedIndexes.data(), Partitioner.Indexes.data(), Partitioner.Indexes.size(),
				[&]( uint32 Index )
				{
					return LevelClusters[ Index ].GUID;
				} );
			std::swap( Partitioner.Indexes, SortedIndexes );
		}

		auto GetCenter = [&]( uint32 Index )
		{
			FBounds& Bounds = LevelClusters[ Index ].Bounds;
			return 0.5f * ( Bounds.Min + Bounds.Max );
		};
		Partitioner.BuildLocalityLinks( DisjointSet, MeshBounds, GetCenter );

		auto* wrestrict Graph = Partitioner.NewGraph( NumAdjacency );

		for( int32 i = 0; i < LevelClusters.size(); i++ )
		{
			Graph->AdjacencyOffset[i] = Graph->Adjacency.size();

			uint32 ClusterIndex = Partitioner.Indexes[i];

			for( auto& Pair : LevelClusters[ ClusterIndex ].AdjacentClusters )
			{
				uint32 OtherClusterIndex = Pair.first;
				uint32 NumSharedEdges = Pair.second;

				const auto& Cluster0 = Clusters[ LevelOffset + ClusterIndex ];
				const auto& Cluster1 = Clusters[ LevelOffset + OtherClusterIndex ];

				bool bSiblings = Cluster0.GroupIndex != wm::MAX_uint32 && Cluster0.GroupIndex == Cluster1.GroupIndex;

				Partitioner.AddAdjacency( Graph, OtherClusterIndex, NumSharedEdges * ( bSiblings ? 1 : 16 ) + 4 );
			}

			Partitioner.AddLocalityLinks( Graph, ClusterIndex, 1 );
		}
		Graph->AdjacencyOffset[ Graph->Num ] = Graph->Adjacency.size();

		LOG_CRC( Graph->Adjacency );
		LOG_CRC( Graph->AdjacencyCost );
		LOG_CRC( Graph->AdjacencyOffset );

		Partitioner.PartitionStrict( Graph, MinGroupSize, MaxGroupSize, true );

		LOG_CRC( Partitioner.Ranges );

		uint32 MaxParents = 0;
		for( auto& Range : Partitioner.Ranges )
		{
			uint32 NumParentIndexes = 0;
			for( uint32 i = Range.Begin; i < Range.End; i++ )
			{
				// Global indexing is needed in Reduce()
				Partitioner.Indexes[i] += LevelOffset;
				NumParentIndexes += Clusters[ Partitioner.Indexes[i] ].Indexes.size();
			}
			MaxParents += wm::DivideAndRoundUp( NumParentIndexes, FCluster::ClusterSize * 6 );
		}

		LevelOffset = Clusters.size();

		Clusters.resize(Clusters.size() + MaxParents, {});
		Groups.resize(Groups.size() + Partitioner.Ranges.size(), {});

		ParallelFor( Partitioner.Ranges.size(),
			[&]( int32 PartitionIndex )
			{
				auto& Range = Partitioner.Ranges[ PartitionIndex ];

				white::span< uint32 > Children( &Partitioner.Indexes[ Range.Begin ], Range.End - Range.Begin );
				uint32 ClusterGroupIndex = PartitionIndex + Groups.size() - Partitioner.Ranges.size();

				DAGReduce( Groups, Clusters, NumClusters, Children, ClusterGroupIndex, MeshIndex );
			});

		// Correct num to atomic count
		Clusters.resize( NumClusters);
	}
	
	// Max out root node
	uint32 RootIndex = LevelOffset;
	FClusterGroup RootClusterGroup;
	RootClusterGroup.Children.emplace_back( RootIndex );
	RootClusterGroup.Bounds = Clusters[ RootIndex ].SphereBounds;
	RootClusterGroup.LODBounds = Sphere{};
	RootClusterGroup.MaxParentLODError = 1e10f;
	RootClusterGroup.MinLODError = -1.0f;
	RootClusterGroup.MipLevel = Clusters[RootIndex].MipLevel + 1;
	RootClusterGroup.MeshIndex = MeshIndex;
	Clusters[ RootIndex ].GroupIndex = Groups.size();
	Groups.emplace_back( RootClusterGroup );
}

static void DAGReduce(std::vector< FClusterGroup >& Groups,std::vector< FCluster >& Clusters, std::atomic< uint32 >& NumClusters, white::span< uint32 > Children, int32 GroupIndex, uint32 MeshIndex )
{
	wconstraint( GroupIndex >= 0 );

	// Merge
	std::vector< const FCluster*> MergeList;
	for( int32 Child : Children )
	{
		MergeList.emplace_back( &Clusters[ Child ] );
	}
	
	// Force a deterministic order
	std::sort(MergeList.begin(), MergeList.end(), [](const FCluster* A, const FCluster* B)
		{
			return A->GUID < B->GUID;
		});

	FCluster Merged( MergeList );

	int32 NumParents = wm::DivideAndRoundUp< int32 >( Merged.Indexes.size(), FCluster::ClusterSize * 6 );
	int32 ParentStart = 0;
	int32 ParentEnd = 0;

	float ParentMaxLODError = 0.0f;

	for( int32 TargetClusterSize = FCluster::ClusterSize - 2; TargetClusterSize > FCluster::ClusterSize / 2; TargetClusterSize -= 2 )
	{
		int32 TargetNumTris = NumParents * TargetClusterSize;

		// Simplify
		ParentMaxLODError = Merged.Simplify( TargetNumTris );

		// Split
		if( NumParents == 1 )
		{
			ParentEnd = ( NumClusters += NumParents );
			ParentStart = ParentEnd - NumParents;

			Clusters[ ParentStart ] = Merged;
			Clusters[ ParentStart ].Bound();
			break;
		}
		else
		{
			FGraphPartitioner Partitioner( Merged.Indexes.size() / 3 );
			Merged.Split( Partitioner );

			if( Partitioner.Ranges.size() <= NumParents )
			{
				NumParents = Partitioner.Ranges.size();
				ParentEnd = ( NumClusters += NumParents );
				ParentStart = ParentEnd - NumParents;

				int32 Parent = ParentStart;
				for( auto& Range : Partitioner.Ranges )
				{
					Clusters[ Parent ] = FCluster( Merged, Range.Begin, Range.End, Partitioner.Indexes );
					Parent++;
				}

				break;
			}
		}
	}

	std::vector<Sphere> Children_SphereBounds;
	std::vector<Sphere> Children_LODBounds;
					
	// Force monotonic nesting.
	float ChildMinLODError = wm::MAX_flt;
	for( int32 Child : Children )
	{
		bool bLeaf = Clusters[ Child ].EdgeLength < 0.0f;
		float LODError = Clusters[ Child ].LODError;

		Children_LODBounds.emplace_back( Clusters[ Child ].LODBounds );
		Children_SphereBounds.emplace_back( Clusters[ Child ].SphereBounds );
		ChildMinLODError = std::min( ChildMinLODError, bLeaf ? -1.0f : LODError );
		ParentMaxLODError = std::max( ParentMaxLODError, LODError );

		Clusters[ Child ].GroupIndex = GroupIndex;
		Groups[ GroupIndex ].Children.emplace_back( Child );
		wconstraint( Groups[ GroupIndex ].Children.size() <= MAX_CLUSTERS_PER_GROUP_TARGET );
	}
	
	Sphere	ParentLODBounds( Children_LODBounds.data(), Children_LODBounds.size() );
	Sphere	ParentBounds( Children_SphereBounds.data(), Children_SphereBounds.size() );

	// Force parents to have same LOD data. They are all dependent.
	for( int32 Parent = ParentStart; Parent < ParentEnd; Parent++ )
	{
		Clusters[ Parent ].LODBounds			= ParentLODBounds;
		Clusters[ Parent ].LODError				= ParentMaxLODError;
		Clusters[ Parent ].GeneratingGroupIndex = GroupIndex;
	}

	Groups[ GroupIndex ].Bounds				= ParentBounds;
	Groups[ GroupIndex ].LODBounds			= ParentLODBounds;
	Groups[ GroupIndex ].MinLODError		= ChildMinLODError;
	Groups[ GroupIndex ].MaxParentLODError	= ParentMaxLODError;
	Groups[ GroupIndex ].MipLevel			= Merged.MipLevel - 1;
	Groups[ GroupIndex ].MeshIndex			= MeshIndex;
}

FCluster FindDAGCut(
	const std::vector< FClusterGroup >& Groups,
	const std::vector< FCluster >& Clusters,
	uint32 TargetNumTris )
{
	const FClusterGroup&	RootGroup = Groups.back();
	const FCluster&			RootCluster = Clusters[ RootGroup.Children[0] ];

	float MinError = RootCluster.LODError;

	white::FBinaryHeap< float > Heap;
	Heap.Add( -RootCluster.LODError, RootGroup.Children[0] );

	while( true )
	{
		// Grab highest error cluster to replace to reduce cut error
		const FCluster& Cluster = Clusters[ Heap.Top() ];

		if( Cluster.MipLevel == 0 )
			break;

		// Could have other targets besides NumTris in the future.
		bool bHitTarget = Heap.Num() * FCluster::ClusterSize > TargetNumTris;
		if( bHitTarget && Cluster.LODError < MinError )
			break;
		
		Heap.Pop();

		wconstraint( Cluster.LODError <= MinError );
		MinError = Cluster.LODError;

		for( uint32 Child : Groups[ Cluster.GeneratingGroupIndex ].Children )
		{
			if( !Heap.IsPresent( Child ) )
			{
				const FCluster& ChildCluster = Clusters[ Child ];

				wconstraint( ChildCluster.MipLevel + 1 == Cluster.MipLevel );
				wconstraint( ChildCluster.LODError <= MinError );
				Heap.Add( -ChildCluster.LODError, Child );
			}
		}
	}

	// Merge
	std::vector< const FCluster*> MergeList;
	MergeList.resize( Heap.Num() );
	for( uint32 i = 0; i < Heap.Num(); i++ )
	{
		MergeList[i] = &Clusters[ Heap.Peek(i) ];
	}

	// Force a deterministic order
	std::sort(MergeList.begin(), MergeList.end(), [](const FCluster* A, const FCluster* B)
		{
			return A->GUID < B->GUID;
		});

	return FCluster( MergeList );
}


template<typename FArchive>
FArchive& operator<<(FArchive& Ar, FClusterGroup& Group)
{
	Ar << Group.Bounds;
	Ar << Group.LODBounds;
	Ar << Group.MinLODError;
	Ar << Group.MaxParentLODError;
	Ar << Group.MipLevel;
	Ar << Group.MeshIndex;

	Ar << Group.PageIndexStart;
	Ar << Group.PageIndexNum;
	Ar << Group.Children;
	return Ar;
}

} // namespace Nanite