// Copyright (C) 2009 Nine Realms, Inc
//

#pragma once

#include "CoreTypes.h"

#define SIMP_NEW_MERGE	1
#define SIMP_REBASE		1

#include "MeshSimplifyElements.h"
#include "Quadric.h"
#include "Core/Hash/Murmur.h"
#include "Core/Container/HashTable.h"
#include "Core/Container/BitArray.h"
#include <unordered_set>
#include "Core/Container/BinaryHeap.h"
#include "Core/Container/DisjointSet.h"
#include "Core/Math/BoxSphereBounds.h"


inline uint32 HashPosition( const wm::float3& Position )
{
	union { float f; uint32 i; } x;
	union { float f; uint32 i; } y;
	union { float f; uint32 i; } z;

	x.f = Position.x;
	y.f = Position.y;
	z.f = Position.z;

	return Murmur32( {
		Position.x == 0.0f ? 0u : x.i,
		Position.y == 0.0f ? 0u : y.i,
		Position.z == 0.0f ? 0u : z.i
	} );
}

inline uint32 Cycle3( uint32 Value )
{
	uint32 ValueMod3 = Value % 3;
	uint32 Value1Mod3 = ( 1 << ValueMod3 ) & 3;
	return Value - ValueMod3 + Value1Mod3;
}

inline uint32 Cycle3( uint32 Value, uint32 Offset )
{
	return Value - Value % 3 + ( Value + Offset ) % 3;
}


class FMeshSimplifier
{
public:
	FMeshSimplifier( float* Verts, uint32 NumVerts, uint32* Indexes, uint32 NumIndexes, int32* MaterialIndexes, uint32 NumAttributes );
									~FMeshSimplifier() = default;

	void		SetAttributeWeights( const float* Weights )			{ AttributeWeights = Weights; }
	void		SetEdgeWeight( float Weight )						{ EdgeWeight = Weight; }
	void		SetCorrectAttributes( void (*Function)( float* ) )	{ CorrectAttributes = Function; }

	void	SetBoundaryLocked( const white::TBitArray<>& UnlockedBoundaryEdges );
	void	GetBoundaryUnlocked(white::TBitArray<>& UnlockedBoundaryEdges );

	float	Simplify( uint32 TargetNumVerts, uint32 TargetNumTris );
	void	Compact();

	uint32		GetRemainingNumVerts() const	{ return RemainingNumVerts; }
	uint32		GetRemainingNumTris() const		{ return RemainingNumTris; }

protected:
	const int32 DegreeLimit			= 24;
	const float DegreePenalty		= 0.5f;
	const float LockPenalty			= 1e8f;
	const float InversionPenalty	= 100.0f;

	uint32		NumVerts;
	uint32		NumIndexes;
	uint32		NumAttributes;
	uint32		NumTris;

	uint32		RemainingNumVerts;
	uint32		RemainingNumTris;
	
	float*			Verts;
	uint32*			Indexes;
	int32*			MaterialIndexes;

	const float*	AttributeWeights = nullptr;
	float			EdgeWeight = 8.0f;
	void			(*CorrectAttributes)( float* ) = nullptr;

	white::FHashTable		VertHash;
	white::FHashTable		CornerHash;

	std::vector< uint32 >	VertRefCount;
	std::vector< uint8 >		CornerFlags;
	white::TBitArray<>		TriRemoved;

	struct FPair
	{
		wm::float3	Position0;
		wm::float3	Position1;
	};
	std::vector< FPair >			Pairs;
	white::FHashTable				PairHash0;
	white::FHashTable				PairHash1;
	white::FBinaryHeap< float >	PairHeap;

	std::vector< uint32 >	MovedVerts;
	std::vector< uint32 >	MovedCorners;
	std::vector< uint32 >	MovedPairs;
	std::vector< uint32 >	ReevaluatePairs;

	using EdgeType = std::tuple< wm::float3, wm::float3 >;
	std::unordered_set<EdgeType,white::combined_hash<EdgeType>>	LockedEdges;

	std::vector< uint8 >			TriQuadrics;
	std::vector< FEdgeQuadric >	EdgeQuadrics;
	white::TBitArray<>				EdgeQuadricsValid;

	std::vector< float > WedgeAttributes;
	white::FDisjointSet	WedgeDisjointSet;

	enum ECornerFlags
	{
		MergeMask		= 3,		// Merge position 0 or 1
		TriMask			= (1 << 2),	// Has been added to AdjTris
		LockedVertMask	= (1 << 3),	// Vert is locked, disallowing position movement
		LockedEdgeMask	= (1 << 4),	// Edge is locked, disallowing position movement
	};

protected:
	wm::float3&		GetPosition( uint32 VertIndex );
	const wm::float3&	GetPosition( uint32 VertIndex ) const;
	float*			GetAttributes( uint32 VertIndex );
	wm::float3			GetNormal( uint32 TriIndex ) const;
	FQuadricAttr&	GetTriQuadric( uint32 TriIndex );

	template< typename FuncType >
	void	ForAllVerts( const wm::float3& Position, FuncType&& Function ) const;

	template< typename FuncType >
	void	ForAllCorners( const wm::float3& Position, FuncType&& Function ) const;

	template< typename FuncType >
	void	ForAllPairs( const wm::float3& Position, FuncType&& Function ) const;

	void	GatherAdjTris( const wm::float3& Position, uint32 Flag, std::vector< uint32>& AdjTris, int32& VertDegree, uint32& FlagUnion );
	bool	AddUniquePair( FPair& Pair, uint32 PairIndex );

	void	CalcTriQuadric( uint32 TriIndex );
	void	CalcEdgeQuadric( uint32 EdgeIndex );

	bool	IsBoundaryEdge( uint32 EdgeIndex ) const;

	float	EvaluateMerge( const wm::float3& Position0, const wm::float3& Position1, bool bMoveVerts );
	
	void	BeginMovePosition( const wm::float3& Position );
	void	EndMovePositions();

	bool	TriWillInvert( uint32 TriIndex, const wm::float3& NewPosition );

	void	FixUpTri( uint32 TriIndex );
	bool	IsDuplicateTri( uint32 TriIndex ) const;
	void	SetVertIndex( uint32 Corner, uint32 NewVertIndex );
	void	RemoveDuplicateVerts( uint32 Corner );
};


inline wm::float3& FMeshSimplifier::GetPosition( uint32 VertIndex )
{
	return *reinterpret_cast< wm::float3* >( &Verts[ ( 3 + NumAttributes ) * VertIndex ] );
}

inline const wm::float3& FMeshSimplifier::GetPosition( uint32 VertIndex ) const
{
	return *reinterpret_cast< const wm::float3* >( &Verts[ ( 3 + NumAttributes ) * VertIndex ] );
}

inline float* FMeshSimplifier::GetAttributes( uint32 VertIndex )
{
	return &Verts[ ( 3 + NumAttributes ) * VertIndex + 3 ];
}

inline wm::float3 FMeshSimplifier::GetNormal( uint32 TriIndex ) const
{
	const wm::float3& p0 = GetPosition( Indexes[ TriIndex * 3 + 0 ] );
	const wm::float3& p1 = GetPosition( Indexes[ TriIndex * 3 + 1 ] );
	const wm::float3& p2 = GetPosition( Indexes[ TriIndex * 3 + 2 ] );

	wm::float3 Normal =wm::cross(( p2 - p0 ),( p1 - p0 ));
	Normal = wm::normalize(Normal);
	return Normal;
}

inline FQuadricAttr& FMeshSimplifier::GetTriQuadric( uint32 TriIndex )
{
	const uint32 QuadricSize = sizeof( FQuadricAttr ) + NumAttributes * 4 * sizeof( QScalar );
	return *reinterpret_cast< FQuadricAttr* >( &TriQuadrics[ TriIndex * QuadricSize ] );
}

template< typename FuncType >
void FMeshSimplifier::ForAllVerts( const wm::float3& Position, FuncType&& Function ) const
{
	uint32 Hash = HashPosition( Position );
	for( uint32 VertIndex = VertHash.First( Hash ); VertHash.IsValid( VertIndex ); VertIndex = VertHash.Next( VertIndex ) )
	{
		if( GetPosition( VertIndex ) == Position )
		{
			Function( VertIndex );
		}
	}
}

template< typename FuncType >
void FMeshSimplifier::ForAllCorners( const wm::float3& Position, FuncType&& Function ) const
{
	uint32 Hash = HashPosition( Position );
	for( uint32 Corner = CornerHash.First( Hash ); CornerHash.IsValid( Corner ); Corner = CornerHash.Next( Corner ) )
	{
		if( GetPosition( Indexes[ Corner ] ) == Position )
		{
			Function( Corner );
		}
	}
}

template< typename FuncType >
void FMeshSimplifier::ForAllPairs( const wm::float3& Position, FuncType&& Function ) const
{
	uint32 Hash = HashPosition( Position );
	for( uint32 PairIndex = PairHash0.First( Hash ); PairHash0.IsValid( PairIndex ); PairIndex = PairHash0.Next( PairIndex ) )
	{
		if( Pairs[ PairIndex ].Position0 == Position )
		{
			Function( PairIndex );
		}
	}

	for( uint32 PairIndex = PairHash1.First( Hash ); PairHash1.IsValid( PairIndex ); PairIndex = PairHash1.Next( PairIndex ) )
	{
		if( Pairs[ PairIndex ].Position1 == Position )
		{
			Function( PairIndex );
		}
	}
}



template< typename T, uint32 NumAttributes >
class TMeshSimplifier
{
	typedef typename TSimpVert<T>::TriIterator	TriIterator;
	typedef TQuadricAttr< NumAttributes >		QuadricType;

public:
						TMeshSimplifier( const T* Verts, uint32 NumVerts, const uint32* Indexes, uint32 NumIndexes );
						~TMeshSimplifier();

	void				SetAttributeWeights( const float* weights );
	void				SetEdgeWeight( float Weight );
	void				SetBoundaryLocked();

	void				InitCosts();

	float				SimplifyMesh( float maxErrorLimit, int minTris, int minVerts = 0);

	int					GetNumVerts() const { return numVerts; }
	int					GetNumTris() const { return numTris; }

	void				OutputMesh( T* Verts, uint32* Indexes, uint32* OutNumVertices = nullptr, uint32* OutNumIndices = nullptr ) const;

	template< typename FaceType >
	void				CompactFaceData( std::vector< FaceType>& FaceData );

protected:
	void				LockVertFlags( uint32 flag );
	void				UnlockVertFlags( uint32 flag );

	void				LockTriFlags( uint32 flag );
	void				UnlockTriFlags( uint32 flag );

	void				GatherUpdates( TSimpVert<T>* v );

	void				GroupVerts();
	void				GroupEdges();

	void				InitVert( TSimpVert<T>* v );

	QuadricType&		GetQuadric( const TSimpTri<T>* Tri );
	QuadricType&		GetQuadric( TSimpVert<T>* v );
	FQuadric&			GetEdgeQuadric( TSimpVert<T>* v );

	// TODO move away from pointers and remove these functions
	uint32				GetVertIndex( const TSimpVert<T>* vert ) const;
	uint32				GetTriIndex( const TSimpTri<T>* tri ) const;
	uint32				GetEdgeIndex( const TSimpEdge<T>* edge ) const;

	uint32				HashPoint( const wm::float3& p ) const;
	uint32				HashEdge( const TSimpVert<T>* u, const TSimpVert<T>* v ) const;
	TSimpEdge<T>*		FindEdge( const TSimpVert<T>* u, const TSimpVert<T>* v );
	
	void				RemoveEdge( TSimpEdge<T>* edge );
	void				RemoveVert( TSimpVert<T>* vert );
	void				ReplaceEdgeVert( const TSimpVert<T>* oldV, const TSimpVert<T>* otherV, TSimpVert<T>* newV );
	void				CollapseEdgeVert( const TSimpVert<T>* oldV, const TSimpVert<T>* otherV, TSimpVert<T>* newV );

	float				Merge( TSimpVert<T>* v0, TSimpVert<T>* v1, bool bMoveVerts );
	float				ComputeMergePenalty( TSimpVert<T>* v0, TSimpVert<T>* v1, const wm::float3& NewPos );
#if !SIMP_NEW_MERGE
	float				ComputeNewVerts( TSimpEdge<T>* edge, std::vector< T, TInlineAllocator<16> >& newVerts );
	float				ComputeEdgeCollapseCost( TSimpEdge<T>* edge, bool bMoveVerts );
#endif
	void				AddEdgeToHeap( TSimpEdge<T>* edge );
	void				Collapse( TSimpEdge<T>* edge );

	void				UpdateTris();
	void				UpdateVerts();
	void				UpdateEdges();

	uint32				vertFlagLock;
	uint32				triFlagLock;
	
	float				attributeWeights[ NumAttributes ];
	float				EdgeWeight = 16.0f;
	
	TSimpVert<T>*		sVerts;
	TSimpTri<T>*		sTris;

	int					numSVerts;
	int					numSTris;

	int					numVerts;
	int					numTris;

	std::vector< TSimpEdge<T> >	edges;
	white::FHashTable				edgeHash;
	white::FBinaryHeap<float>		edgeHeap;

	std::vector< TSimpVert<T>* >	updateVerts;
	std::vector< TSimpTri<T>* >	updateTris;
	std::vector< TSimpEdge<T>* >	updateEdges;

	white::TBitArray<>				VertQuadricsValid;
	std::vector< QuadricType >	VertQuadrics;

	white::TBitArray<>				TriQuadricsValid;
	std::vector< QuadricType >	TriQuadrics;

	white::TBitArray<>				EdgeQuadricsValid;
	std::vector< FQuadric >		EdgeQuadrics;

	white::FHashTable			AdjCorners;
	white::FDisjointSet		WedgeSet;
};


//=============
// TMeshSimplifier
//=============
template< typename T, uint32 NumAttributes >
TMeshSimplifier<T, NumAttributes>::TMeshSimplifier( const T* Verts, uint32 NumVerts, const uint32* Indexes, uint32 NumIndexes )
	: edgeHash( 1 << std::min( 16u, wm::FloorLog2( NumVerts ) ) )
	, AdjCorners( 64 )
{
	vertFlagLock = 0;
	triFlagLock = 0;

	for( uint32 i = 0; i < NumAttributes; i++ )
	{
		attributeWeights[i] = 1.0f;
	}

	numSVerts = NumVerts;
	numSTris = NumIndexes / 3;

	this->numVerts = numSVerts;
	this->numTris = numSTris;

	sVerts = new TSimpVert<T>[ numSVerts ];
	sTris = new TSimpTri<T>[ numSTris ];

	VertQuadricsValid.resize(numSVerts,false);
	VertQuadrics.resize( numSVerts );

	TriQuadricsValid.resize(numSTris,false);
	TriQuadrics.resize( numSTris );

	EdgeQuadricsValid.resize(numSVerts,false);
	EdgeQuadrics.resize( numSVerts );

	for( int i = 0; i < numSVerts; i++ )
	{
		sVerts[i].vert = Verts[i];
	}
	
	for( int i = 0; i < numSTris; i++ )
	{
		for( int j = 0; j < 3; j++ )
		{
			sTris[i].verts[j] = &sVerts[ Indexes[3*i+j] ];
			sTris[i].verts[j]->adjTris.Add( &sTris[i] );
		}
	}

	GroupVerts();

	int maxEdgeSize = std::min( 3 * numSTris, 3 * numSVerts - 6 );
	edges.resize( maxEdgeSize );
	for( int i = 0; i < numSVerts; i++ )
	{
		InitVert( &sVerts[i] );
	}

	// Guessed wrong on num edges. Array was resized so fix up pointers.
	if( edges.size() > maxEdgeSize )
	{
		for( int i = 0; i < edges.size(); i++ )
		{
			TSimpEdge<T>& edge = edges[i];
			edge.next = &edge;
			edge.prev = &edge;
		}
	}

	GroupEdges();

	edgeHash.Resize( edges.size() );
	for( int i = 0; i < edges.Num(); i++ )
	{
		edgeHash.Add( HashEdge( edges[i].v0, edges[i].v1 ), i );
	}

	edgeHeap.Resize( edges.size(), edges.size() );
}

template< typename T, uint32 NumAttributes >
TMeshSimplifier<T, NumAttributes>::~TMeshSimplifier()
{
	delete[] sVerts;
	delete[] sTris;
}

template< typename T, uint32 NumAttributes >
void TMeshSimplifier<T, NumAttributes>::SetAttributeWeights( const float* weights )
{
	for( uint32 i = 0; i < NumAttributes; i++ )
	{
		attributeWeights[i] = weights[i];
	}
}

template< typename T, uint32 NumAttributes >
void TMeshSimplifier<T, NumAttributes>::SetEdgeWeight( float Weight )
{
	EdgeWeight = Weight;
}

template< typename T, uint32 NumAttributes >
void TMeshSimplifier<T, NumAttributes>::SetBoundaryLocked()
{
	for( auto& edge : edges )
	{
		LockTriFlags( SIMP_MARK1 );

		// set if this edge is boundary
		// find faces that share v0 and v1
		edge.v0->EnableAdjTriFlagsGroup( SIMP_MARK1 );

		int faceCount = 0;
		TSimpVert<T>* vert = edge.v1;
		do
		{
			for( TriIterator j = vert->adjTris.Begin(); j != vert->adjTris.End(); ++j )
			{
				TSimpTri<T>* tri = *j;
				faceCount += tri->TestFlags( SIMP_MARK1 ) ? 1 : 0;
			}
			vert = vert->next;
		} while( vert != edge.v1 );

		edge.v0->DisableAdjTriFlagsGroup( SIMP_MARK1 );

		if( faceCount == 1 )
		{
			// only one face on this edge
			edge.v0->EnableFlagsGroup( SIMP_LOCKED );
			edge.v1->EnableFlagsGroup( SIMP_LOCKED );
		}

		UnlockTriFlags( SIMP_MARK1 );
	}
}

// locking functions for nesting safety
template< typename T, uint32 NumAttributes >
inline void TMeshSimplifier<T, NumAttributes>::LockVertFlags( uint32 f )
{
	wconstraint( ( vertFlagLock & f ) == 0 );
	vertFlagLock |= f;
}

template< typename T, uint32 NumAttributes >
inline void TMeshSimplifier<T, NumAttributes>::UnlockVertFlags( uint32 f )
{
	vertFlagLock &= ~f;
}

template< typename T, uint32 NumAttributes >
inline void TMeshSimplifier<T, NumAttributes>::LockTriFlags( uint32 f )
{
	wconstraint( ( triFlagLock & f ) == 0 );
	triFlagLock |= f;
}

template< typename T, uint32 NumAttributes >
inline void TMeshSimplifier<T, NumAttributes>::UnlockTriFlags( uint32 f )
{
	triFlagLock &= ~f;
}

template< typename T, uint32 NumAttributes >
void TMeshSimplifier<T, NumAttributes>::InitVert( TSimpVert<T>* v )
{
	wconstraint( v->adjTris.Num() > 0 );

	std::vector<TSimpVert<T>*> adjVerts;
	v->FindAdjacentVerts( adjVerts );

	TSimpVert<T>* v0 = v;
	for( TSimpVert<T>* v1 : adjVerts )
	{
		if( v0 < v1 )
		{
			// add edge
			edges.emplace_back();
			TSimpEdge<T>& edge = edges.back();
			edge.v0 = v0;
			edge.v1 = v1;
		}
	}
}

template< typename T, uint32 NumAttributes >
void TMeshSimplifier<T, NumAttributes>::GroupVerts()
{
	// group verts that share a point
	white::FHashTable HashTable(1 << std::min(16u, wm::FloorLog2(numSVerts / 2)), numSVerts);

	for( int i = 0; i < numSVerts; i++ )
	{
		HashTable.Add( HashPoint( sVerts[i].GetPos() ), i );
	}
	
	for( int i = 0; i < numSVerts; i++ )
	{
		// already grouped
		if( sVerts[i].next != &sVerts[i] )
		{
			continue;
		}

		// find any matching verts
		uint32 hash = HashPoint( sVerts[i].GetPos() );
		for( int j = HashTable.First( hash ); HashTable.IsValid(j); j = HashTable.Next(j) )
		{
			TSimpVert<T>* v1 = &sVerts[i];
			TSimpVert<T>* v2 = &sVerts[j];

			if( v1 == v2 )
				continue;

			// link
			if( v1->GetPos() == v2->GetPos() )
			{
				wconstraint( v2->next == v2 );
				wconstraint( v2->prev == v2 );

				v2->next = v1->next;
				v2->prev = v1;
				v2->next->prev = v2;
				v2->prev->next = v2;
			}
		}
	}
}

template< typename T, uint32 NumAttributes >
void TMeshSimplifier<T, NumAttributes>::GroupEdges()
{
	white::FHashTable HashTable( 1 << std::min( 16u, wm::FloorLog2( edges.Num() / 2 ) ), edges.Num() );
	for( int i = 0; i < edges.size(); i++ )
	{
		uint32 Hash0 = HashPoint( edges[i].v0->GetPos() );
		uint32 Hash1 = HashPoint( edges[i].v1->GetPos() );
		uint32 Hash = Murmur32( { std::min( Hash0, Hash1 ), std::max( Hash0, Hash1 ) } );
		HashTable.Add( Hash, i );
	}

	for( int i = 0; i < edges.size(); i++ )
	{
		// already grouped
		if( edges[i].next != &edges[i] )
		{
			continue;
		}

		// find any matching edges
		uint32 Hash0 = HashPoint( edges[i].v0->GetPos() );
		uint32 Hash1 = HashPoint( edges[i].v1->GetPos() );
		uint32 Hash = Murmur32( { std::min( Hash0, Hash1 ), std::max( Hash0, Hash1 ) } );
		for( uint32 j = HashTable.First( Hash ); HashTable.IsValid(j); j = HashTable.Next(j) )
		{
			TSimpEdge<T>* e1 = &edges[i];
			TSimpEdge<T>* e2 = &edges[j];

			if( e1 == e2 )
				continue;

			bool m1 =	e1->v0->GetPos() == e2->v0->GetPos() &&
						e1->v1->GetPos() == e2->v1->GetPos();

			bool m2 =	e1->v0->GetPos() == e2->v1->GetPos() &&
						e1->v1->GetPos() == e2->v0->GetPos();

			// backwards
			if( m2 )
			{
				std::swap( e2->v0, e2->v1 );
			}

			// link
			if( m1 || m2 )
			{
				wconstraint( e2->next == e2 );
				wconstraint( e2->prev == e2 );

				e2->next = e1->next;
				e2->prev = e1;
				e2->next->prev = e2;
				e2->prev->next = e2;
			}
		}
	}
}

template< typename T, uint32 NumAttributes >
void TMeshSimplifier<T, NumAttributes>::InitCosts()
{
	for( auto& edge : edges )
	{
		AddEdgeToHeap( &edge );
	}
}

template< typename T, uint32 NumAttributes >
TQuadricAttr< NumAttributes >& TMeshSimplifier<T, NumAttributes>::GetQuadric( const TSimpTri<T>* Tri )
{
	uint32 TriIndex = GetTriIndex( Tri );
	if( !TriQuadricsValid[ TriIndex ] )
	{
		TriQuadricsValid[ TriIndex ] = true;
		TriQuadrics[ TriIndex ] = QuadricType(
			Tri->verts[0]->GetPos(),		Tri->verts[1]->GetPos(),		Tri->verts[2]->GetPos(),
			Tri->verts[0]->GetAttributes(),	Tri->verts[1]->GetAttributes(),	Tri->verts[2]->GetAttributes(),
			attributeWeights );
	}

	return TriQuadrics[ TriIndex ];
}

template< typename T, uint32 NumAttributes >
TQuadricAttr< NumAttributes >& TMeshSimplifier<T, NumAttributes>::GetQuadric( TSimpVert<T>* v )
{
	uint32 VertIndex = GetVertIndex( v );
	if( !VertQuadricsValid[ VertIndex ] )
	{
		QuadricType vertQuadric;
		vertQuadric.Zero();
	
		// sum tri quadrics
		for( TriIterator i = v->adjTris.Begin(); i != v->adjTris.End(); ++i )
		{
			vertQuadric += GetQuadric( *i );
		}

		VertQuadricsValid[ VertIndex ] = true;
		VertQuadrics[ VertIndex ] = vertQuadric;
	}

	return VertQuadrics[ VertIndex ];
}

template< typename T, uint32 NumAttributes >
FQuadric& TMeshSimplifier<T, NumAttributes>::GetEdgeQuadric( TSimpVert<T>* v )
{
	uint32 VertIndex = GetVertIndex( v );
	if( !EdgeQuadricsValid[ VertIndex ] )
	{
		FQuadric vertQuadric;
		vertQuadric.Zero();

		std::vector< TSimpVert<T>*> adjVerts;
		v->FindAdjacentVerts( adjVerts );

		LockTriFlags( SIMP_MARK1 );
		v->EnableAdjTriFlags( SIMP_MARK1 );
	
		for( TSimpVert<T>* vert : adjVerts )
		{
			TSimpTri<T>* face = NULL;
			int faceCount = 0;
			for( TriIterator j = vert->adjTris.Begin(); j != vert->adjTris.End(); ++j )
			{
				TSimpTri<T>* tri = *j;
				if( tri->TestFlags( SIMP_MARK1 ) )
				{
					face = tri;
					faceCount++;
				}
			}

			if( faceCount == 1 )
			{
				// only one face on this edge
				FQuadric edgeQuadric( v->GetPos(), vert->GetPos(), face->GetNormal(), EdgeWeight );
				vertQuadric += edgeQuadric;
			}
		}

		v->DisableAdjTriFlags( SIMP_MARK1 );
		UnlockTriFlags( SIMP_MARK1 );

		EdgeQuadricsValid[ VertIndex ] = true;
		EdgeQuadrics[ VertIndex ] = vertQuadric;
	}

	return EdgeQuadrics[ VertIndex ];
}

template< typename T, uint32 NumAttributes >
inline uint32 TMeshSimplifier<T, NumAttributes>::GetVertIndex( const TSimpVert<T>* vert ) const
{
	ptrdiff_t Index = vert - &sVerts[0];
	return (uint32)Index;
}

template< typename T, uint32 NumAttributes >
inline uint32 TMeshSimplifier<T, NumAttributes>::GetTriIndex( const TSimpTri<T>* tri ) const
{
	ptrdiff_t Index = tri - &sTris[0];
	return (uint32)Index;
}

template< typename T, uint32 NumAttributes >
inline uint32 TMeshSimplifier<T, NumAttributes>::GetEdgeIndex( const TSimpEdge<T>* edge ) const
{
	ptrdiff_t Index = edge - &edges[0];
	return (uint32)Index;
}

template< typename T, uint32 NumAttributes >
inline uint32 TMeshSimplifier<T, NumAttributes>::HashPoint( const wm::float3& p ) const
{
	union { float f; uint32 i; } x;
	union { float f; uint32 i; } y;
	union { float f; uint32 i; } z;

	x.f = p.x;
	y.f = p.y;
	z.f = p.z;

	return Murmur32( { x.i, y.i, z.i } );
}

template< typename T, uint32 NumAttributes >
inline uint32 TMeshSimplifier<T, NumAttributes>::HashEdge( const TSimpVert<T>* u, const TSimpVert<T>* v ) const
{
	uint32 ui = GetVertIndex( u );
	uint32 vi = GetVertIndex( v );
	// must be symmetrical
	return Murmur32( { std::min( ui, vi ), std::max( ui, vi ) } );
}

template< typename T, uint32 NumAttributes >
TSimpEdge<T>* TMeshSimplifier<T, NumAttributes>::FindEdge( const TSimpVert<T>* u, const TSimpVert<T>* v )
{
	uint32 hash = HashEdge( u, v );
	for( uint32 i = edgeHash.First( hash ); edgeHash.IsValid(i); i = edgeHash.Next(i) )
	{
		if( ( edges[i].v0 == u && edges[i].v1 == v ) ||
			( edges[i].v0 == v && edges[i].v1 == u ) )
		{
			return &edges[i];
		}
	}

	return NULL;
}

template< typename T, uint32 NumAttributes >
void TMeshSimplifier<T, NumAttributes>::RemoveEdge( TSimpEdge<T>* edge )
{
	if( edge->TestFlags( SIMP_REMOVED ) )
	{
		wconstraint( edge->next == edge );
		wconstraint( edge->prev == edge );
		return;
	}

	uint32 hash = HashEdge( edge->v0, edge->v1 );
	for( uint32 i = edgeHash.First( hash ); edgeHash.IsValid(i); i = edgeHash.Next(i) )
	{
		if( &edges[i] == edge )
		{
			edgeHash.Remove( hash, i );
			edgeHeap.Remove( i );
			break;
		}
	}

	// remove edge
	edge->EnableFlags( SIMP_REMOVED );

	// ungroup edge
	edge->prev->next = edge->next;
	edge->next->prev = edge->prev;
	edge->next = edge;
	edge->prev = edge;
}

template< typename T, uint32 NumAttributes >
void TMeshSimplifier<T, NumAttributes>::RemoveVert( TSimpVert<T>* vert )
{
	vert->adjTris.Clear();
	vert->EnableFlags( SIMP_REMOVED );

	// ungroup
	vert->prev->next = vert->next;
	vert->next->prev = vert->prev;
	vert->next = vert;
	vert->prev = vert;
	
	numVerts--;
}

template< typename T, uint32 NumAttributes >
void TMeshSimplifier<T, NumAttributes>::ReplaceEdgeVert( const TSimpVert<T>* oldV, const TSimpVert<T>* otherV, TSimpVert<T>* newV )
{
	uint32 hash = HashEdge( oldV, otherV );
	uint32 index;
	for( index = edgeHash.First( hash ); edgeHash.IsValid( index ); index = edgeHash.Next( index ) )
	{
		if( ( edges[ index ].v0 == oldV && edges[ index ].v1 == otherV ) ||
			( edges[ index ].v1 == oldV && edges[ index ].v0 == otherV ) )
			break;
	}

	wconstraint( index != -1 );
	TSimpEdge<T>* edge = &edges[ index ];
	
	edgeHash.Remove( hash, index );

	TSimpEdge<T>* ExistingEdge = FindEdge( newV, otherV );
	if( ExistingEdge )
	{
		// Not entirely sure why this happens. I believe these are invalid edges from bridge tris.
		RemoveEdge( ExistingEdge );
	}

	if( newV )
	{
		edgeHash.Add( HashEdge( newV, otherV ), index );

		if( edge->v0 == oldV )
			edge->v0 = newV;
		else
			edge->v1 = newV;
	}
	else
	{
		// remove edge
		edge->EnableFlags( SIMP_REMOVED );

		// ungroup old edge
		edge->prev->next = edge->next;
		edge->next->prev = edge->prev;
		edge->next = edge;
		edge->prev = edge;

		edgeHeap.Remove( index );
	}
}

template< typename T, uint32 NumAttributes >
void TMeshSimplifier<T, NumAttributes>::CollapseEdgeVert( const TSimpVert<T>* oldV, const TSimpVert<T>* otherV, TSimpVert<T>* newV )
{
	uint32 hash = HashEdge( oldV, otherV );
	uint32 index;
	for( index = edgeHash.First( hash ); edgeHash.IsValid( index ); index = edgeHash.Next( index ) )
	{
		if( ( edges[ index ].v0 == oldV && edges[ index ].v1 == otherV ) ||
			( edges[ index ].v1 == oldV && edges[ index ].v0 == otherV ) )
			break;
	}

	wconstraint( index != -1 );
	TSimpEdge<T>* edge = &edges[ index ];
	
	edgeHash.Remove( hash, index );
	edgeHeap.Remove( index );

	// remove edge
	edge->EnableFlags( SIMP_REMOVED );

	// ungroup old edge
	edge->prev->next = edge->next;
	edge->next->prev = edge->prev;
	edge->next = edge;
	edge->prev = edge;
}

template< typename T, uint32 NumAttributes >
void TMeshSimplifier<T, NumAttributes>::GatherUpdates( TSimpVert<T>* v )
{
	std::vector< TSimpVert<T>*> AdjVerts;
	
	for( TriIterator i = v->adjTris.Begin(); i != v->adjTris.End(); ++i )
	{
		TSimpTri<T>* Tri = *i;

		// Update all tris touching collapse edge.
		white::add_unique(updateTris, Tri );

		for( uint32 CornerIndex = 0; CornerIndex < 3; CornerIndex++ )
		{
			TSimpVert<T>* Vert = Tri->verts[ CornerIndex ];

			if(white::find_index(AdjVerts, Vert ) == white::INDEX_NONE )
			{
				AdjVerts.emplace_back( Vert );

				// Update verts from tris adjacent to collapsed edge
				white::add_unique(updateVerts,Vert );
			}
		}
	}

	LockVertFlags( SIMP_MARK1 | SIMP_MARK2 );

	// update the costs of all edges connected to any face adjacent to v
	for( TSimpVert<T>* AdjVert : AdjVerts )
	{
		// Mark as needing to be tested against this AdjVert
		AdjVert->EnableAdjVertFlags( SIMP_MARK1 );

		// Mark as processed as an AdjVert. No need to try edges connecting to it after we've evaluated all its possible connections already.
		AdjVert->EnableFlags( SIMP_MARK2 );

		for( TriIterator i = AdjVert->adjTris.Begin(); i != AdjVert->adjTris.End(); ++i )
		{
			TSimpTri<T>* Tri = *i;

			for( uint32 CornerIndex = 0; CornerIndex < 3; CornerIndex++ )
			{
				TSimpVert<T>* Vert = Tri->verts[ CornerIndex ];

				// Any vert that hasn't been tested against AdjVert yet nor has been an AdjVert itself already.
				if( Vert->TestFlags( SIMP_MARK1 ) && !Vert->TestFlags( SIMP_MARK2 ) )
				{
					TSimpEdge<T>* edge = FindEdge( AdjVert, Vert );
					updateEdges.AddUnique( edge );
				}
				Vert->DisableFlags( SIMP_MARK1 );
			}
		}
	}

	for( TSimpVert<T>* AdjVert : AdjVerts )
	{
		AdjVert->DisableFlags( SIMP_MARK2 );
	}

	UnlockVertFlags( SIMP_MARK1 | SIMP_MARK2 );
}

template< typename T, uint32 NumAttributes >
float TMeshSimplifier<T, NumAttributes>::Merge( TSimpVert<T>* v0, TSimpVert<T>* v1, bool bMoveVerts )
{
	const wm::float3 Position0 = v0->GetPos();
	const wm::float3 Position1 = v1->GetPos();

	// Find unique adjacent triangles
	std::vector< TSimpTri<T>*> AdjTris;
	
	TSimpVert<T>* v;
	v = v0;
	do {
		wconstraint( !v->TestFlags( SIMP_REMOVED ) );
		wconstraint( v->adjTris.Num() != 0 );

		for( TriIterator i = v->adjTris.Begin(); i != v->adjTris.End(); ++i )
		{
			TSimpTri<T>* Tri = *i;
			wconstraint( !Tri->TestFlags( SIMP_REMOVED ) );

			if( !Tri->TestFlags( SIMP_MARK1 ) )
			{
				Tri->EnableFlags( SIMP_MARK1 );
				AdjTris.Add( Tri );
			}
		}
		v = v->next;
	} while( v != v0 );

	v = v1;
	do {
		wconstraint( !v->TestFlags( SIMP_REMOVED ) );
		wconstraint( v->adjTris.Num() != 0 );

		for( TriIterator i = v->adjTris.Begin(); i != v->adjTris.End(); ++i )
		{
			TSimpTri<T>* Tri = *i;
			wconstraint( !Tri->TestFlags( SIMP_REMOVED ) );

			if( !Tri->TestFlags( SIMP_MARK1 ) )
			{
				Tri->EnableFlags( SIMP_MARK1 );
				AdjTris.Add( Tri );
			}
		}
		v = v->next;
	} while( v != v1 );

	wm::float3	BoundsMin = {  wm::MAX_flt,  wm::MAX_flt,  wm::MAX_flt };
	wm::float3	BoundsMax = { -wm::MAX_flt, -wm::MAX_flt, -wm::MAX_flt };

	AdjCorners.Clear();
	WedgeSet.Init( AdjTris.Num() );

	for( uint32 AdjTriIndex = 0, Num = AdjTris.Num(); AdjTriIndex < Num; AdjTriIndex++ )
	{
		for( uint32 CornerIndex = 0; CornerIndex < 3; CornerIndex++ )
		{
			TSimpVert<T>* wrestrict Vert = AdjTris[ AdjTriIndex ]->verts[ CornerIndex ];
			
			uint32 Hash = HashPoint( Vert->GetPos() );
			uint32 AdjCorner;
			for( AdjCorner = AdjCorners.First( Hash ); AdjCorners.IsValid( AdjCorner ); AdjCorner = AdjCorners.Next( AdjCorner ) )
			{
				uint32 OtherAdjTriIndex = AdjCorner >> 2;
				uint32 OtherCornerIndex = AdjCorner & 3;

				TSimpVert<T>* OtherVert = AdjTris[ OtherAdjTriIndex ]->verts[ OtherCornerIndex ];
				if( Vert == OtherVert )
				{
					// Only union wedges with the merged positions. Shared adjverts elsewhere are irrelevant for wedges.
					if( Vert->GetPos() == Position0 ||
						Vert->GetPos() == Position1 )
					{
						WedgeSet.UnionSequential( AdjTriIndex, OtherAdjTriIndex );
					}

					break;
				}
			}
			if( !AdjCorners.IsValid( AdjCorner ) )
			{
				// if didn't find this vert already present, add it.
				AdjCorners.Add( Hash, (AdjTriIndex << 2) | CornerIndex );

				BoundsMin = wm::float3::Min( BoundsMin, Vert->GetPos() );
				BoundsMax = wm::float3::Max( BoundsMax, Vert->GetPos() );
			}
		}
	}

	float BoundsExtentSqr = wm::length_sq( BoundsMax - BoundsMin );
	
	std::vector< uint32>		WedgeIDs;
	std::vector< QuadricType>	WedgeQuadrics;

	for( uint32 AdjTriIndex = 0, Num = AdjTris.size(); AdjTriIndex < Num; AdjTriIndex++ )
	{
		TSimpTri<T>* wrestrict Tri = AdjTris[ AdjTriIndex ];
		Tri->DisableFlags( SIMP_MARK1 );

		int32 QuadricIndex = white::add_unique(WedgeIDs, WedgeSet.Find( AdjTriIndex ) );
		if( QuadricIndex < WedgeQuadrics.size() )
		{
			WedgeQuadrics[ QuadricIndex ] += GetQuadric( Tri );
		}
		else
		{
			WedgeQuadrics.Add( GetQuadric( Tri ) );
		}
	}

	FQuadricAttrOptimizer QuadricOptimizer;
	for( auto& WedgeQuadric : WedgeQuadrics )
	{
		QuadricOptimizer.AddQuadric( WedgeQuadric, NumAttributes );
	}

	// TODO edge quadrics differently
	FQuadric EdgeQuadric;
	EdgeQuadric.Zero();

	v = v0;
	do {
		EdgeQuadric += GetEdgeQuadric(v);
		v = v->next;
	} while( v != v0 );

	v = v1;
	do {
		EdgeQuadric += GetEdgeQuadric(v);
		v = v->next;
	} while( v != v1 );

	QuadricOptimizer.AddQuadric( EdgeQuadric );


	wm::float3 NewPos;
	{
		bool bLocked0 = v0->TestFlags( SIMP_LOCKED );
		bool bLocked1 = v1->TestFlags( SIMP_LOCKED );

		// find position
		if( bLocked0 && !bLocked1 )
		{
			// v0 position
			NewPos = v0->GetPos();
		}
		else if( bLocked1 && !bLocked0 )
		{
			// v1 position
			NewPos = v1->GetPos();
		}
		else
		{
			bool bIsValid = QuadricOptimizer.OptimizeVolume( NewPos );

			// Limit position to be near the neighborhood bounds
			if( !bIsValid || wm::ComputeSquaredDistanceFromBoxToPoint( BoundsMin, BoundsMax, NewPos ) > BoundsExtentSqr * 4.0f )
			{
				bIsValid = QuadricOptimizer.Optimize( NewPos );
			}
			
			if( !bIsValid || wm::ComputeSquaredDistanceFromBoxToPoint( BoundsMin, BoundsMax, NewPos ) > BoundsExtentSqr * 4.0f )
			{
				// Try a point on the edge.
				bIsValid = QuadricOptimizer.OptimizeLinear( Position0, Position1, NewPos );
			}

			if( !bIsValid )
			{
				// Couldn't find optimal so choose middle
				NewPos = ( Position0 + Position1 ) * 0.5f;
			}
		}
	}


	std::vector< T> NewVerts;
	NewVerts.reserve( WedgeQuadrics.size() );
	
	float Error = 0.0f;
	float EdgeError = EdgeQuadric.Evaluate( NewPos );

	for( auto& WedgeQuadric : WedgeQuadrics )
	{
		T& NewVert = NewVerts.emplace_back();
		NewVert.GetPos() = NewPos;

		if( WedgeQuadric.a > 1e-8 )
		{
			// calculate vert attributes from the new position
			float WedgeError = WedgeQuadric.CalcAttributesAndEvaluate( NewPos, NewVert.GetAttributes(), attributeWeights );
			
			// Correct after eval. Normal length is unimportant for error but can bias the calculation.
			NewVert.Correct();

			Error += WedgeError;
		}
		else
		{
			for( uint32 i = 0; i < NumAttributes; i++ )
			{
				NewVert.GetAttributes()[i] = 0.0f;
			}
		}
	}

	if( bMoveVerts )
	{
		for( uint32 AdjTriIndex = 0, Num = AdjTris.size(); AdjTriIndex < Num; AdjTriIndex++ )
		{
			int32 QuadricIndex = white::find_index(WedgeIDs, WedgeSet[ AdjTriIndex ] );

			for( uint32 CornerIndex = 0; CornerIndex < 3; CornerIndex++ )
			{
				TSimpVert<T>* Vert = AdjTris[ AdjTriIndex ]->verts[ CornerIndex ];

				if( Vert->GetPos() == Position0 ||
					Vert->GetPos() == Position1 )
				{
					// Only use attributes if we calculated them.
					if( WedgeQuadrics[ QuadricIndex ].a > 1e-8 )
						Vert->vert = NewVerts[ QuadricIndex ];
					else
						Vert->GetPos() = NewPos;
				}
			}
		}
		
		// Limit error to be no greater than the size of the triangles it could affect.
		Error = std::min( Error, BoundsExtentSqr );
	}
	else
	{
		Error += EdgeError;
		//Error = std::min( Error, BoundsExtentSqr );
		Error += ComputeMergePenalty( v0, v1, NewPos );
	}

	return Error;
}

#if !SIMP_NEW_MERGE
template< typename T, uint32 NumAttributes >
float TMeshSimplifier<T, NumAttributes>::ComputeNewVerts( TSimpEdge<T>* edge, std::vector< T, TInlineAllocator<16> >& newVerts )
{
	TSimpEdge<T>* e;
	TSimpVert<T>* v;

	std::vector< QuadricType, TInlineAllocator<16> > quadrics;

	FQuadricAttrOptimizer optimizer;

	LockVertFlags( SIMP_MARK1 );
	
	edge->v0->EnableFlagsGroup( SIMP_MARK1 );
	edge->v1->EnableFlagsGroup( SIMP_MARK1 );

	// add edges
	e = edge;
	do {
		wconstraint( e == FindEdge( e->v0, e->v1 ) );
		wconstraint( e->v0->adjTris.Num() > 0 );
		wconstraint( e->v1->adjTris.Num() > 0 );

		newVerts.Add( e->v0->vert );

		QuadricType quadric;
		quadric  = GetQuadric( e->v0 );
		quadric += GetQuadric( e->v1 );
		quadrics.Add( quadric );
		optimizer.AddQuadric( quadric );

		e->v0->DisableFlags( SIMP_MARK1 );
		e->v1->DisableFlags( SIMP_MARK1 );

		e = e->next;
	} while( e != edge );

	// add remainder verts
	v = edge->v0;
	do {
		if( v->TestFlags( SIMP_MARK1 ) )
		{
			wconstraint( !v->TestFlags( SIMP_REMOVED ) );
			wconstraint( v->adjTris.Num() != 0 );

			newVerts.Add( v->vert );

			QuadricType quadric;
			quadric = GetQuadric( v );
			quadrics.Add( quadric );
			optimizer.AddQuadric( quadric );

			v->DisableFlags( SIMP_MARK1 );
		}
		v = v->next;
	} while( v != edge->v0 );

	v = edge->v1;
	do {
		if( v->TestFlags( SIMP_MARK1 ) )
		{
			wconstraint( !v->TestFlags( SIMP_REMOVED ) );
			wconstraint( v->adjTris.Num() != 0 );

			newVerts.Add( v->vert );

			QuadricType quadric;
			quadric = GetQuadric( v );
			quadrics.Add( quadric );
			optimizer.AddQuadric( quadric );

			v->DisableFlags( SIMP_MARK1 );
		}
		v = v->next;
	} while( v != edge->v1 );

	UnlockVertFlags( SIMP_MARK1 );

	FQuadric edgeQuadric;
	edgeQuadric.Zero();

	v = edge->v0;
	do {
		edgeQuadric += GetEdgeQuadric( v );
		v = v->next;
	} while( v != edge->v0 );

	v = edge->v1;
	do {
		edgeQuadric += GetEdgeQuadric( v );
		v = v->next;
	} while( v != edge->v1 );

	optimizer.AddQuadric( edgeQuadric );
	
	wm::float3 newPos;
	{
		bool bLocked0 = edge->v0->TestFlags( SIMP_LOCKED );
		bool bLocked1 = edge->v1->TestFlags( SIMP_LOCKED );
		//wconstraint( !bLocked0 || !bLocked1 );

		// find position
		if( bLocked0 && !bLocked1 )
		{
			// v0 position
			newPos = edge->v0->GetPos();
		}
		else if( bLocked1 && !bLocked0 )
		{
			// v1 position
			newPos = edge->v1->GetPos();
		}
		else
		{
			// optimal position
			wm::float3 pos;
			bool valid = optimizer.Optimize( pos, newPos );
			if( !valid )
			{
				// Couldn't find optimal so choose middle
				newPos = ( edge->v0->GetPos() + edge->v1->GetPos() ) * 0.5f;
			}
		}
	}
	
	float cost = 0.0f;

	for( int i = 0; i < quadrics.Num(); i++ )
	{
		newVerts[i].GetPos() = newPos;

		if( quadrics[i].a > 1e-8 )
		{
			// calculate vert attributes from the new position
			quadrics[i].CalcAttributesAndEvaluate( newVerts[i].GetPos(), newVerts[i].GetAttributes(), attributeWeights );
			newVerts[i].Correct();
		}

		// sum cost of new verts
		cost += quadrics[i].Evaluate( newVerts[i].GetPos(), newVerts[i].GetAttributes(), attributeWeights );
	}

	cost += edgeQuadric.Evaluate( newPos );

	return cost;
}

template< typename T, uint32 NumAttributes >
float TMeshSimplifier<T, NumAttributes>::ComputeEdgeCollapseCost( TSimpEdge<T>* edge, bool bMoveVerts )
{
	std::vector< T, TInlineAllocator<16> > newVerts;
	float Error = ComputeNewVerts( edge, newVerts );

	if( bMoveVerts )
	{
		TSimpVert<T>* v;

		uint32 i = 0;

		LockVertFlags( SIMP_MARK1 );

		edge->v0->EnableFlagsGroup( SIMP_MARK1 );
		edge->v1->EnableFlagsGroup( SIMP_MARK1 );

		// edges
		TSimpEdge<T>* e = edge;
		do {
			wconstraint( e == FindEdge( e->v0, e->v1 ) );
			wconstraint( e->v0->adjTris.Num() > 0 );
			wconstraint( e->v1->adjTris.Num() > 0 );

			e->v1->vert = newVerts[ i++ ];
			e->v0->DisableFlags( SIMP_MARK1 );
			e->v1->DisableFlags( SIMP_MARK1 );

			e = e->next;
		} while( e != edge );

		// remainder verts
		v = edge->v0;
		do {
			if( v->TestFlags( SIMP_MARK1 ) )
			{
				wconstraint( !v->TestFlags( SIMP_REMOVED ) );
				wconstraint( v->adjTris.Num() != 0 );

				v->vert = newVerts[ i++ ];
				v->DisableFlags( SIMP_MARK1 );
			}
			v = v->next;
		} while( v != edge->v0 );

		v = edge->v1;
		do {
			if( v->TestFlags( SIMP_MARK1 ) )
			{
				wconstraint( !v->TestFlags( SIMP_REMOVED ) );
				wconstraint( v->adjTris.Num() != 0 );

				v->vert = newVerts[ i++ ];
				v->DisableFlags( SIMP_MARK1 );
			}
			v = v->next;
		} while( v != edge->v1 );

		UnlockVertFlags( SIMP_MARK1 );
	}
	else
	{
		Error += ComputeMergePenalty( edge->v0, edge->v1, newVerts[0].GetPos() );
	}

	return Error;
}
#endif

template< typename T, uint32 NumAttributes >
float TMeshSimplifier<T, NumAttributes>::ComputeMergePenalty( TSimpVert<T>* v0, TSimpVert<T>* v1, const wm::float3& NewPos )
{
	// add penalties
	// the below penalty code works with groups so no need to worry about remainder verts
	TSimpVert<T>* v;

	float penalty = 0.0f;

	{
		const int degreeLimit = 24;
		const float degreePenalty = 100.0f;

		int degree = 0;

		v = v0;
		do {
			degree += v->adjTris.Num();
			v = v->next;
		} while( v != v0 );

		// v
		v = v1;
		do {
			degree += v->adjTris.Num();
			v = v->next;
		} while( v != v1 );

		if( degree > degreeLimit )
			penalty += degreePenalty * ( degree - degreeLimit );
	}

	{
		const float LockPenalty = 1e8f;
		if( v0->TestFlags( SIMP_LOCKED ) && v1->TestFlags( SIMP_LOCKED ) )
			penalty += LockPenalty;
	}

	{
		// Penalty to prevent edge folding 
		const float invalidPenalty = 1000000.0f;

		LockTriFlags( SIMP_MARK1 );
	
		v1->EnableAdjTriFlagsGroup( SIMP_MARK1 );

		// u
		v = v0;
		do {
			for( TriIterator i = v->adjTris.Begin(); i != v->adjTris.End(); ++i )
			{
				TSimpTri<T>* tri = *i;
				if( !tri->TestFlags( SIMP_MARK1 ) )
					penalty += tri->ReplaceVertexIsValid( v, NewPos ) ? 0.0f : invalidPenalty;
				tri->DisableFlags( SIMP_MARK1 );
			}
			v = v->next;
		} while( v != v0 );

		// v
		v = v1;
		do {
			for( TriIterator i = v->adjTris.Begin(); i != v->adjTris.End(); ++i )
			{
				TSimpTri<T>* tri = *i;
				if( tri->TestFlags( SIMP_MARK1 ) )
					penalty += tri->ReplaceVertexIsValid( v, NewPos ) ? 0.0f : invalidPenalty;
				tri->DisableFlags( SIMP_MARK1 );
			}
			v = v->next;
		} while( v != v1 );

		UnlockTriFlags( SIMP_MARK1 );
	}

	return penalty;
}

template< typename T, uint32 NumAttributes >
void TMeshSimplifier<T, NumAttributes>::AddEdgeToHeap( TSimpEdge<T>* edge )
{
	if( edgeHeap.IsPresent( GetEdgeIndex( edge ) ) )
		return;

#if SIMP_NEW_MERGE
	float cost = Merge( edge->v0, edge->v1, false );
#else
	float cost = ComputeEdgeCollapseCost( edge, false );
#endif

	TSimpEdge<T>* e = edge;
	do {
		uint32 EdgeIndex = GetEdgeIndex(e);
		edgeHeap.Add( cost, EdgeIndex );
		e = e->next;
	} while( e != edge );
}

template< typename T, uint32 NumAttributes >
void TMeshSimplifier<T, NumAttributes>::Collapse( TSimpEdge<T>* edge )
{
	TSimpVert<T>* u = edge->v0;
	TSimpVert<T>* v = edge->v1;
	
	// Collapse the edge uv by moving vertex u onto v
	wconstraint( u && v );
	wconstraint( edge == FindEdge( u, v ) );
	wconstraint( u->adjTris.Num() > 0 );
	wconstraint( v->adjTris.Num() > 0 );

	if( u->TestFlags( SIMP_LOCKED ) )
		v->EnableFlags( SIMP_LOCKED );

	LockVertFlags( SIMP_MARK1 );

	// update edges from u to v
	u->EnableAdjVertFlags( SIMP_MARK1 );
	v->DisableAdjVertFlags( SIMP_MARK1 );

	if( u->TestFlags( SIMP_MARK1 ) )
	{
		// Invalid edge, results from collapsing a bridge tri
		// There are no actual triangles connecting these verts
		u->DisableAdjVertFlags( SIMP_MARK1 );
		UnlockVertFlags( SIMP_MARK1 );
		return;
	}

	for( TriIterator i = u->adjTris.Begin(); i != u->adjTris.End(); ++i )
	{
		TSimpTri<T>* tri = *i;
		for( int j = 0; j < 3; j++ )
		{
			TSimpVert<T>* vert = tri->verts[j];
			if( vert->TestFlags( SIMP_MARK1 ) )
			{
				ReplaceEdgeVert( u, vert, v );
				vert->DisableFlags( SIMP_MARK1 );
			}
		}
	}

	// remove dead edges
	u->EnableAdjVertFlags( SIMP_MARK1 );
	u->DisableFlags( SIMP_MARK1 );
	v->DisableFlags( SIMP_MARK1 );

	for( TriIterator i = v->adjTris.Begin(); i != v->adjTris.End(); ++i )
	{
		TSimpTri<T>* tri = *i;
		for( int j = 0; j < 3; j++ )
		{
			TSimpVert<T>* vert = tri->verts[j];
			if( vert->TestFlags( SIMP_MARK1 ) )
			{
				ReplaceEdgeVert( u, vert, NULL );
				vert->DisableFlags( SIMP_MARK1 );
			}
		}
	}

	u->DisableAdjVertFlags( SIMP_MARK1 );

	// fixup triangles
	for( TriIterator i = u->adjTris.Begin(); i != u->adjTris.End(); ++i )
	{
		TSimpTri<T>* tri = *i;
		wconstraint( !tri->TestFlags( SIMP_REMOVED ) );
		wconstraint( tri->HasVertex(u) );
		
		if( tri->HasVertex(v) )
		{
			// delete triangles on edge uv
			numTris--;
			tri->EnableFlags( SIMP_REMOVED );
			TriQuadricsValid[ GetTriIndex( tri ) ] = false;

			// remove references to tri
			for( int j = 0; j < 3; j++ )
			{
				TSimpVert<T>* vert = tri->verts[j];
				wconstraint( !vert->TestFlags( SIMP_REMOVED ) );
				if( vert != u )
				{
					vert->adjTris.Remove( tri );
					updateVerts.AddUnique( vert );
				}
			}
		}
		else
		{
			// update triangles to have v instead of u
			tri->ReplaceVertex( u, v );
			v->adjTris.Add( tri );
		}
	}
	
	UnlockVertFlags( SIMP_MARK1 );
	
	u->adjTris.Clear();	// u has been removed
	updateVerts.AddUnique( u );
}

template< typename T, uint32 NumAttributes >
void TMeshSimplifier<T, NumAttributes>::UpdateTris()
{
	// remove degenerate triangles
	// not sure why this happens
	for( TSimpTri<T>* tri : updateTris )
	{
		if( tri->TestFlags( SIMP_REMOVED ) )
			continue;

		TriQuadricsValid[ GetTriIndex( tri ) ] = false;

		const wm::float3& p0 = tri->verts[0]->GetPos();
		const wm::float3& p1 = tri->verts[1]->GetPos();
		const wm::float3& p2 = tri->verts[2]->GetPos();

		if( p0 == p1 ||
			p1 == p2 ||
			p2 == p0 )
		{
			numTris--;
			tri->EnableFlags( SIMP_REMOVED );

			// remove references to tri
			for( int j = 0; j < 3; j++ )
			{
				TSimpVert<T>* vert = tri->verts[j];
				vert->adjTris.Remove( tri );

				updateVerts.AddUnique( vert );
				// orphaned verts are removed below
			}
		}
	}
	updateTris.Reset();
}

template< typename T, uint32 NumAttributes >
void TMeshSimplifier<T, NumAttributes>::UpdateVerts()
{
	// remove orphaned verts
	for( TSimpVert<T>* vert : updateVerts )
	{
		if( vert->TestFlags( SIMP_REMOVED ) )
			continue;

		VertQuadricsValid[ GetVertIndex( vert ) ] = false;
		EdgeQuadricsValid[ GetVertIndex( vert ) ] = false;

		if( vert->adjTris.Num() == 0 )
		{
			RemoveVert( vert );
		}
	}
	updateVerts.Reset();
}

template< typename T, uint32 NumAttributes >
void TMeshSimplifier<T, NumAttributes>::UpdateEdges()
{
	// add all grouped edges
	for( uint32 i = 0, Num = updateEdges.Num(); i < Num; i++ )
	{
		TSimpEdge<T>* edge = updateEdges[i];

		if( edge->TestFlags( SIMP_REMOVED ) )
			continue;

		TSimpEdge<T>* e = edge;
		do {
			updateEdges.AddUnique(e);
			e = e->next;
		} while( e != edge );
	}
		
	// remove dead edges
	for( TSimpEdge<T>* edge : updateEdges )
	{
		if( edge->TestFlags( SIMP_REMOVED ) )
			continue;

		if( edge->v0->TestFlags( SIMP_REMOVED ) ||
			edge->v1->TestFlags( SIMP_REMOVED ) )
		{
			RemoveEdge( edge );
			continue;
		}
	}

	// Fix edge groups
	{
		white::FHashTable HashTable( 128, updateEdges.size() );

		// ungroup edges
		for( uint32 i = 0, Num = updateEdges.size(); i < Num; i++ )
		{
			TSimpEdge<T>* edge = updateEdges[i];

			if( edge->TestFlags( SIMP_REMOVED ) )
				continue;
			
			edge->next = edge;
			edge->prev = edge;

			edgeHeap.Remove( GetEdgeIndex( edge ) );

			uint32 Hash0 = HashPoint( edge->v0->GetPos() );
			uint32 Hash1 = HashPoint( edge->v1->GetPos() );
			uint32 Hash = Murmur32( { std::min( Hash0, Hash1 ), std::max( Hash0, Hash1 ) } );
				
			HashTable.Add( Hash, i );
		}

		// regroup edges
		for( TSimpEdge<T>* edge : updateEdges )
		{
			if( edge->TestFlags( SIMP_REMOVED ) )
				continue;

			// already grouped
			if( edge->next != edge )
				continue;

			// find any matching edges
			uint32 Hash0 = HashPoint( edge->v0->GetPos() );
			uint32 Hash1 = HashPoint( edge->v1->GetPos() );
			uint32 Hash = Murmur32( { std::min( Hash0, Hash1 ), std::max( Hash0, Hash1 ) } );
			for( uint32 j = HashTable.First( Hash ); HashTable.IsValid(j); j = HashTable.Next( j ) )
			{
				TSimpEdge<T>* e1 = edge;
				TSimpEdge<T>* e2 = updateEdges[j];

				if( e1 == e2 )
					continue;

				bool m1 =	e1->v0->GetPos() == e2->v0->GetPos() &&
							e1->v1->GetPos() == e2->v1->GetPos();

				bool m2 =	e1->v0->GetPos() == e2->v1->GetPos() &&
							e1->v1->GetPos() == e2->v0->GetPos();

				// backwards
				if( m2 )
					Swap( e2->v0, e2->v1 );

				// link
				if( m1 || m2 )
				{
					wconstraint( e2->next == e2 );
					wconstraint( e2->prev == e2 );

					e2->next = e1->next;
					e2->prev = e1;
					e2->next->prev = e2;
					e2->prev->next = e2;
				}
			}
		}
	}

	// update edges
	for( TSimpEdge<T>* edge : updateEdges )
	{
		if( edge->TestFlags( SIMP_REMOVED ) )
			continue;
		
		AddEdgeToHeap( edge );
	}

	updateEdges.Reset();
}

template< typename T, uint32 NumAttributes >
float TMeshSimplifier<T, NumAttributes>::SimplifyMesh( float maxErrorLimit, int minTris, int minVerts )
{
	TSimpVert<T>* v;

	float maxError = 0.0f;

	while( edgeHeap.Num() > 0 )
	{
		if( numTris <= minTris || numVerts <= minVerts )
			break;

		// get the next vertex to collapse
		uint32 TopIndex = edgeHeap.Top();

		float error = edgeHeap.GetKey( TopIndex );
		if( error > maxErrorLimit )
		{
			break;
		}
		//maxError = std::max( maxError, error );
		
		edgeHeap.Pop();

		TSimpEdge<T>* top = &edges[ TopIndex ];

		int numEdges = 0;
		TSimpEdge<T>* edgeList[32];

		TSimpEdge<T>* edge = top;
		do {
			edgeList[ numEdges++ ] = edge;
			edge = edge->next;
		} while( edge != top );

		wconstraint(top);

		// skip locked edges
		bool locked = false;
		for( int i = 0; i < numEdges; i++ )
		{
			edge = edgeList[i];
			if( edge->v0->TestFlags( SIMP_LOCKED ) && edge->v1->TestFlags( SIMP_LOCKED ) )
			{
				locked = true;
			}
		}
		//if( locked )
		//	continue;

		v = top->v0;
		do {
			GatherUpdates( v );
			v = v->next;
		} while( v != top->v0 );

		v = top->v1;
		do {
			GatherUpdates( v );
			v = v->next;
		} while( v != top->v1 );

#if 1
		// remove edges with already removed verts
		// not sure why this happens
		for( int i = 0; i < numEdges; i++ )
		{
			if( edgeList[i]->v0->adjTris.Num() == 0 ||
				edgeList[i]->v1->adjTris.Num() == 0 )
			{
				RemoveEdge( edgeList[i] );
				edgeList[i] = NULL;
			}
			else
			{
				wconstraint( !edgeList[i]->TestFlags( SIMP_REMOVED ) );
			}
		}
		if( top->v0->adjTris.Num() == 0 || top->v1->adjTris.Num() == 0 )
			continue;
#endif

		// move verts to new verts
		{	
			edge = top;
			
#if SIMP_NEW_MERGE
			float Error = Merge( edge->v0, edge->v1, true );
#else
			float Error = ComputeEdgeCollapseCost( edge, true );
#endif
			maxError = std::max( maxError, Error );
		}

		// collapse all edges
		for( int i = 0; i < numEdges; i++ )
		{
			edge = edgeList[i];
			if( !edge )
				continue;
			if( edge->TestFlags( SIMP_REMOVED ) )	// wtf?
				continue;
			if( edge->v0->adjTris.Num() == 0 )
				continue;
			if( edge->v1->adjTris.Num() == 0 )
				continue;

			Collapse( edge );
			RemoveEdge( edge );
		}

		// add v0 remainder verts to v1
		{
			// combine v0 and v1 groups
			top->v0->next->prev = top->v1->prev;
			top->v1->prev->next = top->v0->next;
			top->v0->next = top->v1;
			top->v1->prev = top->v0;

			// ungroup removed verts
			std::vector< TSimpVert<T>*> vertList;
			
			v = top->v1;
			do {
				vertList.Add(v);
				v = v->next;
			} while( v != top->v1 );
			
			for( TSimpVert<T>* u : vertList )
			{
				if( u->TestFlags( SIMP_REMOVED ) )
				{
					// ungroup
					u->prev->next = u->next;
					u->next->prev = u->prev;
					u->next = u;
					u->prev = u;
				}
			}
		}

		{
			// spread locked flag to vert group
			uint32 flags = 0;

			v = top->v1;
			do {
				flags |= v->flags & SIMP_LOCKED;
				v = v->next;
			} while( v != top->v1 );

			v = top->v1;
			do {
				v->flags |= flags;
				v = v->next;
			} while( v != top->v1 );
		}

		UpdateTris();
		UpdateVerts();
		UpdateEdges();
	}

	// remove degenerate triangles
	// not sure why this happens
	for( int i = 0; i < numSTris; i++ )
	{
		TSimpTri<T>* tri = &sTris[i];

		if( tri->TestFlags( SIMP_REMOVED ) )
			continue;

		const wm::float3& p0 = tri->verts[0]->GetPos();
		const wm::float3& p1 = tri->verts[1]->GetPos();
		const wm::float3& p2 = tri->verts[2]->GetPos();

		if( p0 == p1 ||
			p1 == p2 ||
			p2 == p0 )
		{
			numTris--;
			tri->EnableFlags( SIMP_REMOVED );

			// remove references to tri
			for( int j = 0; j < 3; j++ )
			{
				TSimpVert<T>* vert = tri->verts[j];
				vert->adjTris.Remove( tri );
				// orphaned verts are removed below
			}
		}
	}

	// remove orphaned verts
	for( int i = 0; i < numSVerts; i++ )
	{
		TSimpVert<T>* vert = &sVerts[i];

		if( vert->TestFlags( SIMP_REMOVED ) )
			continue;

		if( vert->adjTris.Num() == 0 )
		{
			RemoveVert( vert );
		}
	}

	return maxError;
}

template< typename T, uint32 NumAttributes >
void TMeshSimplifier<T, NumAttributes>::OutputMesh( T* verts, uint32* indexes, uint32* OutNumVertices, uint32* OutNumIndices ) const
{
	white::FHashTable HashTable( 1 << std::min( 16u, wm::FloorLog2( GetNumVerts() ) ), GetNumVerts() );

#if 1
	int count = 0;
	for( int i = 0; i < numSVerts; i++ )
		count += sVerts[i].TestFlags( SIMP_REMOVED ) ? 0 : 1;

	wconstraint( numVerts == count );
#endif

	int numV = 0;
	int numI = 0;

	for( int i = 0; i < numSTris; i++ )
	{
		if( sTris[i].TestFlags( SIMP_REMOVED ) )
			continue;

		// TODO this is sloppy. There should be no duped verts. Alias by index.
		for( int j = 0; j < 3; j++ )
		{
			TSimpVert<T>* vert = sTris[i].verts[j];
			wconstraint( !vert->TestFlags( SIMP_REMOVED ) );
			wconstraint( vert->adjTris.Num() != 0 );

			const wm::float3& p = vert->GetPos();
			uint32 hash = HashPoint( p );
			uint32 f;
			for( f = HashTable.First( hash ); HashTable.IsValid(f); f = HashTable.Next( f ) )
			{
				if( vert->vert == verts[f] )
					break;
			}
			if( !HashTable.IsValid(f) )
			{
				HashTable.Add( hash, numV );
				verts[ numV ] = vert->vert;
				indexes[ numI++ ] = numV;
				numV++;
			}
			else
			{
				indexes[ numI++ ] = f;
			}
		}
	}

	wconstraint( numV <= numVerts );
	wconstraint( numI <= numTris * 3 );
	
	if (OutNumVertices)
	{
		*OutNumVertices = numV;
	}

	if (OutNumIndices)
	{
		*OutNumIndices = numI;
	}
}

template< typename T, uint32 NumAttributes >
template< typename FaceType >
void TMeshSimplifier<T, NumAttributes>::CompactFaceData( std::vector< FaceType>& FaceData )
{
	std::vector< FaceType> NewFaceData;

	for( int i = 0; i < numSTris; i++ )
	{
		if( sTris[i].TestFlags( SIMP_REMOVED ) )
			continue;

		NewFaceData.Add( FaceData[i] );
	}

	Swap( FaceData, NewFaceData );
}