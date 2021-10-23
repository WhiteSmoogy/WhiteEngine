#pragma once

#include "Cluster.h"
#include "ClusterDAG.h"

namespace Nanite
{
	void Encode(Resources& Resources, const Settings& Settings, std::vector< FCluster >& Clusters,
		std::vector< FClusterGroup >& Groups, const FBounds& MeshBounds,
		uint32 NumMeshes,
		uint32 NumTexCoords,
		bool bHasColors);
}