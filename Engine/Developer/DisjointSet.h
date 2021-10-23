// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include <LBase/linttype.hpp>
#include <LBase/cassert.h>
#include <vector>

namespace leo
{
	class FDisjointSet
	{
	public:
		FDisjointSet() {}
		FDisjointSet(const uint32 Size);

		void	Init(uint32 Size);
		void	Reset();
		void	AddDefaulted(uint32 Num = 1);

		void	Union(uint32 x, uint32 y);
		void	UnionSequential(uint32 x, uint32 y);
		uint32	Find(uint32 i);

		uint32	operator[](uint32 i) const { return Parents[i]; }

	private:
		std::vector< uint32 >	Parents;
	};

	FDisjointSet::FDisjointSet(const uint32 Size)
	{
		Init(Size);
	}

	void FDisjointSet::Init(uint32 Size)
	{
		Parents.resize(Size);
		for (uint32 i = 0; i < Size; i++)
		{
			Parents[i] = i;
		}
	}

	void FDisjointSet::Reset()
	{
		Parents.clear();
	}

	void FDisjointSet::AddDefaulted(uint32 Num)
	{
		uint32 Start = Parents.size();
		Parents.resize(Start+Num);

		for (uint32 i = Start; i < Start + Num; i++)
		{
			Parents[i] = i;
		}
	}

	// Union with splicing
	void FDisjointSet::Union(uint32 x, uint32 y)
	{
		uint32 px = Parents[x];
		uint32 py = Parents[y];

		while (px != py)
		{
			// Pick larger
			if (px < py)
			{
				Parents[x] = py;
				if (x == px)
				{
					return;
				}
				x = px;
				px = Parents[x];
			}
			else
			{
				Parents[y] = px;
				if (y == py)
				{
					return;
				}
				y = py;
				py = Parents[y];
			}
		}
	}

	// Optimized version of Union when iterating for( x : 0 to N ) unioning x with lower indexes.
	// Neither x nor y can have already been unioned with an index > x.
	void FDisjointSet::UnionSequential(uint32 x, uint32 y)
	{
		lconstraint(x >= y);
		lconstraint(x == Parents[x]);

		uint32 px = x;
		uint32 py = Parents[y];
		while (px != py)
		{
			Parents[y] = px;
			if (y == py)
			{
				return;
			}
			y = py;
			py = Parents[y];
		}
	}

	// Find with path compression
	uint32 FDisjointSet::Find(uint32 i)
	{
		// Find root
		uint32 Start = i;
		uint32 Root = Parents[i];
		while (Root != i)
		{
			i = Root;
			Root = Parents[i];
		}

		// Point all nodes on path to root
		i = Start;
		uint32 Parent = Parents[i];
		while (Parent != Root)
		{
			Parents[i] = Root;
			i = Parent;
			Parent = Parents[i];
		}

		return Root;
	}
}