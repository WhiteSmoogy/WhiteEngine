#pragma once

#include <WBase/winttype.hpp>
#include <WBase/cassert.h>
#include "Core/Math/PlatformMath.h"
#include "Core/PlatformAtomics.h"
namespace white
{
	/*-----------------------------------------------------------------------------
	Dynamically sized hash table, used to index another data structure.
	Vastly simpler and faster than TMap.

	Example find:

	uint32 Key = HashFunction( ID );
	for( uint32 i = HashTable.First( Key ); HashTable.IsValid( i ); i = HashTable.Next( i ) )
	{
		if( Array[i].ID == ID )
		{
			return Array[i];
		}
	}
	-----------------------------------------------------------------------------*/
	class FHashTable
	{
	public:
		FHashTable(uint32 InHashSize = 1024, uint32 InIndexSize = 0);
		FHashTable(const FHashTable& Other);
		~FHashTable();

		void			Clear();
		void			Clear(uint32 InHashSize, uint32 InIndexSize = 0);
		void			Free();
		void	Resize(uint32 NewIndexSize);

		// Functions used to search
		uint32			First(uint32 Key) const;
		uint32			Next(uint32 Index) const;
		bool			IsValid(uint32 Index) const;

		void			Add(uint32 Key, uint32 Index);
		void			Add_Concurrent(uint32 Key, uint32 Index);
		void			Remove(uint32 Key, uint32 Index);

		// Average # of compares per search
		float	AverageSearch() const;

	protected:
		// Avoids allocating hash until first add
		static uint32	EmptyHash[1];

		uint32			HashSize;
		uint32			HashMask;
		uint32			IndexSize;

		uint32* Hash;
		uint32* NextIndex;
	};


	inline FHashTable::FHashTable(uint32 InHashSize, uint32 InIndexSize)
		: HashSize(InHashSize)
		, HashMask(0)
		, IndexSize(InIndexSize)
		, Hash(EmptyHash)
		, NextIndex(nullptr)
	{
		wconstraint(HashSize > 0);
		wconstraint(math::IsPowerOfTwo(HashSize));

		if (IndexSize)
		{
			HashMask = HashSize - 1;

			Hash = new uint32[HashSize];
			NextIndex = new uint32[IndexSize];

			std::memset(Hash, 0xff, HashSize * 4);
		}
	}

	inline FHashTable::FHashTable(const FHashTable& Other)
		: HashSize(Other.HashSize)
		, HashMask(Other.HashMask)
		, IndexSize(Other.IndexSize)
		, Hash(EmptyHash)
	{
		if (IndexSize)
		{
			Hash = new uint32[HashSize];
			NextIndex = new uint32[IndexSize];

			std::memcpy(Hash, Other.Hash, HashSize * 4);
			std::memcpy(NextIndex, Other.NextIndex, IndexSize * 4);
		}
	}

	inline FHashTable::~FHashTable()
	{
		Free();
	}

	inline void FHashTable::Clear()
	{
		if (IndexSize)
		{
			std::memset(Hash, 0xff, HashSize * 4);
		}
	}

	inline void FHashTable::Clear(uint32 InHashSize, uint32 InIndexSize)
	{
		Free();

		HashSize = InHashSize;
		IndexSize = InIndexSize;

		wconstraint(HashSize > 0);
		wconstraint(math::IsPowerOfTwo(HashSize));

		if (IndexSize)
		{
			HashMask = HashSize - 1;

			Hash = new uint32[HashSize];
			NextIndex = new uint32[IndexSize];

			std::memset(Hash, 0xff, HashSize * 4);
		}
	}

	inline void FHashTable::Free()
	{
		if (IndexSize)
		{
			HashMask = 0;
			IndexSize = 0;

			delete[] Hash;
			Hash = EmptyHash;

			delete[] NextIndex;
			NextIndex = nullptr;
		}
	}

	// First in hash chain
	inline uint32 FHashTable::First(uint32 Key) const
	{
		Key &= HashMask;
		return Hash[Key];
	}

	// Next in hash chain
	inline uint32 FHashTable::Next(uint32 Index) const
	{
		wconstraint(Index < IndexSize);
		wconstraint(NextIndex[Index] != Index); // check for corrupt tables
		return NextIndex[Index];
	}

	inline bool FHashTable::IsValid(uint32 Index) const
	{
		return Index != ~0u;
	}

	inline void FHashTable::Add(uint32 Key, uint32 Index)
	{
		if (Index >= IndexSize)
		{
			Resize(std::max< uint32 >(32u, std::bit_ceil(Index + 1)));
		}

		Key &= HashMask;
		NextIndex[Index] = Hash[Key];
		Hash[Key] = Index;
	}

	// Safe for many threads to add concurrently.
	// Not safe to search the table while other threads are adding.
	// Will not resize. Only use for presized tables.
	inline void FHashTable::Add_Concurrent(uint32 Key, uint32 Index)
	{
		wconstraint(Index < IndexSize);

		Key &= HashMask;
		NextIndex[Index] =white::InterlockedExchange((int32*)&Hash[Key], Index);
	}

	inline void FHashTable::Remove(uint32 Key, uint32 Index)
	{
		if (Index >= IndexSize)
		{
			return;
		}

		Key &= HashMask;

		if (Hash[Key] == Index)
		{
			// Head of chain
			Hash[Key] = NextIndex[Index];
		}
		else
		{
			for (uint32 i = Hash[Key]; IsValid(i); i = NextIndex[i])
			{
				if (NextIndex[i] == Index)
				{
					// Next = Next->Next
					NextIndex[i] = NextIndex[Index];
					break;
				}
			}
		}
	}
}