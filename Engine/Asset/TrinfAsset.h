#pragma once

#include "DStorageAsset.h"
#include "FileFormat.h"

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

		struct MeshGroupHeader
		{

		};

		struct TrinfHeader
		{
			uint32 Signature = DSTrinf_Signature;
			TrinfVersion Version = TrinfVersion::ForgeGeometry;

			Region<MeshGroupHeader> CpuMedadata;
		};
	}
}