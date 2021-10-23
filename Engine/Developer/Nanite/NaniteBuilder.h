#pragma once

#include "Renderer/Nanite.h"

namespace Nanite
{
	struct FStaticMeshBuildVertex;

	void Build(
		Resources& Resources,
		std::vector<FStaticMeshBuildVertex>& Vertices, // TODO: Do not require this vertex type for all users of Nanite
		std::vector<uint32>& TriangleIndices,
		std::vector<int32>& MaterialIndices,
		std::vector<uint32>& MeshTriangleCounts,
		uint32 NumTexCoords,
		const Settings& Settings);
}