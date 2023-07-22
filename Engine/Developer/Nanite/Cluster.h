// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include <WBase/winttype.hpp>
#include <WBase/span.hpp>

#include "Bounds.h"

#include "Runtime/Renderer/Nanite.h"
#include "Core/Container/BitArray.h"
#include "Core/Math/Sphere.h"

#include "RenderInterface/Color_T.hpp"

#include <unordered_map>

namespace WhiteEngine {
	class FGraphPartitioner;
}

namespace Nanite
{
using namespace WhiteEngine;
using namespace white::inttype;

/** The information used to build a static-mesh vertex. */
struct FStaticMeshBuildVertex
{
	wm::float3 Position;

	wm::float3 TangentX;
	wm::float3 TangentY;
	wm::float3 TangentZ;

	wm::float2 UVs[8];
	FColor Color;
};

struct FMaterialRange
{
	white::uint32 RangeStart;
	white::uint32 RangeLength;
	white::uint32 MaterialIndex;

	template<typename FArchive>
	friend FArchive& operator<<(FArchive& Ar, FMaterialRange& Range);
};

struct FStripDesc
{
	white::uint32 Bitmasks[4][3];
	white::uint32 NumPrevRefVerticesBeforeDwords;
	white::uint32 NumPrevNewVerticesBeforeDwords;

	template<typename FArchive>
	friend FArchive& operator<<(FArchive& Ar, FStripDesc& Desc);
};

class FCluster
{
public:
	FCluster() {}
	FCluster(
		const std::vector< FStaticMeshBuildVertex >& InVerts,
		const white::span< const uint32 >& InIndexes,
		const white::span< const int32 >& InMaterialIndexes,
		const white::TBitArray<>& InBoundaryEdges,
		uint32 TriBegin, uint32 TriEnd, const std::vector< uint32 >& TriIndexes, uint32 NumTexCoords, bool bHasColors );

	FCluster( FCluster& SrcCluster, uint32 TriBegin, uint32 TriEnd, const std::vector< uint32 >& TriIndexes );
	FCluster( const std::vector< const FCluster*>& MergeList );

	float	Simplify( uint32 NumTris );
	void	Split( FGraphPartitioner& Partitioner ) const;
	void	Bound();

private:
	void	FindExternalEdges();

public:
	uint32				GetVertSize() const;
	wm::float3&			GetPosition( uint32 VertIndex );
	float*				GetAttributes( uint32 VertIndex );
	wm::float3&			GetNormal( uint32 VertIndex );
	LinearColor&		GetColor( uint32 VertIndex );
	wm::float2*			GetUVs( uint32 VertIndex );

	const wm::float3&		GetPosition( uint32 VertIndex ) const;
	const wm::float3&		GetNormal( uint32 VertIndex ) const;
	const LinearColor&	GetColor( uint32 VertIndex ) const;
	const wm::float2*	GetUVs( uint32 VertIndex ) const;

	template<typename FArchive>
	friend FArchive& operator<<(FArchive& Ar, FCluster& Cluster);

	static const uint32	ClusterSize = 128;

	white::uint32		NumVerts = 0;
	white::uint32		NumTris = 0;
	white::uint32		NumTexCoords = 0;
	bool		bHasColors = false;

	std::vector< float >		Verts;
	std::vector< uint32 >	Indexes;
	std::vector< int32 >		MaterialIndexes;
	white::TBitArray<>			BoundaryEdges;
	white::TBitArray<>			ExternalEdges;
	white::uint32				NumExternalEdges;

	std::unordered_map< uint32, uint32 >	AdjacentClusters;

	FBounds		Bounds;
	float		SurfaceArea = 0.0f;
	uint32		GUID = 0;
	int32		MipLevel = 0;

	wm::int3	QuantizedPosStart	= { 0u, 0u, 0u };
	uint32		QuantizedPosShift	= 0u;
	wm::int3  QuantizedPosBits	= {};

	wm::float3		MeshBoundsMin;
	wm::float3		MeshBoundsDelta;

	float		EdgeLength = 0.0f;
	float		LODError = 0.0f;
	
	Sphere		SphereBounds;
	Sphere		LODBounds;

	uint32		GroupIndex			= wm::MAX_uint32;
	uint32		GroupPartIndex		= wm::MAX_uint32;
	uint32		GeneratingGroupIndex= wm::MAX_uint32;

	std::vector<FMaterialRange> MaterialRanges;
	std::vector<wm::int3>	QuantizedPositions;

	FStripDesc		StripDesc;
	std::vector<uint8>	StripIndexData;
};

inline uint32 FCluster::GetVertSize() const
{
	return 6 + ( bHasColors ? 4 : 0 ) + NumTexCoords * 2;
}

inline wm::float3& FCluster::GetPosition( uint32 VertIndex )
{
	return *reinterpret_cast< wm::float3* >( &Verts[ VertIndex * GetVertSize() ] );
}

inline const wm::float3& FCluster::GetPosition( uint32 VertIndex ) const
{
	return *reinterpret_cast< const wm::float3* >( &Verts[ VertIndex * GetVertSize() ] );
}

inline float* FCluster::GetAttributes( uint32 VertIndex )
{
	return &Verts[ VertIndex * GetVertSize() + 3 ];
}

inline wm::float3& FCluster::GetNormal( uint32 VertIndex )
{
	return *reinterpret_cast< wm::float3* >( &Verts[ VertIndex * GetVertSize() + 3 ] );
}

inline const wm::float3& FCluster::GetNormal( uint32 VertIndex ) const
{
	return *reinterpret_cast< const wm::float3* >( &Verts[ VertIndex * GetVertSize() + 3 ] );
}

inline LinearColor& FCluster::GetColor( uint32 VertIndex )
{
	return *reinterpret_cast< LinearColor* >( &Verts[ VertIndex * GetVertSize() + 6 ] );
}

inline const LinearColor& FCluster::GetColor( uint32 VertIndex ) const
{
	return *reinterpret_cast< const LinearColor* >( &Verts[ VertIndex * GetVertSize() + 6 ] );
}

inline wm::float2* FCluster::GetUVs( uint32 VertIndex )
{
	return reinterpret_cast< wm::float2* >( Verts.data() + VertIndex * GetVertSize() + 6 + ( bHasColors ? 4 : 0 ));
}

inline const wm::float2* FCluster::GetUVs( uint32 VertIndex ) const
{
	return reinterpret_cast< const wm::float2* >( &Verts[ VertIndex * GetVertSize() + 6 + ( bHasColors ? 4 : 0 ) ] );
}

} // namespace Nanite