// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Core/Hash/Murmur.h"
#include "Math/PlatformMath.h"
#include "Developer/MeshBuild.h"

#include <unordered_map>

template< typename T > uint32 Min3Index( const T A, const T B, const T C ) { return ( A < B ) ? ( ( A < C ) ? 0 : 2 ) : ( ( B < C ) ? 1 : 2 ); }
template< typename T > uint32 Max3Index( const T A, const T B, const T C ) { return ( A > B ) ? ( ( A > C ) ? 0 : 2 ) : ( ( B > C ) ? 1 : 2 ); }

uint32 Hash3( const uint32 x, const uint32 y, const uint32 z )
{
	//return ( 73856093 * x ) ^ ( 15485867 * y ) ^ ( 83492791 * z );
	return Murmur32( { x, y, z } );
}

uint32 HashPoint( const wm::float3 Point )
{
	int32 x = wm::FloorToInt( Point.x / ( 2.0f * WhiteEngine::THRESH_POINTS_ARE_SAME ) );
	int32 y = wm::FloorToInt( Point.y / ( 2.0f * WhiteEngine::THRESH_POINTS_ARE_SAME ) );
	int32 z = wm::FloorToInt( Point.z / ( 2.0f * WhiteEngine::THRESH_POINTS_ARE_SAME ) );

	return Hash3( x, y, z );
}

uint32 HashPoint( const wm::float3 Point, const uint32 Octant )
{
	int32 x = wm::FloorToInt( Point.x / ( 2.0f * WhiteEngine::THRESH_POINTS_ARE_SAME ) - 0.5f );
	int32 y = wm::FloorToInt( Point.y / ( 2.0f * WhiteEngine::THRESH_POINTS_ARE_SAME ) - 0.5f );
	int32 z = wm::FloorToInt( Point.z / ( 2.0f * WhiteEngine::THRESH_POINTS_ARE_SAME ) - 0.5f );

	x += ( Octant >> 0 ) & 1;
	y += ( Octant >> 1 ) & 1;
	z += ( Octant >> 2 ) & 1;

	return Hash3( x, y, z );
}

template< typename T >
wm::float3 GetPosition( const T& Vert )
{
	// Assumes XYZ floats at start of type T
	return *reinterpret_cast< const wm::float3* >( &Vert );
}

/** Split edges until less than MaxEdgeSize */
template< typename T, class LerpClass >
void MeshTessellate( std::vector<T>& Verts, std::vector< uint32 >& Indexes, float MaxEdgeSize, LerpClass Lerp )
{
	wconstraint( Indexes.size() % 3 == 0 );

	std::unordered_multimap< uint32, int32 > HashTable;
	HashTable.reserve( Verts.size()*2 );
	for( size_t i = 0; i < Verts.size(); i++ )
	{
		const T& Vert = Verts[i];
		HashTable.emplace( HashPoint( GetPosition( Vert ) ), i );
	}
	
	
	/*
	===========
		v0
		/\
	e2 /  \ e0
	  /____\
	v2  e1  v1
	===========
	*/

	for( size_t Tri = 0; Tri < Indexes.size(); )
	{
		float EdgeLength2[3];
		for( int i = 0; i < 3; i++ )
		{
			const uint32 i0 = i;
			const uint32 i1 = (1 << i0) & 3;

			auto p0 = GetPosition( Verts[ Indexes[ Tri + i0 ] ] );
			auto p1 = GetPosition( Verts[ Indexes[ Tri + i1 ] ] );

			EdgeLength2[i] = wm::length_sq( p0 - p1 );
		}

		const uint32 e0 = Max3Index( EdgeLength2[0], EdgeLength2[1], EdgeLength2[2] );
		const uint32 e1 = (1 << e0) & 3;
		const uint32 e2 = (1 << e1) & 3;
		if( EdgeLength2[ e0 ] < MaxEdgeSize * MaxEdgeSize )
		{
			// Triangle is small enough
			Tri += 3;
			continue;
		}
		
		const uint32 i0 = Indexes[ Tri + e0 ];
		const uint32 i1 = Indexes[ Tri + e1 ];
		const uint32 i2 = Indexes[ Tri + e2 ];

		const T MidV = Lerp( Verts[i0], Verts[i1], 0.5f );
		uint32  MidI = ~0u;

		auto MidPosition = GetPosition( MidV );

		// Find if there already exists one
		for( uint32 Octant = 0; Octant < 8; Octant++ )
		{
			uint32 Hash = HashPoint( MidPosition, Octant );
			auto range = HashTable.equal_range(Hash);
			for( auto Iter = range.first;Iter != range.second;++Iter)
			{
				if( std::memcmp( &MidV, &Verts[ Iter->second ], sizeof(T) ) == 0 )
				{
					MidI = Iter->second;
					break;
				}
			}
			if( MidI != ~0u )
			{
				break;
			}
		}
		if( MidI == ~0u )
		{
			MidI = Verts.emplace_back( MidV );
			HashTable.emplace( HashPoint( MidPosition ), MidI );
		}

		if( !WAssert( MidI != i0 && MidI != i1 && MidI != i2,"Degenerate triangle generated" ) )
		{
			Tri += 3;
			continue;
		}
	
		// Replace e0
		Indexes.emplace_back( MidI );
		Indexes.emplace_back( i1 );
		Indexes.emplace_back( i2 );

		// Replace e1
		Indexes[ Tri + e1 ] = MidI;
	}
}

template< typename T, class ShouldWeldClass >
void WeldVerts(std::vector<T>& Verts, std::vector< uint32 >& Indexes, ShouldWeldClass ShouldWeld )
{
	uint32 NumTris = Indexes.Num() / 3;

	std::vector<T>			WeldedVerts;
	std::vector< uint32 >	WeldedIndexes;

	WeldedVerts.reserve( NumTris * 2 );
	WeldedIndexes.reserve( Indexes.Num() );

	std::unordered_multimap< uint32, int32 > HashTable;
	HashTable.reserve( NumTris * 2 );

	for( size_t i = 0; i < Indexes.size(); i++ )
	{
		const T& Vert = Verts[ Indexes[i] ];
		
		// Assumes XYZ floats at start of type T
		auto Position = *reinterpret_cast< const wm::float3* >( &Vert );

		// Find if there already exists one within Equals threshold
		uint32 Index = ~0u;
		for( uint32 Octant = 0; Octant < 8; Octant++ )
		{
			uint32 Hash = HashPoint( Position, Octant );

			auto range = HashTable.equal_range(Hash);
			for (auto Iter = range.first; Iter != range.second; ++Iter)
			{
				if( ShouldWeld( Vert, WeldedVerts[ Iter->second ] ) )
				{
					Index = Iter->second;
					break;
				}
			}
			if( Index != ~0u )
			{
				break;
			}
		}
		if( Index == ~0u )
		{
			Index = WeldedVerts.emplace_back( Vert );
			WeldedIndexes.emplace_back( Index );
			HashTable.emplace( HashPoint( Position ), Index );
		}
	}

	std::swap( Verts,	WeldedVerts );
	std::swap( Indexes,	WeldedIndexes );
}