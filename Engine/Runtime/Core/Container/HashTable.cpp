#include "HashTable.h"

using namespace white;

uint32 FHashTable::EmptyHash[1] = { ~0u };

void FHashTable::Resize(uint32 NewIndexSize)
{
	if (NewIndexSize == IndexSize)
	{
		return;
	}

	if (NewIndexSize == 0)
	{
		Free();
		return;
	}

	if (IndexSize == 0)
	{
		HashMask = (uint16)(HashSize - 1);
		Hash = new uint32[HashSize];
		std::memset(Hash, 0xff, HashSize * 4);
	}

	uint32* NewNextIndex = new uint32[NewIndexSize];

	if (NextIndex)
	{
		std::memcpy(NewNextIndex, NextIndex, IndexSize * 4);
		delete[] NextIndex;
	}

	IndexSize = NewIndexSize;
	NextIndex = NewNextIndex;
}

float FHashTable::AverageSearch() const
{
	uint32 SumAvgSearch = 0;
	uint32 NumElements = 0;
	for (uint32 Key = 0; Key < HashSize; Key++)
	{
		uint32 NumInBucket = 0;
		for (uint32 i = First((uint16)Key); IsValid(i); i = Next(i))
		{
			NumInBucket++;
		}

		SumAvgSearch += NumInBucket * (NumInBucket + 1);
		NumElements += NumInBucket;
	}
	return (float)(SumAvgSearch >> 1) / (float)NumElements;
}
