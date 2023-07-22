// Copyright Epic Games, Inc. All Rights Reserved.
#if defined(_MSC_VER) && !defined(__clang__)
#pragma warning(disable:4267)
#endif

#include "Cluster.h"
#include "Bounds.h"
#include "Developer/MeshSimplifier/MeshSimplify.h"
#include "Core/Container/HashTable.h"
#include "Core/PlatformMemory.h"
#include "Developer/MeshBuild.h"
#include "Core/Container/DisjointSet.h"
#include "Core/Math/Float32.h"
#include "GraphPartitioner.h"

#include "Core/Container/map.hpp"

template< typename T > inline uint32 Min3Index( const T A, const T B, const T C ) { return ( A < B ) ? ( ( A < C ) ? 0 : 2 ) : ( ( B < C ) ? 1 : 2 ); }
template< typename T > inline uint32 Max3Index( const T A, const T B, const T C ) { return ( A > B ) ? ( ( A > C ) ? 0 : 2 ) : ( ( B > C ) ? 1 : 2 ); }

namespace Nanite
{

void CorrectAttributes( float* Attributes )
{
	wm::float3& Normal = *reinterpret_cast< wm::float3* >( Attributes );
	Normal = wm::normalize(Normal);
}

void CorrectAttributesColor( float* Attributes )
{
	CorrectAttributes( Attributes );
	
	LinearColor& Color = *reinterpret_cast< LinearColor* >( Attributes + 3 );
	Color = clamp(Color);
}


FCluster::FCluster(
	const std::vector< FStaticMeshBuildVertex >& InVerts,
	const white::span< const uint32 >& InIndexes,
	const white::span< const int32 >& InMaterialIndexes,
	const white::TBitArray<>& InBoundaryEdges,
	uint32 TriBegin, uint32 TriEnd, const std::vector< uint32 >& TriIndexes, uint32 InNumTexCoords, bool bInHasColors )
{
	GUID = Murmur32( { TriBegin, TriEnd } );
	
	const uint32 NumTriangles = TriEnd - TriBegin;
	//ensure(NumTriangles <= FCluster::ClusterSize);
	
	bHasColors = bInHasColors;
	NumTexCoords = InNumTexCoords;

	Verts.reserve( NumTriangles * GetVertSize() );
	Indexes.reserve( 3 * NumTriangles );
	BoundaryEdges.reserve( 3 * NumTriangles );
	MaterialIndexes.reserve( NumTriangles );

	wconstraint(InMaterialIndexes.size() * 3 == InIndexes.size());

	std::unordered_map< uint32, uint32 > OldToNewIndex;
	OldToNewIndex.reserve( NumTriangles );

	for( uint32 i = TriBegin; i < TriEnd; i++ )
	{
		uint32 TriIndex = TriIndexes[i];

		for( uint32 k = 0; k < 3; k++ )
		{
			uint32 OldIndex = InIndexes[ TriIndex * 3 + k ];
			uint32* NewIndexPtr = white::find(OldToNewIndex,OldIndex );
			uint32 NewIndex = NewIndexPtr ? *NewIndexPtr : ~0u;

			if( NewIndex == ~0u )
			{
				Verts.resize(Verts.size() + GetVertSize() );
				NewIndex = NumVerts++;
				OldToNewIndex.emplace( OldIndex, NewIndex );
				
				const FStaticMeshBuildVertex& InVert = InVerts[ OldIndex ];

				GetPosition( NewIndex ) = InVert.Position;
				GetNormal( NewIndex ) = wm::any_nan(InVert.TangentY) ? wm::UpVector : InVert.TangentZ;
	
				if( bHasColors )
				{
					GetColor( NewIndex ) =as_linear(InVert.Color);
				}

				wm::float2* UVs = GetUVs( NewIndex );
				for( uint32 UVIndex = 0; UVIndex < NumTexCoords; UVIndex++ )
				{
					UVs[UVIndex] = wm::any_nan(InVert.UVs[UVIndex]) ? wm::float2{} : InVert.UVs[UVIndex];
				}

				float* Attributes = GetAttributes( NewIndex );

				// Make sure this vertex is valid from the start
				if( bHasColors )
					CorrectAttributesColor( Attributes );
				else
					CorrectAttributes( Attributes );
			}

			Indexes.emplace_back( NewIndex );
			BoundaryEdges.emplace_back( InBoundaryEdges[ TriIndex * 3 + k ] );
		}

		{
			const wm::float3& Position0 = InVerts[ InIndexes[ TriIndex * 3 + 0 ] ].Position;
			const wm::float3& Position1 = InVerts[ InIndexes[ TriIndex * 3 + 1 ] ].Position;
			const wm::float3& Position2 = InVerts[ InIndexes[ TriIndex * 3 + 2 ] ].Position;

			wm::float3 Edge01 = Position1 - Position0;
			wm::float3 Edge12 = Position2 - Position1;
			wm::float3 Edge20 = Position0 - Position2;

			float TriArea = 0.5f * wm::length(wm::cross( Edge01, Edge20 ));
			SurfaceArea += TriArea;
		}

		MaterialIndexes.emplace_back( InMaterialIndexes[ TriIndex ] );
	}

	FindExternalEdges();
	Bound();
}

// Split
FCluster::FCluster( FCluster& SrcCluster, uint32 TriBegin, uint32 TriEnd, const std::vector< uint32 >& TriIndexes )
	: MipLevel( SrcCluster.MipLevel )
{
	GUID = Murmur32( { SrcCluster.GUID, TriBegin, TriEnd } );

	NumTexCoords = SrcCluster.NumTexCoords;
	bHasColors   = SrcCluster.bHasColors;
	
	const uint32 NumTriangles = TriEnd - TriBegin;

	Verts.reserve( NumTriangles * GetVertSize() );
	Indexes.reserve( 3 * NumTriangles );
	BoundaryEdges.reserve( 3 * NumTriangles );
	MaterialIndexes.reserve( NumTriangles );

	std::unordered_map< uint32, uint32 > OldToNewIndex;
	OldToNewIndex.reserve( NumTriangles );

	for( uint32 i = TriBegin; i < TriEnd; i++ )
	{
		uint32 TriIndex = TriIndexes[i];

		for( uint32 k = 0; k < 3; k++ )
		{
			uint32 OldIndex = SrcCluster.Indexes[ TriIndex * 3 + k ];
			uint32* NewIndexPtr = white::find(OldToNewIndex, OldIndex );
			uint32 NewIndex = NewIndexPtr ? *NewIndexPtr : ~0u;

			if( NewIndex == ~0u )
			{
				Verts.resize(Verts.size()+ GetVertSize() );
				NewIndex = NumVerts++;
				OldToNewIndex.emplace( OldIndex, NewIndex );

				std::memcpy( &GetPosition( NewIndex ), &SrcCluster.GetPosition( OldIndex ), GetVertSize() * sizeof( float ) );
			}

			Indexes.emplace_back( NewIndex );
			BoundaryEdges.emplace_back( SrcCluster.BoundaryEdges[ TriIndex * 3 + k ] );
		}

		{
			const wm::float3& Position0 = SrcCluster.GetPosition( SrcCluster.Indexes[ TriIndex * 3 + 0 ] );
			const wm::float3& Position1 = SrcCluster.GetPosition( SrcCluster.Indexes[ TriIndex * 3 + 1 ] );
			const wm::float3& Position2 = SrcCluster.GetPosition( SrcCluster.Indexes[ TriIndex * 3 + 2 ] );

			wm::float3 Edge01 = Position1 - Position0;
			wm::float3 Edge12 = Position2 - Position1;
			wm::float3 Edge20 = Position0 - Position2;

			float TriArea = 0.5f *wm::length(wm::cross( Edge01 , Edge20 ));
			SurfaceArea += TriArea;
		}

		const int32 MaterialIndex = SrcCluster.MaterialIndexes[ TriIndex ];
		MaterialIndexes.emplace_back( MaterialIndex );
	}

	FindExternalEdges();
	Bound();
}

// Merge
FCluster::FCluster( const std::vector< const FCluster*>& MergeList )
{
	NumTexCoords = MergeList[0]->NumTexCoords;
	bHasColors = MergeList[0]->bHasColors;

	// Only need a guess
	const uint32 NumTriangles = ClusterSize * MergeList.size();

	Verts.reserve( NumTriangles * GetVertSize() );
	Indexes.reserve( 3 * NumTriangles );
	BoundaryEdges.reserve( 3 * NumTriangles );
	MaterialIndexes.reserve( NumTriangles );

	white::FHashTable HashTable( 1 << wm::FloorLog2( NumTriangles ), NumTriangles );

	for( const FCluster* Child : MergeList )
	{
		Bounds += Child->Bounds;
		SurfaceArea	+= Child->SurfaceArea;

		// Can jump multiple levels but guarantee it steps at least 1.
		MipLevel = std::max( MipLevel, Child->MipLevel + 1 );

		for( int32 i = 0; i < Child->Indexes.size(); i++ )
		{
			const wm::float3& Position = Child->GetPosition( Child->Indexes[i] );

			uint32 Hash = HashPosition( Position );
			uint32 NewIndex;
			for( NewIndex = HashTable.First( Hash ); HashTable.IsValid( NewIndex ); NewIndex = HashTable.Next( NewIndex ) )
			{
				if( 0 == std::memcmp( &GetPosition( NewIndex ), &Position, GetVertSize() * sizeof( float ) ) )
				{
					break;
				}
			}
			if( !HashTable.IsValid( NewIndex ) )
			{
				Verts.resize(Verts.size() + GetVertSize() );
				NewIndex = NumVerts++;
				HashTable.Add( Hash, NewIndex );

				std::memcpy( &GetPosition( NewIndex ), &Position, GetVertSize() * sizeof( float ) );
			}

			Indexes.emplace_back( NewIndex );
			BoundaryEdges.emplace_back( Child->BoundaryEdges[i] );
		}

		for (int32 i = 0; i < Child->MaterialIndexes.size(); i++)
		{
			const int32 MaterialIndex = Child->MaterialIndexes[i];
			MaterialIndexes.emplace_back(MaterialIndex);
		}
	}
}

float FCluster::Simplify( uint32 TargetNumTris )
{
	if( TargetNumTris * 3 >= (uint32)Indexes.size() )
	{
		return 0.0f;
	}
	
	float TriangleSize = std::sqrt( SurfaceArea * 3.0f / Indexes.size() );
	
	white::FFloat32 CurrentSize( std::max( TriangleSize, THRESH_POINTS_ARE_SAME ) );
	white::FFloat32 DesiredSize( 0.25f );
	white::FFloat32 FloatScale( 1.0f );

	// Lossless scaling by only changing the float exponent.
	int32 Exponent = wm::clamp( (int)DesiredSize.Components.Exponent - (int)CurrentSize.Components.Exponent, -126, 127 );
	FloatScale.Components.Exponent = Exponent + 127;	//ExpBias
	// Scale ~= DesiredSize / CurrentSize
	float PositionScale = FloatScale.FloatValue;
	
	// Normalize UVWeights using min/max UV range.
	float MinUV[ 8 ] = { +FLT_MAX, +FLT_MAX };
	float MaxUV[ 8 ] = { -FLT_MAX, -FLT_MAX };

	for( uint32 i = 0; i < NumVerts; i++ )
	{
		GetPosition(i) *= PositionScale;

		for( uint32 UVIndex = 0; UVIndex < NumTexCoords; UVIndex++ )
		{
			float U = GetUVs(i)[ UVIndex ].x;
			float V = GetUVs(i)[ UVIndex ].y;
			if (!std::isfinite(U)) U = 0.0f;
			if (!std::isfinite(V)) V = 0.0f;
			MinUV[UVIndex] = std::min({ MinUV[UVIndex], U, V });
			MaxUV[UVIndex] = std::max({ MaxUV[UVIndex], U, V });
		}
	}

	uint32 NumAttributes = GetVertSize() - 3;
	float* AttributeWeights = (float*)lalloca( NumAttributes * sizeof( float ) );

	// Normal
	AttributeWeights[0] = 1.0f;
	AttributeWeights[1] = 1.0f;
	AttributeWeights[2] = 1.0f;

	if( bHasColors )
	{
		float* ColorWeights = AttributeWeights + 3;
		ColorWeights[0] = 0.0625f;
		ColorWeights[1] = 0.0625f;
		ColorWeights[2] = 0.0625f;
		ColorWeights[3] = 0.0625f;
	}
	
	uint32 TexCoordOffset = 3 + ( bHasColors ? 4 : 0 );
	float* UVWeights = AttributeWeights + TexCoordOffset;

	for( uint32 UVIndex = 0; UVIndex < NumTexCoords; UVIndex++ )
	{
		float Range = std::max( 1.0f, MaxUV[ UVIndex ] - MinUV[ UVIndex ] );
		UVWeights[ 2 * UVIndex + 0 ] = 1.0f / ( 1024.0f * NumTexCoords * Range );
		UVWeights[ 2 * UVIndex + 1 ] = 1.0f / ( 1024.0f * NumTexCoords * Range );
	}

	FMeshSimplifier Simplifier( Verts.data(), NumVerts, Indexes.data(), Indexes.size(), MaterialIndexes.data(), NumAttributes );

	Simplifier.SetBoundaryLocked( BoundaryEdges );
	
	Simplifier.SetAttributeWeights( AttributeWeights );
	Simplifier.SetCorrectAttributes( bHasColors ? CorrectAttributesColor : CorrectAttributes );
	Simplifier.SetEdgeWeight( 2.0f );

	float MaxErrorSqr = Simplifier.Simplify( NumVerts, TargetNumTris );

	wconstraint( Simplifier.GetRemainingNumVerts() > 0 );
	wconstraint( Simplifier.GetRemainingNumTris() > 0 );
	
	Simplifier.GetBoundaryUnlocked( BoundaryEdges );
	Simplifier.Compact();
	
	Verts.resize( Simplifier.GetRemainingNumVerts() * GetVertSize() );
	Indexes.resize( Simplifier.GetRemainingNumTris() * 3 );
	MaterialIndexes.resize( Simplifier.GetRemainingNumTris() );

	NumVerts = Simplifier.GetRemainingNumVerts();

	float InvScale = 1.0f / PositionScale;
	for( uint32 i = 0; i < NumVerts; i++ )
	{
		GetPosition(i) *= InvScale;
		Bounds += GetPosition(i);
	}

	return std::sqrt( MaxErrorSqr ) * InvScale;
}

void FCluster::Split(WhiteEngine::FGraphPartitioner& Partitioner ) const
{
	uint32 NumTriangles = Indexes.size() / 3;
	
	white::FDisjointSet DisjointSet( NumTriangles );
	
	std::vector< int32 > SharedEdge;
	SharedEdge.resize( Indexes.size() );

	std::unordered_multimap< uint32, int32 > EdgeHashTable;
	EdgeHashTable.reserve( Indexes.size() );

	for( int i = 0; i < Indexes.size(); i++ )
	{
		uint32 TriI = i / 3;
		uint32 i0 = Indexes[ 3 * TriI + (i + 0) % 3 ];
		uint32 i1 = Indexes[ 3 * TriI + (i + 1) % 3 ];

		uint32 Hash0 = HashPosition( GetPosition( i0 ) );
		uint32 Hash1 = HashPosition( GetPosition( i1 ) );
		uint32 Hash = Murmur32( { std::min( Hash0, Hash1 ), std::max( Hash0, Hash1 ) } );

		bool bFound = false;
		auto range = EdgeHashTable.equal_range(Hash);
		for( auto Iter = range.first; Iter != range.second; ++Iter )
		{
			int32 j = Iter->second;
			if( SharedEdge[j] == -1 )
			{
				uint32 TriJ = j / 3;
				uint32 j0 = Indexes[ 3 * TriJ + (j + 0) % 3 ];
				uint32 j1 = Indexes[ 3 * TriJ + (j + 1) % 3 ];

				if( GetPosition( i0 ) == GetPosition( j1 ) &&
					GetPosition( i1 ) == GetPosition( j0 ) )
				{
					// Link edges
					SharedEdge[i] = TriJ;
					SharedEdge[j] = TriI;
					DisjointSet.UnionSequential( TriI, TriJ );
					bFound = true;
					break;
				}
			}
		}
		if( !bFound )
		{
			EdgeHashTable.emplace( Hash, i );
			SharedEdge[i] = -1;
		}
	}

	auto GetCenter = [ this ]( uint32 TriIndex )
	{
		wm::float3 Center;
		Center  = GetPosition( Indexes[ TriIndex * 3 + 0 ] );
		Center += GetPosition( Indexes[ TriIndex * 3 + 1 ] );
		Center += GetPosition( Indexes[ TriIndex * 3 + 2 ] );
		return Center * (1.0f / 3.0f);
	};

	Partitioner.BuildLocalityLinks( DisjointSet, Bounds, GetCenter );

	auto* wrestrict Graph = Partitioner.NewGraph( NumTriangles * 3 );

	for( uint32 i = 0; i < NumTriangles; i++ )
	{
		Graph->AdjacencyOffset[i] = Graph->Adjacency.size();

		uint32 TriIndex = Partitioner.Indexes[i];

		// Add shared edges
		for( int k = 0; k < 3; k++ )
		{
			int32 AdjIndex = SharedEdge[ 3 * TriIndex + k ];
			if( AdjIndex != -1 )
			{
				Partitioner.AddAdjacency( Graph, AdjIndex, 4 * 65 );
			}
	}

		Partitioner.AddLocalityLinks( Graph, TriIndex, 1 );
	}
	Graph->AdjacencyOffset[ NumTriangles ] = Graph->Adjacency.size();

	Partitioner.PartitionStrict( Graph, ClusterSize - 4, ClusterSize, false );
}

void FCluster::FindExternalEdges()
{
	ExternalEdges.resize(Indexes.size(), true);
	NumExternalEdges = Indexes.size();

	white::FHashTable HashTable( 1 << wm::FloorLog2( Indexes.size() ), Indexes.size() );

	for( int32 EdgeIndex = 0; EdgeIndex < Indexes.size(); EdgeIndex++ )
	{
		if( BoundaryEdges[ EdgeIndex ] )
		{
			ExternalEdges[ EdgeIndex ] = false;
			NumExternalEdges--;
			continue;
		}

		uint32 VertIndex0 = Indexes[ EdgeIndex ];
		uint32 VertIndex1 = Indexes[ Cycle3( EdgeIndex ) ];
	
		const wm::float3& Position0 = GetPosition( VertIndex0 );
		const wm::float3& Position1 = GetPosition( VertIndex1 );
	
		// Find edge with opposite direction that shares these 2 verts.
		/*
			  /\
			 /  \
			o-<<-o
			o->>-o
			 \  /
			  \/
		*/
		uint32 Hash0 = HashPosition( Position0 );
		uint32 Hash1 = HashPosition( Position1 );
		uint32 Hash = Murmur32( { Hash1, Hash0 } );

		uint32 OtherEdgeIndex;
		for( OtherEdgeIndex = HashTable.First( Hash ); HashTable.IsValid( OtherEdgeIndex ); OtherEdgeIndex = HashTable.Next( OtherEdgeIndex ) )
		{
			if( ExternalEdges[ OtherEdgeIndex ] )
			{
				uint32 OtherVertIndex0 = Indexes[ OtherEdgeIndex ];
				uint32 OtherVertIndex1 = Indexes[ Cycle3( OtherEdgeIndex ) ];
			
				if( Position0 == GetPosition( OtherVertIndex1 ) &&
					Position1 == GetPosition( OtherVertIndex0 ) )
				{
					// Found matching edge.
					ExternalEdges[ EdgeIndex ] = false;
					ExternalEdges[ OtherEdgeIndex ] = false;
					NumExternalEdges -= 2;
					break;
				}
			}
		}
		if( !HashTable.IsValid( OtherEdgeIndex ) )
		{
			HashTable.Add( Murmur32( { Hash0, Hash1 } ), EdgeIndex );
		}
	}
}

void FCluster::Bound()
{
	NumTris = Indexes.size() / 3;
	Bounds = FBounds();
	
	std::vector< wm::float3> Positions;
	Positions.resize( NumVerts);

	for( uint32 i = 0; i < NumVerts; i++ )
	{
		Positions[i] = GetPosition(i);
		Bounds += Positions[i];
	}
	SphereBounds = Sphere( Positions.data(), Positions.size() );
	LODBounds = SphereBounds;

	//auto& Normals = Positions;
	//Normals.Reset( Cluster.NumTris );

	wm::float3 SurfaceMean{};
	
	float MaxEdgeLength2 = 0.0f;
	wm::float3 AvgNormal = {};
	for( int i = 0; i < Indexes.size(); i += 3 )
	{
		wm::float3 v[3];
		v[0] = GetPosition( Indexes[ i + 0 ] );
		v[1] = GetPosition( Indexes[ i + 1 ] );
		v[2] = GetPosition( Indexes[ i + 2 ] );

		wm::float3 Edge01 = v[1] - v[0];
		wm::float3 Edge12 = v[2] - v[1];
		wm::float3 Edge20 = v[0] - v[2];

		MaxEdgeLength2 = std::max( MaxEdgeLength2,wm::length_sq( Edge01) );
		MaxEdgeLength2 = std::max( MaxEdgeLength2, wm::length_sq(Edge12) );
		MaxEdgeLength2 = std::max( MaxEdgeLength2, wm::length_sq(Edge20) );



		float TriArea = 0.5f *wm::length(wm::cross( Edge01 , Edge20 ));

		for( int k = 0; k < 3; k++ )
			SurfaceMean += TriArea * v[k];
	}
	EdgeLength = std::sqrt( MaxEdgeLength2 );
	SurfaceMean /= 3.0f * SurfaceArea;
}

template<typename FArchive>
FArchive& operator<<(FArchive& Ar, FMaterialRange& Range)
{
	Ar << Range.RangeStart;
	Ar << Range.RangeLength;
	Ar << Range.MaterialIndex;
	return Ar;
}

template<typename FArchive>
FArchive& operator<<(FArchive& Ar, FStripDesc& Desc)
{
	for (uint32 i = 0; i < 4; i++)
	{
		for (uint32 j = 0; j < 3; j++)
		{
			Ar << Desc.Bitmasks[i][j];
		}
	}
	Ar << Desc.NumPrevRefVerticesBeforeDwords;
	Ar << Desc.NumPrevNewVerticesBeforeDwords;
	return Ar;
}

template<typename FArchive>
FArchive& operator<<(FArchive& Ar, FCluster& Cluster)
{
	Ar << Cluster.NumVerts;
	Ar << Cluster.NumTris;
	Ar << Cluster.NumTexCoords;
	Ar << Cluster.bHasColors;

	Ar << Cluster.Verts;
	Ar << Cluster.Indexes;
	Ar << Cluster.MaterialIndexes;
	Ar << Cluster.BoundaryEdges;
	Ar << Cluster.ExternalEdges;
	Ar << Cluster.NumExternalEdges;

	Ar << Cluster.AdjacentClusters;

	Ar << Cluster.Bounds;
	Ar << Cluster.SurfaceArea;
	Ar << Cluster.GUID;
	Ar << Cluster.MipLevel;

	Ar << Cluster.QuantizedPosStart;
	Ar << Cluster.QuantizedPosShift;

	Ar << Cluster.MeshBoundsMin;
	Ar << Cluster.MeshBoundsDelta;

	Ar << Cluster.EdgeLength;
	Ar << Cluster.LODError;

	Ar << Cluster.SphereBounds;
	Ar << Cluster.LODBounds;

	Ar << Cluster.GroupIndex;
	Ar << Cluster.GroupPartIndex;
	Ar << Cluster.GeneratingGroupIndex;

	Ar << Cluster.MaterialRanges;
	Ar << Cluster.QuantizedPositions;

	Ar << Cluster.StripDesc;
	Ar << Cluster.StripIndexData;
	return Ar;
}

} // namespace Nanite