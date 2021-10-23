// Copyright Epic Games, Inc. All Rights Reserved.

#include "MeshSimplify.h"
#if WB_IMPL_MSCPP
#pragma warning(disable:4267)
#endif

FMeshSimplifier::FMeshSimplifier( float* InVerts, uint32 InNumVerts, uint32* InIndexes, uint32 InNumIndexes, int32* InMaterialIndexes, uint32 InNumAttributes )
	: NumVerts( InNumVerts )
	, NumIndexes( InNumIndexes )
	, NumAttributes( InNumAttributes )
	, NumTris( NumIndexes / 3 )
	, RemainingNumVerts( NumVerts )
	, RemainingNumTris( NumTris )
	, Verts( InVerts )
	, Indexes( InIndexes )
	, MaterialIndexes( InMaterialIndexes )
	, VertHash( 1 << std::min( 16u, wm::FloorLog2( NumVerts ) ) )
	, CornerHash( 1 << std::min( 16u, wm::FloorLog2( NumIndexes ) ) )
	, TriRemoved( false, NumTris )
{
	for( uint32 VertIndex = 0; VertIndex < NumVerts; VertIndex++ )
	{
		VertHash.Add( HashPosition( GetPosition( VertIndex ) ), VertIndex );
	}

	VertRefCount.resize(VertRefCount.size() + NumVerts);
	CornerFlags.resize(CornerFlags.size()+ NumIndexes );
	
	EdgeQuadrics.resize(EdgeQuadrics.size()+ NumIndexes );
	EdgeQuadricsValid.resize(NumIndexes,false);

	// Guess number of edges based on Euler's formula.
	uint32 NumEdges = std::min({ NumIndexes, 3 * NumVerts - 6, NumTris + NumVerts });
	Pairs.reserve( NumEdges );
	PairHash0.Clear( 1 << std::min( 16u, wm::FloorLog2( NumEdges ) ), NumEdges );
	PairHash1.Clear( 1 << std::min( 16u, wm::FloorLog2( NumEdges ) ), NumEdges );

	for( uint32 Corner = 0; Corner < NumIndexes; Corner++ )
	{
		uint32 VertIndex = Indexes[ Corner ];
		
		VertRefCount[ VertIndex ]++;

		const wm::float3& Position = GetPosition( VertIndex );
		CornerHash.Add( HashPosition( Position ), Corner );

		uint32 OtherCorner = Cycle3( Corner );

		FPair Pair;
		Pair.Position0 = Position;
		Pair.Position1 = GetPosition( Indexes[ Cycle3( Corner ) ] );

		if( AddUniquePair( Pair, Pairs.size() ) )
		{
			Pairs.emplace_back( Pair );
		}
	}
}

bool FMeshSimplifier::AddUniquePair( FPair& Pair, uint32 PairIndex )
{
	uint32 Hash0 = HashPosition( Pair.Position0 );
	uint32 Hash1 = HashPosition( Pair.Position1 );

	if( Hash0 > Hash1 )
	{
		std::swap( Hash0, Hash1 );
		std::swap( Pair.Position0, Pair.Position1 );
	}

	uint32 OtherPairIndex;
	for( OtherPairIndex = PairHash0.First( Hash0 ); PairHash0.IsValid( OtherPairIndex ); OtherPairIndex = PairHash0.Next( OtherPairIndex ) )
	{
		wconstraint( PairIndex != OtherPairIndex );

		FPair& OtherPair = Pairs[ OtherPairIndex ];

		if( Pair.Position0 == OtherPair.Position0 &&
			Pair.Position1 == OtherPair.Position1 )
		{
			// Found a duplicate
			return false;
		}
	}

	PairHash0.Add( Hash0, PairIndex );
	PairHash1.Add( Hash1, PairIndex );

	return true;
}

void FMeshSimplifier::CalcTriQuadric( uint32 TriIndex )
{
	uint32 i0 = Indexes[ TriIndex * 3 + 0 ];
	uint32 i1 = Indexes[ TriIndex * 3 + 1 ];
	uint32 i2 = Indexes[ TriIndex * 3 + 2 ];

	new( &GetTriQuadric( TriIndex ) ) FQuadricAttr(
		GetPosition( i0 ),
		GetPosition( i1 ),
		GetPosition( i2 ),
		GetAttributes( i0 ),
		GetAttributes( i1 ),
		GetAttributes( i2 ),
		AttributeWeights,
		NumAttributes );
}

void FMeshSimplifier::CalcEdgeQuadric( uint32 EdgeIndex )
{
	uint32 TriIndex = EdgeIndex / 3;
	if( TriRemoved[ TriIndex ] )
	{
		EdgeQuadricsValid[ EdgeIndex ] = false;
		return;
	}

	int32 MaterialIndex = MaterialIndexes[ TriIndex ];

	uint32 VertIndex0 = Indexes[ EdgeIndex ];
	uint32 VertIndex1 = Indexes[ Cycle3( EdgeIndex ) ];

	const wm::float3& Position0 = GetPosition( VertIndex0 );
	const wm::float3& Position1 = GetPosition( VertIndex1 );
	
	// Find edge with opposite direction that shares these 2 verts.
	// If none then we need to add an edge constraint.
	/*
		  /\
		 /  \
		o-<<-o
		o->>-o
		 \  /
		  \/
	*/
	uint32 Hash = HashPosition( Position1 );
	uint32 Corner;
	for( Corner = CornerHash.First( Hash ); CornerHash.IsValid( Corner ); Corner = CornerHash.Next( Corner ) )
	{
		uint32 OtherVertIndex0 = Indexes[ Corner ];
		uint32 OtherVertIndex1 = Indexes[ Cycle3( Corner ) ];
			
		if( VertIndex0 == OtherVertIndex1 &&
			VertIndex1 == OtherVertIndex0 &&
			MaterialIndex == MaterialIndexes[ Corner / 3 ] )
		{
			// Found matching edge.
			// No constraints needed so remove any that exist.
			EdgeQuadricsValid[ EdgeIndex ] = false;
			return;
		}
	}

	// Don't double count attribute discontinuities.
	float Weight = EdgeWeight;
	for( Corner = CornerHash.First( Hash ); CornerHash.IsValid( Corner ); Corner = CornerHash.Next( Corner ) )
	{
		uint32 OtherVertIndex0 = Indexes[ Corner ];
		uint32 OtherVertIndex1 = Indexes[ Cycle3( Corner ) ];
			
		if( Position0 == GetPosition( OtherVertIndex1 ) &&
			Position1 == GetPosition( OtherVertIndex0 ) )
		{
			// Found matching edge.
			Weight *= 0.5f;
			break;;
		}
	}

	// Didn't find matching edge. Add edge constraint.
	EdgeQuadrics[ EdgeIndex ] = FEdgeQuadric( GetPosition( VertIndex0 ), GetPosition( VertIndex1 ), GetNormal( TriIndex ), Weight );
	EdgeQuadricsValid[ EdgeIndex ] = true;
}

bool FMeshSimplifier::IsBoundaryEdge( uint32 EdgeIndex ) const
{
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
	uint32 Hash = HashPosition( Position1 );
	for( uint32 Corner = CornerHash.First( Hash ); CornerHash.IsValid( Corner ); Corner = CornerHash.Next( Corner ) )
	{
		uint32 OtherVertIndex0 = Indexes[ Corner ];
		uint32 OtherVertIndex1 = Indexes[ Cycle3( Corner ) ];
			
		if( Position0 == GetPosition( OtherVertIndex1 ) &&
			Position1 == GetPosition( OtherVertIndex0 ) )
		{
			// Found matching edge.
			return false;
		}
	}

	return true;
}

void FMeshSimplifier::SetBoundaryLocked( const white::TBitArray<>& UnlockedBoundaryEdges )
{
	wconstraint( UnlockedBoundaryEdges.size() == NumIndexes );

	for( uint32 EdgeIndex = 0; EdgeIndex < NumIndexes; EdgeIndex++ )
	{
		if( !UnlockedBoundaryEdges[ EdgeIndex ] && IsBoundaryEdge( EdgeIndex ) )
		{
			uint32 VertIndex0 = Indexes[ EdgeIndex ];
			uint32 VertIndex1 = Indexes[ Cycle3( EdgeIndex ) ];
	
			const wm::float3& Position0 = GetPosition( VertIndex0 );
			const wm::float3& Position1 = GetPosition( VertIndex1 );

			CornerFlags[ EdgeIndex ] |= LockedEdgeMask;

			auto LockCorner =
				[ this ]( uint32 Corner )
				{
					CornerFlags[ Corner ] |= LockedVertMask;
				};
			ForAllCorners( Position0, LockCorner );
			ForAllCorners( Position1, LockCorner );

			// Just in case this triangle is removed.
			LockedEdges.insert( std::make_tuple( Position0, Position1 ) );
		}
	}
}

void FMeshSimplifier::GetBoundaryUnlocked( white::TBitArray<>& UnlockedBoundaryEdges )
{
	UnlockedBoundaryEdges.resize(RemainingNumTris * 3,false);

	uint32 OutputEdgeIndex = 0;

	for( uint32 EdgeIndex = 0; EdgeIndex < NumIndexes; EdgeIndex++ )
	{
		if( TriRemoved[ EdgeIndex / 3 ] )
			continue;

		// TODO move to flags
		bool bPossibleBoundary = EdgeQuadricsValid[ EdgeIndex ];
		if( bPossibleBoundary )
		{
			uint8 Flag = CornerFlags[ EdgeIndex ];			

			bool bIsEdgeLocked = Flag & LockedEdgeMask;
			bool bPossibleLost = ( Flag & ( LockedVertMask | LockedEdgeMask ) ) == LockedVertMask;

			// Only possible if both verts are locked
			if( bPossibleLost )
				bPossibleLost = CornerFlags[ Cycle3( EdgeIndex ) ] & LockedVertMask;

			if( bPossibleLost )
			{
				auto Edge = std::make_tuple(
					GetPosition( Indexes[ EdgeIndex ] ),
					GetPosition( Indexes[ Cycle3( EdgeIndex ) ] )
				);
				if( LockedEdges.contains( Edge ) )
				{
					bIsEdgeLocked = true;
				}
			}
			
			if( !bIsEdgeLocked && IsBoundaryEdge( EdgeIndex ) )
			{
				UnlockedBoundaryEdges[ OutputEdgeIndex ] = true;
			}
		}

		OutputEdgeIndex++;
	}
	wconstraint( OutputEdgeIndex == RemainingNumTris * 3 );
}

void FMeshSimplifier::GatherAdjTris( const wm::float3& Position, uint32 Flag, std::vector< uint32>& AdjTris, int32& VertDegree, uint32& FlagsUnion )
{
	struct FWedgeVert
	{
		uint32 VertIndex;
		uint32 AdjTriIndex;
	};
	std::vector< FWedgeVert > WedgeVerts;

	ForAllCorners( Position,
		[ this, &AdjTris, &WedgeVerts, &VertDegree, Flag, &FlagsUnion ]( uint32 Corner )
		{
			VertDegree++;
			
			{
				uint8& wrestrict CornerFlag = CornerFlags[ Corner ];
				FlagsUnion |= CornerFlag;
				CornerFlag |= Flag;
			}
			
			uint32 TriIndex = Corner / 3;
			uint32 AdjTriIndex;
			bool bNewTri = true;
			
			uint8& wrestrict FirstCornerFlag = CornerFlags[ TriIndex * 3 ];
			if( ( FirstCornerFlag & TriMask ) == 0 )
			{
				FirstCornerFlag |= TriMask;
				AdjTriIndex = AdjTris.size();
				AdjTris.emplace_back(TriIndex);
				WedgeDisjointSet.AddDefaulted();
			}
			else
			{
				// Should only happen 2 times per collapse on average
				AdjTriIndex = white::find_index(AdjTris, TriIndex );
				bNewTri = false;
			}

			uint32 VertIndex = Indexes[ Corner ];
			uint32 OtherAdjTriIndex = ~0u;
			for( FWedgeVert& WedgeVert : WedgeVerts )
			{
				if( VertIndex == WedgeVert.VertIndex )
				{
					OtherAdjTriIndex = WedgeVert.AdjTriIndex;
					break;
				}
			}
			if( OtherAdjTriIndex == ~0u )
			{
				WedgeVerts.emplace_back<FWedgeVert>( { VertIndex, AdjTriIndex } );
			}
			else
			{
				if( bNewTri )
					WedgeDisjointSet.UnionSequential( AdjTriIndex, OtherAdjTriIndex );
				else
					WedgeDisjointSet.Union( AdjTriIndex, OtherAdjTriIndex );
			}
		} );
}

float FMeshSimplifier::EvaluateMerge( const wm::float3& Position0, const wm::float3& Position1, bool bMoveVerts )
{
	wconstraint( Position0 != Position1 );

	// Find unique adjacent triangles
	std::vector< uint32> AdjTris;

	WedgeDisjointSet.Reset();

	int32 VertDegree = 0;

	uint32 FlagsUnion0 = 0;
	uint32 FlagsUnion1 = 0;

	GatherAdjTris( Position0, 1, AdjTris, VertDegree, FlagsUnion0 );
	GatherAdjTris( Position1, 2, AdjTris, VertDegree, FlagsUnion1 );

	if( VertDegree == 0 )
	{
		return 0.0f;
	}

	bool bLocked0 = FlagsUnion0 & LockedVertMask;
	bool bLocked1 = FlagsUnion1 & LockedVertMask;

	float Penalty = 0.0f;

	if( VertDegree > DegreeLimit )
		Penalty += DegreePenalty * ( VertDegree - DegreeLimit );
	
	std::vector< uint32>	WedgeIDs;
	std::vector< uint8>	WedgeQuadrics;
	
	const uint32 QuadricSize = sizeof( FQuadricAttr ) + NumAttributes * 4 * sizeof( QScalar );

	auto GetWedgeQuadric =
		[ &WedgeQuadrics, QuadricSize ]( int32 WedgeIndex ) -> FQuadricAttr&
		{
			return *reinterpret_cast< FQuadricAttr* >( &WedgeQuadrics[ WedgeIndex * QuadricSize ] );
		};

	for( uint32 AdjTriIndex = 0, Num = AdjTris.size(); AdjTriIndex < Num; AdjTriIndex++ )
	{
		uint32 TriIndex = AdjTris[ AdjTriIndex ];

		FQuadricAttr& wrestrict TriQuadric = GetTriQuadric( TriIndex );

		uint32 WedgeID = WedgeDisjointSet.Find( AdjTriIndex );
		int32 WedgeIndex = white::find_index(WedgeIDs,WedgeID );
		if( WedgeIndex != white::INDEX_NONE )
		{
			FQuadricAttr& wrestrict WedgeQuadric = GetWedgeQuadric( WedgeIndex );

#if SIMP_REBASE
			uint32 VertIndex0 = Indexes[ TriIndex * 3 ];
			WedgeQuadric.Add( TriQuadric, GetPosition( VertIndex0 ) - Position0, GetAttributes( VertIndex0 ), AttributeWeights, NumAttributes );
#else
			WedgeQuadric.Add( TriQuadric, NumAttributes );
#endif
		}
		else
		{
			WedgeIndex = WedgeIDs.size();
			WedgeIDs.emplace_back(WedgeID);
			WedgeQuadrics.resize(WedgeQuadrics.size() + QuadricSize );

			FQuadricAttr& wrestrict WedgeQuadric = GetWedgeQuadric( WedgeIndex );

			std::memcpy( &WedgeQuadric, &TriQuadric, QuadricSize );
#if SIMP_REBASE
			uint32 VertIndex0 = Indexes[ TriIndex * 3 ];
			WedgeQuadric.Rebase( GetPosition( VertIndex0 ) - Position0, GetAttributes( VertIndex0 ), AttributeWeights, NumAttributes );
#endif
		}
	}

	FQuadricAttrOptimizer QuadricOptimizer;
	for( int32 WedgeIndex = 0, Num = WedgeIDs.size(); WedgeIndex < Num; WedgeIndex++ )
	{
		QuadricOptimizer.AddQuadric( GetWedgeQuadric( WedgeIndex ), NumAttributes );
	}

	wm::float3	BoundsMin = { wm::MAX_flt,  wm::MAX_flt,  wm::MAX_flt };
	wm::float3	BoundsMax = { -wm::MAX_flt, -wm::MAX_flt, -wm::MAX_flt };

	FQuadric EdgeQuadric;
	EdgeQuadric.Zero();

	for( uint32 TriIndex : AdjTris )
	{
		for( uint32 CornerIndex = 0; CornerIndex < 3; CornerIndex++ )
		{
			uint32 Corner = TriIndex * 3 + CornerIndex;

			const wm::float3& Position = GetPosition( Indexes[ Corner ] );

			BoundsMin = wm::min( BoundsMin, Position );
			BoundsMax = wm::max( BoundsMax, Position );
			
			if( EdgeQuadricsValid[ Corner ] )
			{
				// Only if edge is part of this pair
				uint32 EdgeFlags;
				EdgeFlags  = CornerFlags[ Corner ];
				EdgeFlags |= CornerFlags[ TriIndex * 3 + ( ( 1 << CornerIndex ) & 3 ) ];
				if( EdgeFlags & MergeMask )
				{
#if SIMP_REBASE
					EdgeQuadric.Add( EdgeQuadrics[ Corner ], GetPosition( Indexes[ Corner ] ) - Position0 );
#else
					uint32 VertIndex0 = Indexes[ Corner ];
					uint32 VertIndex1 = Indexes[ Cycle3( Corner ) ];
					//EdgeQuadric += FQuadric( GetPosition( VertIndex0 ), GetPosition( VertIndex1 ), GetNormal( TriIndex ), EdgeWeight );
					EdgeQuadric.Add( EdgeQuadrics[ Corner ], GetPosition( Indexes[ Corner ] ) - QuadricOrigin );
#endif
				}
			}
		}
	}

	QuadricOptimizer.AddQuadric( EdgeQuadric );
	
	auto IsValidPosition =
		[ this, &AdjTris, &BoundsMin, &BoundsMax ]( const wm::float3& Position ) -> bool
		{
			// Limit position to be near the neighborhood bounds
			if( ComputeSquaredDistanceFromBoxToPoint( BoundsMin, BoundsMax, Position ) > wm::length_sq( BoundsMax - BoundsMin ) * 4.0f )
				return false;

			for( uint32 TriIndex : AdjTris )
			{
				if( TriWillInvert( TriIndex, Position ) )
					return false;
			}

			return true;
		};

	wm::float3 NewPosition;
	{
		if( bLocked0 && bLocked1 )
			Penalty += LockPenalty;

		// find position
		if( bLocked0 && !bLocked1 )
		{
			NewPosition = Position0;

			if( !IsValidPosition( NewPosition ) )
				Penalty += InversionPenalty;
		}
		else if( bLocked1 && !bLocked0 )
		{
			NewPosition = Position1;

			if( !IsValidPosition( NewPosition ) )
				Penalty += InversionPenalty;
		}
		else
		{
			bool bIsValid = QuadricOptimizer.OptimizeVolume( NewPosition );
#if SIMP_REBASE
			NewPosition += Position0;
#endif
			if( bIsValid )
				bIsValid = IsValidPosition( NewPosition );

			if( !bIsValid )
			{
				bIsValid = QuadricOptimizer.Optimize( NewPosition );
#if SIMP_REBASE
				NewPosition += Position0;
#endif
				if( bIsValid )
					bIsValid = IsValidPosition( NewPosition );
			}
			
			if( !bIsValid )
			{
				// Try a point on the edge.
#if SIMP_REBASE
				bIsValid = QuadricOptimizer.OptimizeLinear(wm::float3{}, Position1 - Position0, NewPosition);
				NewPosition += Position0;
#else
				bIsValid = QuadricOptimizer.OptimizeLinear( Position0, Position1, NewPosition );
#endif
				if( bIsValid )
					bIsValid = IsValidPosition( NewPosition );
			}

			if( !bIsValid )
			{
				// Couldn't find optimal so choose middle
				NewPosition = ( Position0 + Position1 ) * 0.5f;

				if( !IsValidPosition( NewPosition ) )
					Penalty += InversionPenalty;
			}
		}
	}

	int32 NumWedges = WedgeIDs.size();
	WedgeAttributes.clear();
	WedgeAttributes.resize( NumWedges * NumAttributes );

	float Error = 0.0f;
#if SIMP_REBASE
	float EdgeError = EdgeQuadric.Evaluate( NewPosition - Position0 );
#else
	float EdgeError = EdgeQuadric.Evaluate( NewPosition );
#endif
	float SurfaceArea = 0.0f;

	for( int32 WedgeIndex = 0; WedgeIndex < NumWedges; WedgeIndex++ )
	{
		float* wrestrict NewAttributes = &WedgeAttributes[ WedgeIndex * NumAttributes ];

		FQuadricAttr& wrestrict WedgeQuadric = GetWedgeQuadric( WedgeIndex );
		if( WedgeQuadric.a > 1e-8 )
		{
			// calculate vert attributes from the new position
#if SIMP_REBASE
			float WedgeError = WedgeQuadric.CalcAttributesAndEvaluate( NewPosition - Position0, NewAttributes, AttributeWeights, NumAttributes );
#else
			float WedgeError = WedgeQuadric.CalcAttributesAndEvaluate( NewPosition, NewAttributes, AttributeWeights, NumAttributes );
#endif
			
			// Correct after eval. Normal length is unimportant for error but can bias the calculation.
			if( CorrectAttributes != nullptr )
				CorrectAttributes( NewAttributes );

			Error += WedgeError;
		}
		else
		{
			for( uint32 i = 0; i < NumAttributes; i++ )
			{
				NewAttributes[i] = 0.0f;
			}
		}
		
		SurfaceArea += WedgeQuadric.a;
	}

	Error += EdgeError;

	// Limit error to be no greater than the size of the triangles it could affect.
	Error = std::min( Error, SurfaceArea );

	if( bMoveVerts )
	{
		BeginMovePosition( Position0 );
		BeginMovePosition( Position1 );

		for( uint32 AdjTriIndex = 0, Num = AdjTris.size(); AdjTriIndex < Num; AdjTriIndex++ )
		{
			int32 WedgeIndex = white::find_index(WedgeIDs,WedgeDisjointSet[ AdjTriIndex ] );

			for( uint32 CornerIndex = 0; CornerIndex < 3; CornerIndex++ )
			{
				uint32 Corner = AdjTris[ AdjTriIndex ] * 3 + CornerIndex;
				uint32 VertIndex = Indexes[ Corner ];

				wm::float3& OldPosition = GetPosition( VertIndex );
				if( OldPosition == Position0 ||
					OldPosition == Position1 )
				{
					OldPosition = NewPosition;

					// Only use attributes if we calculated them.
					if( GetWedgeQuadric( WedgeIndex ).a > 1e-8 )
					{
						float* wrestrict NewAttributes = &WedgeAttributes[ WedgeIndex * NumAttributes ];
						float* wrestrict OldAttributes = GetAttributes( VertIndex );

						for( uint32 i = 0; i < NumAttributes; i++ )
						{
							OldAttributes[i] = NewAttributes[i];
						}
					}
					
					// If either position was locked then lock the new verts.
					if( bLocked0 || bLocked1 )
						CornerFlags[ Corner ] |= LockedVertMask;
				}
			}
		}

		for( uint32 PairIndex : MovedPairs )
		{
			FPair& Pair = Pairs[ PairIndex ];

			wconstraint( Pair.Position0 != Position0 || Pair.Position1 != Position1 );

			if( Pair.Position0 == Position0 ||
				Pair.Position0 == Position1 )
			{
				Pair.Position0 = NewPosition;
			}

			if( Pair.Position1 == Position0 ||
				Pair.Position1 == Position1 )
			{
				Pair.Position1 = NewPosition;
			}
		}

		EndMovePositions();

		std::vector< uint32> AdjVerts;
		for( uint32 TriIndex : AdjTris )
		{
			for( uint32 CornerIndex = 0; CornerIndex < 3; CornerIndex++ )
			{
				white::add_unique(AdjVerts,Indexes[ TriIndex * 3 + CornerIndex ] );
			}
		}

		// Reevaluate all pairs touching an adjacent tri.
		// Duplicate pairs have already been removed.
		for( uint32 VertIndex : AdjVerts )
		{
			const wm::float3& Position = GetPosition( VertIndex );

			ForAllPairs( Position,
				[ this ]( uint32 PairIndex )
				{
					// IsPresent used to mark Pairs we have already added to the list.
					if( PairHeap.IsPresent( PairIndex ) )
					{
						PairHeap.Remove( PairIndex );
						ReevaluatePairs.emplace_back( PairIndex );
					}
				} );
		}

		for( uint32 TriIndex : AdjTris )
		{
			FixUpTri( TriIndex );
		}
	}
	else
	{
		Error += Penalty;
	}

	for( uint32 TriIndex : AdjTris )
	{
		for( uint32 CornerIndex = 0; CornerIndex < 3; CornerIndex++ )
		{
			uint32 Corner = TriIndex * 3 + CornerIndex;

			if( bMoveVerts )
				CalcEdgeQuadric( Corner );

			// Clear flags
			CornerFlags[ Corner ] &= ~( MergeMask | TriMask );
		}
	}

	return Error;
}

void FMeshSimplifier::BeginMovePosition( const wm::float3& Position )
{
	uint32 Hash = HashPosition( Position );

	ForAllVerts( Position, 
		[ this, Hash ]( uint32 VertIndex )
		{
			// Safe to remove while iterating.
			VertHash.Remove( Hash, VertIndex );
			MovedVerts.emplace_back( VertIndex );
		} );

	ForAllCorners( Position, 
		[ this, Hash ]( uint32 Corner )
		{
			// Safe to remove while iterating.
			CornerHash.Remove( Hash, Corner );
			MovedCorners.emplace_back( Corner );
		} );

	ForAllPairs( Position,
		[ this ]( uint32 PairIndex )
		{
			// Safe to remove while iterating.
			PairHash0.Remove( HashPosition( Pairs[ PairIndex ].Position0 ), PairIndex );
			PairHash1.Remove( HashPosition( Pairs[ PairIndex ].Position1 ), PairIndex );
			MovedPairs.emplace_back( PairIndex );
		} );
}

void FMeshSimplifier::EndMovePositions()
{
	for( uint32 VertIndex : MovedVerts )
	{
		VertHash.Add( HashPosition( GetPosition( VertIndex ) ), VertIndex );
	}

	for( uint32 Corner : MovedCorners )
	{
		CornerHash.Add( HashPosition( GetPosition( Indexes[ Corner ] ) ), Corner );
	}

	for( uint32 PairIndex : MovedPairs )
	{
		FPair& Pair = Pairs[ PairIndex ];

		if( Pair.Position0 == Pair.Position1 || !AddUniquePair( Pair, PairIndex ) )
		{
			// Found invalid or duplicate pair
			PairHeap.Remove( PairIndex );
		}
	}

	MovedVerts.clear();
	MovedCorners.clear();
	MovedPairs.clear();
}

bool FMeshSimplifier::TriWillInvert( uint32 TriIndex, const wm::float3& NewPosition )
{
	uint32 IndexMoved = 3;
	for( uint32 CornerIndex = 0; CornerIndex < 3; CornerIndex++ )
	{
		uint32 Corner = TriIndex * 3 + CornerIndex;

		if( CornerFlags[ Corner ] & MergeMask )
		{
			if( IndexMoved == 3 )
				IndexMoved = CornerIndex;
			else
				IndexMoved = 4;
		}
	}

	if( IndexMoved < 3 )
	{
		uint32 Corner = TriIndex * 3 + IndexMoved;

		const wm::float3& p0 = GetPosition( Indexes[ Corner ] );
		const wm::float3& p1 = GetPosition( Indexes[ Cycle3( Corner ) ] );
		const wm::float3& p2 = GetPosition( Indexes[ Cycle3( Corner, 2 ) ] );

		const wm::float3 d21 = p2 - p1;
		const wm::float3 d01 = p0 - p1;
		const wm::float3 dp1 = NewPosition - p1;

	#if 1
		wm::float3 n0 = wm::cross(d01, d21);
		wm::float3 n1 = wm::cross(dp1, d21);

		return wm::dot(n0 , n1) < 0.0f;
	#else
		wm::float3 n = d21 ^ d01 ^ d21;
		//n.Normalize();

		float InversionThreshold = 0.0f;
		return (dp1 | n) < InversionThreshold * (d01 | n);
	#endif
	}

	return false;
}

void FMeshSimplifier::FixUpTri( uint32 TriIndex )
{
	wconstraint( !TriRemoved[ TriIndex ] );

	const wm::float3& p0 = GetPosition( Indexes[ TriIndex * 3 + 0 ] );
	const wm::float3& p1 = GetPosition( Indexes[ TriIndex * 3 + 1 ] );
	const wm::float3& p2 = GetPosition( Indexes[ TriIndex * 3 + 2 ] );
	
	// Remove degenerates
	bool bRemoveTri =
		p0 == p1 ||
		p1 == p2 ||
		p2 == p0;

	if( !bRemoveTri )
	{
		for( uint32 k = 0; k < 3; k++ )
		{
			RemoveDuplicateVerts( TriIndex * 3 + k );
		}

		bRemoveTri = IsDuplicateTri( TriIndex );
	}

	if( bRemoveTri )
	{
		TriRemoved[ TriIndex ] = true;
		RemainingNumTris--;

		// Remove references to tri
		for( uint32 k = 0; k < 3; k++ )
		{
			uint32 Corner = TriIndex * 3 + k;
			uint32 VertIndex = Indexes[ Corner ];
			uint32 Hash = HashPosition( GetPosition( VertIndex ) );

			CornerHash.Remove( Hash, Corner );
			EdgeQuadricsValid[ Corner ] = false;

			SetVertIndex( Corner, ~0u );
		}
	}
	else
	{
		CalcTriQuadric( TriIndex );
	}
}

bool FMeshSimplifier::IsDuplicateTri( uint32 TriIndex ) const
{
	uint32 i0 = Indexes[ TriIndex * 3 + 0 ];
	uint32 i1 = Indexes[ TriIndex * 3 + 1 ];
	uint32 i2 = Indexes[ TriIndex * 3 + 2 ];

	uint32 Hash = HashPosition( GetPosition( i0 ) );
	for( uint32 Corner = CornerHash.First( Hash ); CornerHash.IsValid( Corner ); Corner = CornerHash.Next( Corner ) )
	{
		if( Corner != TriIndex * 3 &&
			i0 == Indexes[ Corner ] &&
			i1 == Indexes[ Cycle3( Corner ) ] &&
			i2 == Indexes[ Cycle3( Corner, 2 ) ] )
		{
			return true;
		}
	}

	return false;
}

void FMeshSimplifier::SetVertIndex( uint32 Corner, uint32 NewVertIndex )
{
	uint32& VertIndex = Indexes[ Corner ];
	wconstraint( VertIndex != ~0u );
	wconstraint( VertRefCount[ VertIndex ] > 0 );

	if( VertIndex == NewVertIndex )
		return;

	uint32 RefCount = --VertRefCount[ VertIndex ];
	if( RefCount == 0 )
	{
		VertHash.Remove( HashPosition( GetPosition( VertIndex ) ), VertIndex );
		RemainingNumVerts--;
	}

	VertIndex = NewVertIndex;
	if( VertIndex != ~0u )
	{
		VertRefCount[ VertIndex ]++;
	}
}

// Remove identical valued verts.
void FMeshSimplifier::RemoveDuplicateVerts( uint32 Corner )
{
	uint32& VertIndex = Indexes[ Corner ];
	float* VertData = &Verts[ ( 3 + NumAttributes ) * VertIndex ];

	uint32 Hash = HashPosition( GetPosition( VertIndex ) );
	for( uint32 OtherVertIndex = VertHash.First( Hash ); VertHash.IsValid( OtherVertIndex ); OtherVertIndex = VertHash.Next( OtherVertIndex ) )
	{
		if( VertIndex == OtherVertIndex )
			break;

		float* OtherVertData = &Verts[ ( 3 + NumAttributes ) * OtherVertIndex ];
		if( std::memcmp( VertData, OtherVertData, ( 3 + NumAttributes ) * sizeof( float ) ) == 0 )
		{
			// First entry in hashtable for this vert value is authoritative.
			SetVertIndex( Corner, OtherVertIndex );
			break;
		}
	}
}

float FMeshSimplifier::Simplify( uint32 TargetNumVerts, uint32 TargetNumTris )
{
	wconstraint( TargetNumVerts < NumVerts || TargetNumTris < NumTris );

	const uint32 QuadricSize = sizeof( FQuadricAttr ) + NumAttributes * 4 * sizeof( QScalar );

	TriQuadrics.resize(TriQuadrics.size()+ NumTris * QuadricSize );
	for( uint32 TriIndex = 0; TriIndex < NumTris; TriIndex++ )
	{
		CalcTriQuadric( TriIndex );
	}

	for( uint32 i = 0; i < NumIndexes; i++ )
	{
		CalcEdgeQuadric(i);
	}

	// Initialize heap
	PairHeap.Resize( Pairs.size(), Pairs.size() );
	
	for( uint32 PairIndex = 0, Num = Pairs.size(); PairIndex < Num; PairIndex++ )
	{
		FPair& Pair = Pairs[ PairIndex ];

		float MergeError = EvaluateMerge( Pair.Position0, Pair.Position1, false );
		PairHeap.Add( MergeError, PairIndex );
	}

	float MaxError = 0.0f;

	while( PairHeap.Num() > 0 )
	{
		{
			uint32 PairIndex = PairHeap.Top();
			PairHeap.Pop();

			FPair& Pair = Pairs[ PairIndex ];
		
			PairHash0.Remove( HashPosition( Pair.Position0 ), PairIndex );
			PairHash1.Remove( HashPosition( Pair.Position1 ), PairIndex );

			float MergeError = EvaluateMerge( Pair.Position0, Pair.Position1, true );
			MaxError = std::max( MaxError, MergeError );
		}

		if( RemainingNumVerts <= TargetNumVerts && RemainingNumTris <= TargetNumTris )
			break;

		for( uint32 PairIndex : ReevaluatePairs )
		{
			FPair& Pair = Pairs[ PairIndex ];

			float MergeError = EvaluateMerge( Pair.Position0, Pair.Position1, false );
			PairHeap.Add( MergeError, PairIndex );
		}
		ReevaluatePairs.clear();
	}

	wconstraint( RemainingNumVerts <= TargetNumVerts && RemainingNumTris <= TargetNumTris );
	
	return MaxError;
}

void FMeshSimplifier::Compact()
{
	uint32 OutputVertIndex = 0;
	for( uint32 VertIndex = 0; VertIndex < NumVerts; VertIndex++ )
	{
		if( VertRefCount[ VertIndex ] > 0 )
		{
			if( VertIndex != OutputVertIndex )
			{
				float* SrcData = &Verts[ ( 3 + NumAttributes ) * VertIndex ];
				float* DstData = &Verts[ ( 3 + NumAttributes ) * OutputVertIndex ];
				std::memcpy( DstData, SrcData, ( 3 + NumAttributes ) * sizeof( float ) );
			}
			
			// Reuse VertRefCount as OutputVertIndex
			VertRefCount[ VertIndex ] = OutputVertIndex++;
		}
	}
	wconstraint( OutputVertIndex == RemainingNumVerts );
	
	uint32 OutputTriIndex = 0;
	for( uint32 TriIndex = 0; TriIndex < NumTris; TriIndex++ )
	{
		if( !TriRemoved[ TriIndex ] )
		{
			for( uint32 k = 0; k < 3; k++ )
			{
				// Reuse VertRefCount as OutputVertIndex
				uint32 VertIndex = Indexes[ TriIndex * 3 + k ];
				uint32 OutVertIndex = VertRefCount[ VertIndex ];
				Indexes[ OutputTriIndex * 3 + k ] = OutVertIndex;
			}

			MaterialIndexes[ OutputTriIndex++ ] = MaterialIndexes[ TriIndex ];
		}
	}
	wconstraint( OutputTriIndex == RemainingNumTris );
}