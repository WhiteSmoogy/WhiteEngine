// Copyright (C) 2009 Nine Realms, Inc
//

#pragma once

#include "CoreTypes.h"
#include "Developer/MeshSimplifier/UnrolledLinkList.h"
#include "Runtime/Core/Container/vector.hpp"

template< typename T > class TSimpTri;

enum ESimpElementFlags
{
	SIMP_DEFAULT	= 0,
	SIMP_REMOVED	= 1 << 0,
	SIMP_MARK1		= 1 << 1,
	SIMP_MARK2		= 1 << 2,
	SIMP_LOCKED		= 1 << 3,
};

template< typename T >
class TSimpTri;

template< typename T >
class TSimpVert
{
public:
					TSimpVert();

	wm::float3&		GetPos()					{ return vert.GetPos(); }
	const wm::float3&	GetPos() const				{ return vert.GetPos(); }
	float*			GetAttributes()				{ return vert.GetAttributes(); }
	const float*	GetAttributes() const		{ return vert.GetAttributes(); }

	// this vert
	void			EnableFlags( white::uint32 f );
	void			DisableFlags( white::uint32 f );
	bool			TestFlags( white::uint32 f ) const;

	void			EnableAdjVertFlags( white::uint32 f );
	void			DisableAdjVertFlags( white::uint32 f );

	void			EnableAdjTriFlags( white::uint32 f );
	void			DisableAdjTriFlags( white::uint32 f );

	template< typename Allocator >
	void			FindAdjacentVerts( std::vector< TSimpVert<T>*, Allocator >& adjVerts );

	// all verts in group
	void			EnableFlagsGroup( white::uint32 f );
	void			DisableFlagsGroup( white::uint32 f );
	//bool			TestFlagsGroup( white::uint32 f ) const;

	void			EnableAdjVertFlagsGroup( white::uint32 f );
	void			DisableAdjVertFlagsGroup( white::uint32 f );

	void			EnableAdjTriFlagsGroup( white::uint32 f );
	void			DisableAdjTriFlagsGroup( white::uint32 f );

	template< typename Allocator >
	void			FindAdjacentVertsGroup(std::vector< TSimpVert<T>*, Allocator >& adjVerts );

	typedef TUnrolledLinkList< TSimpTri<T>*, 8 >	TriList;
	typedef typename TriList::TIterator				TriIterator;
	
	// other verts sharing same point grouped
	TSimpVert<T>*	next;
	TSimpVert<T>*	prev;
	
	// bitfield of ESimpElementFlags
	white::uint32			flags;

	T				vert;

	// Adjacent triangles, all triangles which reference this vert
	// TODO could be link list of index buffer elements
	// /3 gives tri index
	TriList			adjTris;
};

template< typename T >
class TSimpTri
{
public:
	TSimpTri()
	: flags( SIMP_DEFAULT )
	{}

	void			EnableFlags( white::uint32 f );
	void			DisableFlags( white::uint32 f );
	bool			TestFlags( white::uint32 f ) const;
	
	bool			HasVertex( const TSimpVert<T>* v ) const;
	wm::float3			GetNormal() const;
	
	bool			ReplaceVertexIsValid( const TSimpVert<T>* oldV, const wm::float3& pos ) const;
	void			ReplaceVertex( TSimpVert<T>* oldV, TSimpVert<T>* newV );

	TSimpVert<T>*	verts[3];

	// bitfield of ESimpElementFlags
	white::uint32			flags;
};

template< typename T >
class TSimpEdge
{
public:
					TSimpEdge();

	void			EnableFlags( white::uint32 f );
	void			DisableFlags( white::uint32 f );
	bool			TestFlags( white::uint32 f ) const;

	// Edge group
	// Link list of all edges sharing the same end points, different attributes.
	// Need multiple edges to know which in both vert groups are connected.
	TSimpEdge*		next;
	TSimpEdge*		prev;

	TSimpVert<T>*	v0;
	TSimpVert<T>*	v1;

	// bitfield of ESimpElementFlags
	white::uint32			flags;
};


//=============
// TSimpVert
//=============
template< typename T >
TSimpVert<T>::TSimpVert()
: flags( SIMP_DEFAULT )
{
	next = this;
	prev = this;
}

template< typename T >
void TSimpVert<T>::EnableFlags( white::uint32 f )
{
	flags |= f;
}

template< typename T >
void TSimpVert<T>::DisableFlags( white::uint32 f )
{
	flags &= ~f;
}

template< typename T >
bool TSimpVert<T>::TestFlags( white::uint32 f ) const
{
	return ( flags & f ) == f;
}

template< typename T >
void TSimpVert<T>::EnableAdjVertFlags( white::uint32 f )
{
	for( TriIterator i = adjTris.Begin(); i != adjTris.End(); ++i )
	{
		for( int j = 0; j < 3; j++ )
		{
			(*i)->verts[j]->EnableFlags(f);
		}
	}
}

template< typename T >
void TSimpVert<T>::DisableAdjVertFlags( white::uint32 f )
{
	for( TriIterator i = adjTris.Begin(); i != adjTris.End(); ++i )
	{
		for( int j = 0; j < 3; j++ )
		{
			(*i)->verts[j]->DisableFlags(f);
		}
	}
}

template< typename T >
void TSimpVert<T>::EnableAdjTriFlags( white::uint32 f )
{
	for( TriIterator i = adjTris.Begin(); i != adjTris.End(); ++i )
	{
		(*i)->EnableFlags(f);
	}
}

template< typename T >
void TSimpVert<T>::DisableAdjTriFlags( white::uint32 f )
{
	for( TriIterator i = adjTris.Begin(); i != adjTris.End(); ++i )
	{
		(*i)->DisableFlags(f);
	}
}

template< typename T >
template< typename Allocator >
void TSimpVert<T>::FindAdjacentVerts( std::vector< TSimpVert<T>*, Allocator >& adjVerts )
{
	// fill array
	for( TriIterator i = adjTris.Begin(); i != adjTris.End(); ++i )
	{
		for( int j = 0; j < 3; j++ )
		{
			TSimpVert* v = (*i)->verts[j];
			if( v != this )
			{
				white::add_unique(adjVerts, v);;
			}
		}
	}
}

template< typename T >
void TSimpVert<T>::EnableFlagsGroup( white::uint32 f )
{
	TSimpVert<T>* v = this;
	do {
		v->EnableFlags( f );
		v = v->next;
	} while( v != this );
}

template< typename T >
void TSimpVert<T>::DisableFlagsGroup( white::uint32 f )
{
	TSimpVert<T>* v = this;
	do {
		v->DisableFlags( f );
		v = v->next;
	} while( v != this );
}

template< typename T >
void TSimpVert<T>::EnableAdjVertFlagsGroup( white::uint32 f )
{
	TSimpVert<T>* v = this;
	do {
		v->EnableAdjVertFlags( f );
		v = v->next;
	} while( v != this );
}

template< typename T >
void TSimpVert<T>::DisableAdjVertFlagsGroup( white::uint32 f )
{
	TSimpVert<T>* v = this;
	do {
		v->DisableAdjVertFlags( f );
		v = v->next;
	} while( v != this );
}

template< typename T >
void TSimpVert<T>::EnableAdjTriFlagsGroup( white::uint32 f )
{
	TSimpVert<T>* v = this;
	do {
		v->EnableAdjTriFlags( f );
		v = v->next;
	} while( v != this );
}

template< typename T >
void TSimpVert<T>::DisableAdjTriFlagsGroup( white::uint32 f )
{
	TSimpVert<T>* v = this;
	do {
		v->DisableAdjTriFlags( f );
		v = v->next;
	} while( v != this );
}

template< typename T >
template< typename Allocator >
void TSimpVert<T>::FindAdjacentVertsGroup(std::vector< TSimpVert<T>*, Allocator >& adjVerts )
{
	// fill array
	TSimpVert<T>* v = this;
	do {
		for( TriIterator i = adjTris.Begin(); i != adjTris.End(); ++i )
		{
			for( int j = 0; j < 3; j++ )
			{
				TSimpVert* TriVert = (*i)->verts[j];
				if( TriVert != v )
				{
					white::add_unique(adjVerts,TriVert );
				}
			}
		}
		v = v->next;
	} while( v != this );
}


//=============
// TSimpTri
//=============
template< typename T >
void TSimpTri<T>::EnableFlags( white::uint32 f )
{
	flags |= f;
}

template< typename T >
void TSimpTri<T>::DisableFlags( white::uint32 f )
{
	flags &= ~f;
}

template< typename T >
bool TSimpTri<T>::TestFlags( white::uint32 f ) const
{
	return ( flags & f ) == f;
}

template< typename T >
bool TSimpTri<T>::HasVertex( const TSimpVert<T>* v ) const
{
	return ( v == verts[0] || v == verts[1] || v == verts[2] );
}

template< typename T >
wm::float3 TSimpTri<T>::GetNormal() const
{
	wm::float3 n = wm::cross(( verts[2]->GetPos() - verts[0]->GetPos() ), ( verts[1]->GetPos() - verts[0]->GetPos() ));
	n = wm::normalize(n);
	return n;
}

template< typename T >
bool TSimpTri<T>::ReplaceVertexIsValid( const TSimpVert<T>* oldV, const wm::float3& pos ) const
{
	wconstraint( oldV );
	wconstraint( oldV == verts[0] || oldV == verts[1] || oldV == verts[2] );

	white::uint32 k;
	if( oldV == verts[0] )
		k = 0;
	else if( oldV == verts[1] )
		k = 1;
	else
		k = 2;
	
	const wm::float3& v0 = verts[k]->GetPos();
	const wm::float3& v1 = verts[ k = (1 << k) & 3 ]->GetPos();
	const wm::float3& v2 = verts[ k = (1 << k) & 3 ]->GetPos();
	
	const wm::float3 d21 = v2 - v1;
	const wm::float3 d01 = v0 - v1;
	const wm::float3 dp1 = pos - v1;

	wm::float3 n0 = d01 ^ d21;
	wm::float3 n1 = dp1 ^ d21;

	return wm::dot(n0 , n1) > 0.0f;
}

template< typename T >
void TSimpTri<T>::ReplaceVertex( TSimpVert<T>* oldV, TSimpVert<T>* newV )
{
	wconstraint( oldV && newV );
	wconstraint( oldV == verts[0] || oldV == verts[1] || oldV == verts[2] );
	wconstraint( newV != verts[0] && newV != verts[1] && newV != verts[2] );

	if( oldV == verts[0] )
		verts[0] = newV;
	else if( oldV == verts[1] )
		verts[1] = newV;
	else
		verts[2] = newV;

	wconstraint( !HasVertex( oldV ) );
}


//=============
// TSimpEdge
//=============
template< typename T >
TSimpEdge<T>::TSimpEdge()
: flags( SIMP_DEFAULT )
{
	next = this;
	prev = this;
}

template< typename T >
void TSimpEdge<T>::EnableFlags( white::uint32 f )
{
	flags |= f;
}

template< typename T >
void TSimpEdge<T>::DisableFlags( white::uint32 f )
{
	flags &= ~f;
}

template< typename T >
bool TSimpEdge<T>::TestFlags( white::uint32 f ) const
{
	return ( flags & f ) == f;
}
