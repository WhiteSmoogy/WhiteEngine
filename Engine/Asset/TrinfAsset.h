#pragma once

#include "DStorageAsset.h"
#include "FileFormat.h"

#include <WBase/wmath.hpp>

namespace platform
{
	class TriinfMesh;
}

namespace platform_ex
{
	namespace DSFileFormat
	{
		enum class TrinfVersion : uint32
		{
			ForgeGeometry
		};

		struct ClusterCompact
		{
			uint32 TriangleCount;
			uint32 ClusterStart;
		};

		struct Cluster
		{
			white::math::float3 AABBMin;
			white::math::float3 AABBMax;
		};

		struct TriinfMetadata
		{
			OffsetPtr<char> Name;

			OffsetArray<ClusterCompact> ClusterCompacts;
			uint32 ClusterCount;
			OffsetArray< Cluster> Clusters;
		};

		struct TrinfGridHeader
		{
			uint32 StagingBufferSize;

			OffsetArray<TriinfMetadata> Trinfs;
			uint32 TrinfsCount;

			GpuRegion Position;
			GpuRegion Tangent;
			GpuRegion TexCoord;

			OffsetArray<GpuRegion> AdditioalVB;
			uint32 AdditioalVBCount;

			GpuRegion Index;
		};

		struct TrinfHeader
		{
			uint32 Signature = DSTrinf_Signature;
			TrinfVersion Version = TrinfVersion::ForgeGeometry;

			Region<TrinfGridHeader> CpuMedadata;
		};
	}
}