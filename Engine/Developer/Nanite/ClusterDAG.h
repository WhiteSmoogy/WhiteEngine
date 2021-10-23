// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Cluster.h"

namespace Nanite
{

struct FClusterGroup
{
	Sphere				Bounds;
	Sphere				LODBounds;
	float				MinLODError;
	float				MaxParentLODError;
	int32				MipLevel;
	uint32				MeshIndex;
	
	uint32				PageIndexStart;
	uint32				PageIndexNum;
	std::vector< uint32 >	Children;

	template<typename FArchive>
	friend FArchive& operator<<(FArchive& Ar, FClusterGroup& Group);
};

// Performs DAG reduction and appends the resulting clusters and groups
void BuildDAG(std::vector< FClusterGroup >& Groups, std::vector< FCluster >& Cluster, uint32 ClusterBaseStart, uint32 ClusterBaseNum, uint32 MeshIndex, FBounds& MeshBounds );

FCluster FindDAGCut(
	const std::vector< FClusterGroup >& Groups,
	const std::vector< FCluster >& Clusters,
	uint32 TargetNumTris );

} // namespace Nanite