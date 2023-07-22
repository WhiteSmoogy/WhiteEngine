#include "GraphPartitioner.h"
#include <coroutine>
#include "Core/Coroutine/SyncWait.h"
#include "System/SystemEnvironment.h"

namespace WhiteEngine
{
	FGraphPartitioner::FGraphPartitioner(uint32 InNumElements)
		: NumElements(InNumElements)
	{
		Indexes.resize(NumElements);
		for (uint32 i = 0; i < NumElements; i++)
		{
			Indexes[i] = i;
		}
	}

	FGraphPartitioner::FGraphData* FGraphPartitioner::NewGraph(uint32 NumAdjacency) const
	{
		NumAdjacency +=static_cast<uint32>(LocalityLinks.size());

		FGraphData* wrestrict Graph = new FGraphPartitioner::FGraphData;
		Graph->Offset = 0;
		Graph->Num = NumElements;
		Graph->Adjacency.reserve(NumAdjacency);
		Graph->AdjacencyCost.reserve(NumAdjacency);
		Graph->AdjacencyOffset.resize(NumElements + 1);
		return Graph;
	}

	void FGraphPartitioner::Partition(FGraphData* Graph, int32 InMinPartitionSize, int32 InMaxPartitionSize)
	{
		MinPartitionSize = InMinPartitionSize;
		MaxPartitionSize = InMaxPartitionSize;

		const int32 TargetPartitionSize = (MinPartitionSize + MaxPartitionSize) / 2;
		const int32 TargetNumPartitions = wm::DivideAndRoundUp(Graph->Num, TargetPartitionSize);

		if (TargetNumPartitions > 1)
		{
			PartitionIDs.resize(PartitionIDs.size() + NumElements);

			idx_t NumConstraints = 1;
			idx_t NumParts = TargetNumPartitions;
			idx_t EdgesCut = 0;

			idx_t Options[METIS_NOPTIONS];
			METIS_SetDefaultOptions(Options);

			Options[METIS_OPTION_UFACTOR] = 200;//( 1000 * MaxPartitionSize * TargetNumPartitions ) / NumElements - 1000;
			//Options[ METIS_OPTION_NCUTS ] = 8;
			//Options[ METIS_OPTION_IPTYPE ] = METIS_IPTYPE_RANDOM;
			//Options[ METIS_OPTION_SEED ] = 17;

			//int r = METIS_PartGraphRecursive(
			int r = METIS_PartGraphKway(
				&Graph->Num,
				&NumConstraints,			// number of balancing constraints
				Graph->AdjacencyOffset.data(),
				Graph->Adjacency.data(),
				NULL,						// Vert weights
				NULL,						// Vert sizes for computing the total communication volume
				Graph->AdjacencyCost.data(),	// Edge weights
				&NumParts,
				NULL,						// Target partition weight
				NULL,						// Allowed load imbalance tolerance
				Options,
				&EdgesCut,
				PartitionIDs.data()
			);
			wconstraint(r == METIS_OK);
			if (r == METIS_OK)
			{
				std::vector< uint32 > ElementCount;
				ElementCount.resize(TargetNumPartitions);

				for (uint32 i = 0; i < NumElements; i++)
				{
					ElementCount[PartitionIDs[i]]++;
				}

				uint32 Begin = 0;
				Ranges.resize(Ranges.size() + TargetNumPartitions);
				for (int32 PartitionIndex = 0; PartitionIndex < TargetNumPartitions; PartitionIndex++)
				{
					Ranges[PartitionIndex] = { Begin, Begin + ElementCount[PartitionIndex] };
					Begin += ElementCount[PartitionIndex];
					ElementCount[PartitionIndex] = 0;
				}

				std::vector< uint32 > OldIndexes;
				std::swap(Indexes, OldIndexes);

				Indexes.resize(Indexes.size() + NumElements);
				for (uint32 i = 0; i < NumElements; i++)
				{
					uint32 PartitionIndex = PartitionIDs[i];
					uint32 Offset = Ranges[PartitionIndex].Begin;
					uint32 Num = ElementCount[PartitionIndex]++;

					Indexes[Offset + Num] = OldIndexes[i];
				}

				PartitionIDs.clear();
			}
		}
		else
		{
			// Single
			Ranges.emplace_back(0, NumElements);
		}
	}

	void FGraphPartitioner::BisectGraph(FGraphData* Graph, FGraphData* ChildGraphs[2])
	{
		ChildGraphs[0] = nullptr;
		ChildGraphs[1] = nullptr;

		auto AddPartition =
			[this](int32 Offset, int32 Num)
		{
			FRange& Range = Ranges[NumPartitions++];
			Range.Begin = Offset;
			Range.End = Offset + Num;
		};

		if (Graph->Num <= MaxPartitionSize)
		{
			AddPartition(Graph->Offset, Graph->Num);
			return;
		}

		const int32 TargetPartitionSize = (MinPartitionSize + MaxPartitionSize) / 2;
		const int32 TargetNumPartitions = std::max(2, wm::DivideAndRoundNearest(Graph->Num, TargetPartitionSize));

		wconstraint(Graph->AdjacencyOffset.size() == Graph->Num + 1);

		idx_t NumConstraints = 1;
		idx_t NumParts = 2;
		idx_t EdgesCut = 0;

		real_t PartitionWeights[] = {
			float(TargetNumPartitions / 2) / TargetNumPartitions,
			1.0f - float(TargetNumPartitions / 2) / TargetNumPartitions
		};

		idx_t Options[METIS_NOPTIONS];
		METIS_SetDefaultOptions(Options);

		// Allow looser tolerance when at the higher levels. Strict balance isn't that important until it gets closer to partition sized.
		bool bLoose = TargetNumPartitions >= 128 || MaxPartitionSize / MinPartitionSize > 1;
		bool bSlow = Graph->Num < 4096;

		Options[METIS_OPTION_UFACTOR] = bLoose ? 200 : 1;
		//Options[ METIS_OPTION_NCUTS ] = Graph->Num < 1024 ? 8 : ( Graph->Num < 4096 ? 4 : 1 );
		//Options[ METIS_OPTION_NCUTS ] = bSlow ? 4 : 1;
		//Options[ METIS_OPTION_NITER ] = bSlow ? 20 : 10;
		//Options[ METIS_OPTION_IPTYPE ] = METIS_IPTYPE_RANDOM;
		//Options[ METIS_OPTION_MINCONN ] = 1;

		int r = METIS_PartGraphRecursive(
			&Graph->Num,
			&NumConstraints,			// number of balancing constraints
			Graph->AdjacencyOffset.data(),
			Graph->Adjacency.data(),
			NULL,						// Vert weights
			NULL,						// Vert sizes for computing the total communication volume
			Graph->AdjacencyCost.data(),	// Edge weights
			&NumParts,
			PartitionWeights,			// Target partition weight
			NULL,						// Allowed load imbalance tolerance
			Options,
			&EdgesCut,
			PartitionIDs.data() + Graph->Offset
		);

		wconstraint(r == METIS_OK);
		if (r == METIS_OK)
		{
			// In place divide the array
			// Both sides remain sorted but back is reversed.
			int32 Front = Graph->Offset;
			int32 Back = Graph->Offset + Graph->Num - 1;
			while (Front <= Back)
			{
				while (Front <= Back && PartitionIDs[Front] == 0)
				{
					SwappedWith[Front] = Front;
					Front++;
				}
				while (Front <= Back && PartitionIDs[Back] == 1)
				{
					SwappedWith[Back] = Back;
					Back--;
				}

				if (Front < Back)
				{
					std::swap(Indexes[Front], Indexes[Back]);

					SwappedWith[Front] = Back;
					SwappedWith[Back] = Front;
					Front++;
					Back--;
				}
			}

			int32 Split = Front;

			int32 Num[2];
			Num[0] = Split - Graph->Offset;
			Num[1] = Graph->Offset + Graph->Num - Split;

			wconstraint(Num[0] > 1);
			wconstraint(Num[1] > 1);

			if (Num[0] <= MaxPartitionSize && Num[1] <= MaxPartitionSize)
			{
				AddPartition(Graph->Offset, Num[0]);
				AddPartition(Split, Num[1]);
			}
			else
			{
				for (int32 i = 0; i < 2; i++)
				{
					ChildGraphs[i] = new FGraphData;
					ChildGraphs[i]->Adjacency.reserve(Graph->Adjacency.size() >> 1);
					ChildGraphs[i]->AdjacencyCost.reserve(Graph->Adjacency.size() >> 1);
					ChildGraphs[i]->AdjacencyOffset.reserve(Num[i] + 1);
					ChildGraphs[i]->Num = Num[i];
				}

				ChildGraphs[0]->Offset = Graph->Offset;
				ChildGraphs[1]->Offset = Split;

				for (int32 i = 0; i < Graph->Num; i++)
				{
					FGraphData* ChildGraph = ChildGraphs[i >= ChildGraphs[0]->Num];

					ChildGraph->AdjacencyOffset.emplace_back(static_cast<idx_t>(ChildGraph->Adjacency.size()));

					int32 OrgIndex = SwappedWith[Graph->Offset + i] - Graph->Offset;
					for (idx_t AdjIndex = Graph->AdjacencyOffset[OrgIndex]; AdjIndex < Graph->AdjacencyOffset[OrgIndex + 1]; AdjIndex++)
					{
						idx_t Adj = Graph->Adjacency[AdjIndex];
						idx_t AdjCost = Graph->AdjacencyCost[AdjIndex];

						// Remap to child
						Adj = SwappedWith[Graph->Offset + Adj] - ChildGraph->Offset;

						// Edge connects to node in this graph
						if (0 <= Adj && Adj < ChildGraph->Num)
						{
							ChildGraph->Adjacency.emplace_back(Adj);
							ChildGraph->AdjacencyCost.emplace_back(AdjCost);
						}
					}
				}
				ChildGraphs[0]->AdjacencyOffset.emplace_back(static_cast<idx_t>(ChildGraphs[0]->Adjacency.size()));
				ChildGraphs[1]->AdjacencyOffset.emplace_back(static_cast<idx_t>(ChildGraphs[1]->Adjacency.size()));
			}
		}
	}

	void FGraphPartitioner::RecursiveBisectGraph(FGraphData* Graph)
	{
		FGraphData* ChildGraphs[2];
		BisectGraph(Graph, ChildGraphs);
		delete Graph;

		if (ChildGraphs[0] && ChildGraphs[1])
		{
			RecursiveBisectGraph(ChildGraphs[0]);
			RecursiveBisectGraph(ChildGraphs[1]);
		}
	}

	void FGraphPartitioner::PartitionStrict(FGraphData* Graph, int32 InMinPartitionSize, int32 InMaxPartitionSize, bool bThreaded)
	{
		MinPartitionSize = InMinPartitionSize;
		MaxPartitionSize = InMaxPartitionSize;

		PartitionIDs.resize(PartitionIDs.size() + NumElements);
		SwappedWith.resize(SwappedWith.size() + NumElements);

		// Adding to atomically so size big enough to not need to grow.
		int32 NumPartitionsExpected = wm::DivideAndRoundUp(Graph->Num, MinPartitionSize);
		Ranges.resize(Ranges.size() + NumPartitionsExpected * 2);
		NumPartitions = 0;

		if (bThreaded && NumPartitionsExpected > 4)
		{
			class FBuildTask
			{
			public:
				FBuildTask(FGraphPartitioner* InPartitioner, FGraphData* InGraph)
					: Partitioner(InPartitioner)
					, Graph(InGraph)
				{}

				struct ThreadJoinDeleter
				{
					void operator()(std::thread* thread)
					{
						thread->join();
						delete thread;
					}
				};

				void BisectGraph()
				{
					FGraphData* ChildGraphs[2];
					Partitioner->BisectGraph(Graph, ChildGraphs);
					delete Graph;

					std::unique_ptr<std::thread, ThreadJoinDeleter> wait_task;

					if (ChildGraphs[0] && ChildGraphs[1])
					{
						if (ChildGraphs[0]->Num > 256)
						{
							wait_task.reset(new std::thread([=] {
								FBuildTask(Partitioner, ChildGraphs[0]).BisectGraph();
								}));
						}
						else
						{
							FBuildTask(Partitioner, ChildGraphs[0]).BisectGraph();
						}

						FBuildTask(Partitioner, ChildGraphs[1]).BisectGraph();
					}
				}

			private:
				FGraphPartitioner* Partitioner;
				FGraphData* Graph;
			};

			FBuildTask(this, Graph).BisectGraph();
		}
		else
		{
			RecursiveBisectGraph(Graph);
		}

		Ranges.resize(NumPartitions);

		if (bThreaded)
		{
			// Force a deterministic order
			std::sort(Ranges.begin(), Ranges.end());
		}

		PartitionIDs.clear();
		SwappedWith.clear();
	}
}