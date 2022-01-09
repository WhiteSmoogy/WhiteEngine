#pragma once

#include "CoreTypes.h"
#include "Runtime/Core/ParallelFor.h"
#include "Runtime/Core/Sorting.h"
#include "Runtime/Core/Container/DisjointSet.h"
#include "Runtime/Core/Math/PlatformMath.h"
#include "Bounds.h"
#include "metis.h"
#include <atomic>
#include <unordered_map>
namespace WhiteEngine
{
	class FGraphPartitioner
	{
	public:
		struct FGraphData
		{
			int32	Offset;
			int32	Num;

			std::vector< idx_t >	Adjacency;
			std::vector< idx_t >	AdjacencyCost;
			std::vector< idx_t >	AdjacencyOffset;
		};

		// Inclusive
		struct FRange
		{
			uint32	Begin;
			uint32	End;

			bool operator<(const FRange& Other) const { return Begin < Other.Begin; }
		};
		std::vector< FRange >	Ranges;
		std::vector< uint32 >	Indexes;

	public:
		FGraphPartitioner(uint32 InNumElements);

		FGraphData* NewGraph(uint32 NumAdjacency) const;

		void		AddAdjacency(FGraphData* Graph, uint32 AdjIndex, idx_t Cost);
		void		AddLocalityLinks(FGraphData* Graph, uint32 Index, idx_t Cost);

		template< typename FGetCenter >
		void		BuildLocalityLinks(white::FDisjointSet& DisjointSet, const FBounds& Bounds, FGetCenter& GetCenter);

		void		Partition(FGraphData* Graph, int32 InMinPartitionSize, int32 InMaxPartitionSize);
		void		PartitionStrict(FGraphData* Graph, int32 InMinPartitionSize, int32 InMaxPartitionSize, bool bThreaded);

	private:
		void		BisectGraph(FGraphData* Graph, FGraphData* ChildGraphs[2]);
		void		RecursiveBisectGraph(FGraphData* Graph);

		uint32		NumElements;
		int32		MinPartitionSize = 0;
		int32		MaxPartitionSize = 0;

		std::atomic< uint32 >	NumPartitions;

		std::vector< idx_t >		PartitionIDs;
		std::vector< int32 >		SwappedWith;
		std::vector< uint32 >	SortedTo;

		std::unordered_multimap< uint32, uint32 >	LocalityLinks;
	};

	inline void FGraphPartitioner::AddAdjacency(FGraphData* Graph, uint32 AdjIndex, idx_t Cost)
	{
		Graph->Adjacency.emplace_back(SortedTo[AdjIndex]);
		Graph->AdjacencyCost.emplace_back(Cost);
	}

	inline void FGraphPartitioner::AddLocalityLinks(FGraphData* Graph, uint32 Index, idx_t Cost)
	{
		auto range = LocalityLinks.equal_range(Index);
		for (auto Iter = range.first; Iter != range.second; ++Iter)
		{
			uint32 AdjIndex = Iter->second;
			Graph->Adjacency.emplace_back(SortedTo[AdjIndex]);
			Graph->AdjacencyCost.emplace_back(Cost);
		}
	}

	template< typename FGetCenter >
	void FGraphPartitioner::BuildLocalityLinks(white::FDisjointSet& DisjointSet, const FBounds& Bounds, FGetCenter& GetCenter)
	{
		std::vector< uint32 > SortKeys;
		SortKeys.resize(NumElements);
		SortedTo.resize(NumElements);

		const bool bSingleThreaded = NumElements < 5000;
		ParallelFor(NumElements,
			[&](uint32 Index)
			{
				auto Center = GetCenter(Index);
				auto CenterLocal = (Center - Bounds.Min) / (Bounds.Max - Bounds.Min);

				uint32 Morton;
				Morton = wm::MortonCode3(CenterLocal.x * 1023);
				Morton |= wm::MortonCode3(CenterLocal.y * 1023) << 1;
				Morton |= wm::MortonCode3(CenterLocal.z * 1023) << 2;
				SortKeys[Index] = Morton;
			}, bSingleThreaded?ParallelForFlags::SingleThread:ParallelForFlags::None);

		RadixSort32(SortedTo.data(), Indexes.data(), NumElements,
			[&](uint32 Index)
			{
				return SortKeys[Index];
			});

		SortKeys.clear();

		std::swap(Indexes, SortedTo);
		for (uint32 i = 0; i < NumElements; i++)
		{
			SortedTo[Indexes[i]] = i;
		}

		std::vector< FRange > IslandRuns;
		IslandRuns.resize(NumElements);

		// Run length acceleration
		// Range of identical IslandID denoting that elements are connected.
		// Used for jumping past connected elements to the next nearby disjoint element.
		{
			uint32 RunIslandID = 0;
			uint32 RunFirstElement = 0;

			for (uint32 i = 0; i < NumElements; i++)
			{
				uint32 IslandID = DisjointSet.Find(Indexes[i]);

				if (RunIslandID != IslandID)
				{
					// We found the end so rewind to the beginning of the run and fill.
					for (uint32 j = RunFirstElement; j < i; j++)
					{
						IslandRuns[j].End = i - 1;
					}

					// Start the next run
					RunIslandID = IslandID;
					RunFirstElement = i;
				}

				IslandRuns[i].Begin = RunFirstElement;
			}
			// Finish the last run
			for (uint32 j = RunFirstElement; j < NumElements; j++)
			{
				IslandRuns[j].End = NumElements - 1;
			}
		}

		for (uint32 i = 0; i < NumElements; i++)
		{
			uint32 Index = Indexes[i];

			uint32 RunLength = IslandRuns[i].End - IslandRuns[i].Begin + 1;
			if (RunLength < 128)
			{
				uint32 IslandID = DisjointSet[Index];

				auto Center = GetCenter(Index);

				uint32 ClosestIndex[3] = { ~0u, ~0u, ~0u };
				float  ClosestDist2[3] = { wm::MAX_flt, wm::MAX_flt, wm::MAX_flt };

				for (int Direction = 0; Direction < 2; Direction++)
				{
					uint32 Limit = Direction ? NumElements - 1 : 0;
					uint32 Step = Direction ? 1 : -1;

					uint32 Adj = i;
					for (int32 Iterations = 0; Iterations < 16; Iterations++)
					{
						if (Adj == Limit)
							break;
						Adj += Step;

						uint32 AdjIndex = Indexes[Adj];
						uint32 AdjIslandID = DisjointSet[AdjIndex];
						if (IslandID == AdjIslandID)
						{
							// Skip past this run
							if (Direction)
								Adj = IslandRuns[Adj].End;
							else
								Adj = IslandRuns[Adj].Begin;
						}
						else
						{
							// Add to sorted list
							float AdjDist2 = wm::length_sq(Center - GetCenter(AdjIndex));
							for (int k = 0; k < 3; k++)
							{
								if (AdjDist2 < ClosestDist2[k])
								{
									std::swap(AdjIndex, ClosestIndex[k]);
									std::swap(AdjDist2, ClosestDist2[k]);
								}
							}
						}
					}
				}

				for (int k = 0; k < 3; k++)
				{
					if (ClosestIndex[k] != ~0u)
					{
						/**
						* Add a key-value association to the map.  The association doesn't replace any of the key's existing associations.
						 * However, if both the key and value match an existing association in the map, no new association is made and the existing association's
						 * value is returned.*/
						auto add_unique = [&](uint32 key, uint32 value)
						{
							auto range = LocalityLinks.equal_range(key);
							for (auto itr = range.first; itr != range.second; ++itr)
								if (itr->second == value)
									return;
							LocalityLinks.emplace(key, value);
						};
						// Add both directions
						add_unique(Index, ClosestIndex[k]);
						add_unique(ClosestIndex[k], Index);
					}
				}
			}
		}
	}
}


