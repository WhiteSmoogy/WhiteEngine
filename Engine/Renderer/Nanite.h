#pragma once
#include "CoreTypes.h"
#include "Math/Sphere.h"
#include "Core/Serialization/BulkData.h"
#include "Core/Serialization/Archive.h"
#include "spdlog/spdlog.h"
#include "spdlog/stopwatch.h"
#define USE_STRIP_INDICES	1	// must match define in NaniteDataDecode.h

namespace Nanite
{
	using namespace white::inttype;

	constexpr auto MAX_CLUSTER_TRIANGLES = 128;
	constexpr auto MAX_CLUSTER_VERTICES = 256;
	constexpr auto  NUM_ROOT_PAGES = 1u;										// Should probably be made a per-resource option
	constexpr auto MAX_CLUSTER_INDICES = (MAX_CLUSTER_TRIANGLES * 3);
	constexpr auto  MAX_NANITE_UVS = 4;											// must match define in NaniteDataDecode.ush

	constexpr auto  CLUSTER_PAGE_GPU_SIZE_BITS = 17;
	constexpr auto  CLUSTER_PAGE_GPU_SIZE = (1 << CLUSTER_PAGE_GPU_SIZE_BITS);
	constexpr auto  CLUSTER_PAGE_DISK_SIZE = (CLUSTER_PAGE_GPU_SIZE * 2);
	constexpr auto  MAX_CLUSTERS_PER_PAGE_BITS = 10;
	constexpr auto  MAX_CLUSTERS_PER_PAGE_MASK = ((1 << MAX_CLUSTERS_PER_PAGE_BITS) - 1);
	constexpr auto  MAX_CLUSTERS_PER_PAGE = (1 << MAX_CLUSTERS_PER_PAGE_BITS);
	constexpr auto MAX_CLUSTERS_PER_GROUP_BITS = 9;
	constexpr auto MAX_CLUSTERS_PER_GROUP_MASK = ((1 << MAX_CLUSTERS_PER_GROUP_BITS) - 1);
	constexpr auto  MAX_CLUSTERS_PER_GROUP = ((1 << MAX_CLUSTERS_PER_GROUP_BITS) - 1);

	constexpr auto  MAX_CLUSTERS_PER_GROUP_TARGET = 128;														// what we are targeting. MAX_CLUSTERS_PER_GROUP needs to be large 
	// enough that it won't overflow after constraint-based splitting

	constexpr auto MAX_HIERACHY_CHILDREN_BITS = 6;
	constexpr auto MAX_HIERACHY_CHILDREN = (1 << MAX_HIERACHY_CHILDREN_BITS);
	constexpr auto MAX_GPU_PAGES_BITS = 14;
	constexpr auto	MAX_GPU_PAGES = (1 << MAX_GPU_PAGES_BITS);
	constexpr auto MAX_GROUP_PARTS_BITS = 3;
	constexpr auto MAX_GROUP_PARTS_MASK = ((1 << MAX_GROUP_PARTS_BITS) - 1);
	constexpr auto MAX_GROUP_PARTS = (1 << MAX_GROUP_PARTS_BITS);

	constexpr auto PERSISTENT_CLUSTER_CULLING_GROUP_SIZE = 64;									// must match define in Culling.ush

	constexpr auto MAX_RESOURCE_PAGES_BITS = 20;
	constexpr auto MAX_RESOURCE_PAGES = (1 << MAX_RESOURCE_PAGES_BITS);
	constexpr auto MAX_BVH_NODE_FANOUT_BITS = 3;
	constexpr auto MAX_BVH_NODE_FANOUT = (1 << MAX_BVH_NODE_FANOUT_BITS);
	constexpr auto MAX_BVH_NODES_PER_GROUP = (PERSISTENT_CLUSTER_CULLING_GROUP_SIZE / MAX_BVH_NODE_FANOUT);


	constexpr auto  MAX_POSITION_QUANTIZATION_BITS = 21;		// (21*3 = 63) < 64								// must match define in NaniteDataDecode.ush
	constexpr auto NORMAL_QUANTIZATION_BITS = 9;

	constexpr auto VERTEX_COLOR_MODE_WHITE = 0;
	constexpr auto VERTEX_COLOR_MODE_CONSTANT = 1;
	constexpr auto VERTEX_COLOR_MODE_VARIABLE = 2;

	constexpr auto  NANITE_CLUSTER_FLAG_LEAF = 0x1;


	struct FMaterialTriangle
	{
		uint32 Index0;
		uint32 Index1;
		uint32 Index2;
		uint32 MaterialIndex;
		uint32 RangeCount;
	};

	inline uint32 GetBits(uint32 Value, uint32 NumBits, uint32 Offset)
	{
		uint32 Mask = (1u << NumBits) - 1u;
		return (Value >> Offset) & Mask;
	}

	inline void SetBits(uint32& Value, uint32 Bits, uint32 NumBits, uint32 Offset)
	{
		uint32 Mask = (1u << NumBits) - 1u;
		wconstraint(Bits <= Mask);
		Mask <<= Offset;
		Value = (Value & ~Mask) | (Bits << Offset);
	}

	// Packed Cluster as it is used by the GPU
	struct FPackedCluster
	{
		// Members needed for rasterization
		wm::int3	QuantizedPosStart;
		uint32		NumVerts_PositionOffset;					// NumVerts:9, PositionOffset:23

		wm::float3		MeshBoundsMin;
		uint32		NumTris_IndexOffset;						// NumTris:8, IndexOffset: 24

		wm::float3		MeshBoundsDelta;
		uint32		BitsPerIndex_QuantizedPosShift_PosBits;		// BitsPerIndex:4, QuantizedPosShift:6, QuantizedPosBits:5.5.5

		// Members needed for culling
		WhiteEngine::Sphere		LODBounds;

		wm::float3		BoxBoundsCenter;
		uint32		LODErrorAndEdgeLength;

		wm::float3		BoxBoundsExtent;
		uint32		Flags;

		// Members needed by materials
		uint32		AttributeOffset_BitsPerAttribute;			// AttributeOffset: 22, BitsPerAttribute: 10
		uint32		DecodeInfoOffset_NumUVs_ColorMode;			// DecodeInfoOffset: 22, NumUVs: 3, ColorMode: 2
		uint32		UV_Prec;									// U0:4, V0:4, U1:4, V1:4, U2:4, V2:4, U3:4, V3:4
		uint32		PackedMaterialInfo;

		uint32		ColorMin;
		uint32		ColorBits;									// R:4, G:4, B:4, A:4
		uint32		GroupIndex;									// Debug only
		uint32		Pad0;

		uint32		GetNumVerts() const { return GetBits(NumVerts_PositionOffset, 9, 0); }
		uint32		GetPositionOffset() const { return GetBits(NumVerts_PositionOffset, 23, 9); }

		uint32		GetNumTris() const { return GetBits(NumTris_IndexOffset, 8, 0); }
		uint32		GetIndexOffset() const { return GetBits(NumTris_IndexOffset, 24, 8); }

		uint32		GetBitsPerIndex() const { return GetBits(BitsPerIndex_QuantizedPosShift_PosBits, 4, 0); }
		uint32		GetQuantizedPosShift() const { return GetBits(BitsPerIndex_QuantizedPosShift_PosBits, 6, 4); }
		uint32		GetPosBitsX() const { return GetBits(BitsPerIndex_QuantizedPosShift_PosBits, 5, 10); }
		uint32		GetPosBitsY() const { return GetBits(BitsPerIndex_QuantizedPosShift_PosBits, 5, 15); }
		uint32		GetPosBitsZ() const { return GetBits(BitsPerIndex_QuantizedPosShift_PosBits, 5, 20); }

		uint32		GetAttributeOffset() const { return GetBits(AttributeOffset_BitsPerAttribute, 22, 0); }
		uint32		GetBitsPerAttribute() const { return GetBits(AttributeOffset_BitsPerAttribute, 10, 22); }

		void		SetNumVerts(uint32 NumVerts) { SetBits(NumVerts_PositionOffset, NumVerts, 9, 0); }
		void		SetPositionOffset(uint32 Offset) { SetBits(NumVerts_PositionOffset, Offset, 23, 9); }

		void		SetNumTris(uint32 NumTris) { SetBits(NumTris_IndexOffset, NumTris, 8, 0); }
		void		SetIndexOffset(uint32 Offset) { SetBits(NumTris_IndexOffset, Offset, 24, 8); }

		void		SetBitsPerIndex(uint32 BitsPerIndex) { SetBits(BitsPerIndex_QuantizedPosShift_PosBits, BitsPerIndex, 4, 0); }
		void		SetQuantizedPosShift(uint32 PosShift) { SetBits(BitsPerIndex_QuantizedPosShift_PosBits, PosShift, 6, 4); }
		void		SetPosBitsX(uint32 NumBits) { SetBits(BitsPerIndex_QuantizedPosShift_PosBits, NumBits, 5, 10); }
		void		SetPosBitsY(uint32 NumBits) { SetBits(BitsPerIndex_QuantizedPosShift_PosBits, NumBits, 5, 15); }
		void		SetPosBitsZ(uint32 NumBits) { SetBits(BitsPerIndex_QuantizedPosShift_PosBits, NumBits, 5, 20); }

		void		SetAttributeOffset(uint32 Offset) { SetBits(AttributeOffset_BitsPerAttribute, Offset, 22, 0); }
		void		SetBitsPerAttribute(uint32 Bits) { SetBits(AttributeOffset_BitsPerAttribute, Bits, 10, 22); }

		void		SetDecodeInfoOffset(uint32 Offset) { SetBits(DecodeInfoOffset_NumUVs_ColorMode, Offset, 22, 0); }
		void		SetNumUVs(uint32 Num) { SetBits(DecodeInfoOffset_NumUVs_ColorMode, Num, 3, 22); }
		void		SetColorMode(uint32 Mode) { SetBits(DecodeInfoOffset_NumUVs_ColorMode, Mode, 2, 22 + 3); }
	};

	struct FPackedHierarchyNode
	{
		WhiteEngine::Sphere		LODBounds[MAX_BVH_NODE_FANOUT];

		struct
		{
			wm::float3		BoxBoundsCenter;
			uint32		MinLODError_MaxParentLODError;
		} Misc0[MAX_BVH_NODE_FANOUT];

		struct
		{
			wm::float3		BoxBoundsExtent;
			uint32		ChildStartReference;
		} Misc1[MAX_BVH_NODE_FANOUT];

		struct
		{
			uint32		ResourcePageIndex_NumPages_GroupPartSize;
		} Misc2[MAX_BVH_NODE_FANOUT];
	};

	struct FPageStreamingState
	{
		uint32			BulkOffset;
		uint32			BulkSize;
		uint32			PageUncompressedSize;
		uint32			DependenciesStart;
		uint32			DependenciesNum;
	};

	class FHierarchyFixup
	{
	public:
		FHierarchyFixup() {}

		FHierarchyFixup(uint32 InPageIndex, uint32 NodeIndex, uint32 ChildIndex, uint32 InClusterGroupPartStartIndex, uint32 PageDependencyStart, uint32 PageDependencyNum)
		{
			wconstraint(InPageIndex < MAX_RESOURCE_PAGES);
			PageIndex = InPageIndex;

			wconstraint(NodeIndex < (1 << (32 - MAX_HIERACHY_CHILDREN_BITS)));
			wconstraint(ChildIndex < MAX_HIERACHY_CHILDREN);
			wconstraint(InClusterGroupPartStartIndex < (1 << (32 - MAX_CLUSTERS_PER_GROUP_BITS)));
			HierarchyNodeAndChildIndex = (NodeIndex << MAX_HIERACHY_CHILDREN_BITS) | ChildIndex;
			ClusterGroupPartStartIndex = InClusterGroupPartStartIndex;

			wconstraint(PageDependencyStart < MAX_RESOURCE_PAGES);
			wconstraint(PageDependencyNum <= MAX_GROUP_PARTS_MASK);
			PageDependencyStartAndNum = (PageDependencyStart << MAX_GROUP_PARTS_BITS) | PageDependencyNum;
		}

		uint32 GetPageIndex() const { return PageIndex; }
		uint32 GetNodeIndex() const { return HierarchyNodeAndChildIndex >> MAX_HIERACHY_CHILDREN_BITS; }
		uint32 GetChildIndex() const { return HierarchyNodeAndChildIndex & (MAX_HIERACHY_CHILDREN - 1); }
		uint32 GetClusterGroupPartStartIndex() const { return ClusterGroupPartStartIndex; }
		uint32 GetPageDependencyStart() const { return PageDependencyStartAndNum >> MAX_GROUP_PARTS_BITS; }
		uint32 GetPageDependencyNum() const { return PageDependencyStartAndNum & MAX_GROUP_PARTS_MASK; }

		uint32 PageIndex;
		uint32 HierarchyNodeAndChildIndex;
		uint32 ClusterGroupPartStartIndex;
		uint32 PageDependencyStartAndNum;
	};

	class FClusterFixup
	{
	public:
		FClusterFixup() {}

		FClusterFixup(uint32 PageIndex, uint32 ClusterIndex, uint32 PageDependencyStart, uint32 PageDependencyNum)
		{
			wconstraint(PageIndex < (1 << (32 - MAX_CLUSTERS_PER_GROUP_BITS)));
			wconstraint(ClusterIndex < MAX_CLUSTERS_PER_PAGE);
			PageAndClusterIndex = (PageIndex << MAX_CLUSTERS_PER_PAGE_BITS) | ClusterIndex;

			wconstraint(PageDependencyStart < MAX_RESOURCE_PAGES);
			wconstraint(PageDependencyNum <= MAX_GROUP_PARTS_MASK);
			PageDependencyStartAndNum = (PageDependencyStart << MAX_GROUP_PARTS_BITS) | PageDependencyNum;
		}

		uint32 GetPageIndex() const { return PageAndClusterIndex >> MAX_CLUSTERS_PER_PAGE_BITS; }
		uint32 GetClusterIndex() const { return PageAndClusterIndex & (MAX_CLUSTERS_PER_PAGE - 1u); }
		uint32 GetPageDependencyStart() const { return PageDependencyStartAndNum >> MAX_GROUP_PARTS_BITS; }
		uint32 GetPageDependencyNum() const { return PageDependencyStartAndNum & MAX_GROUP_PARTS_MASK; }

		uint32 PageAndClusterIndex;
		uint32 PageDependencyStartAndNum;
	};

	struct FPageDiskHeader
	{
		uint32 GpuSize;
		uint32 NumClusters;
		uint32 NumRawFloat4s;
		uint32 NumTexCoords;
		uint32 DecodeInfoOffset;
		uint32 StripBitmaskOffset;
		uint32 VertexRefBitmaskOffset;
	};

	struct FClusterDiskHeader
	{
		uint32 IndexDataOffset;
		uint32 VertexRefDataOffset;
		uint32 PositionDataOffset;
		uint32 AttributeDataOffset;
		uint32 NumPrevRefVerticesBeforeDwords;
		uint32 NumPrevNewVerticesBeforeDwords;
	};

	class FFixupChunk	//TODO: rename to something else
	{
	public:
		struct FHeader
		{
			uint16 NumClusters = 0;
			uint16 NumHierachyFixups = 0;
			uint16 NumClusterFixups = 0;
			uint16 Pad = 0;
		} Header;

		uint8 Data[sizeof(FHierarchyFixup) * MAX_CLUSTERS_PER_PAGE + sizeof(FClusterFixup) * MAX_CLUSTERS_PER_PAGE];	// One hierarchy fixup per cluster and at most one cluster fixup per cluster.

		FClusterFixup& GetClusterFixup(uint32 Index) const { wconstraint(Index < Header.NumClusterFixups);  return ((FClusterFixup*)(Data + Header.NumHierachyFixups * sizeof(FHierarchyFixup)))[Index]; }
		FHierarchyFixup& GetHierarchyFixup(uint32 Index) const { wconstraint(Index < Header.NumHierachyFixups); return ((FHierarchyFixup*)Data)[Index]; }
		uint32				GetSize() const { return sizeof(Header) + Header.NumHierachyFixups * sizeof(FHierarchyFixup) + Header.NumClusterFixups * sizeof(FClusterFixup); }
	};


	class Resources
	{
	public:
		// Persistent State
		std::vector< uint8 >					RootClusterPage;		// Root page is loaded on resource load, so we always have something to draw.
		WhiteEngine::ByteBulkData					StreamableClusterPages;			// Remaining pages are streamed on demand.

		std::vector< uint32 >				HierarchyRootOffsets;
		std::vector< FPackedHierarchyNode >	HierarchyNodes;

		std::vector< FPageStreamingState >	PageStreamingStates;
		std::vector< uint32 >				PageDependencies;

		int32 PositionPrecision;

		bool	bLZCompressed = false;
	};

	struct Settings
	{
		int32 PositionPrecision = std::numeric_limits<int32>::min();
	};

	namespace detail
	{
		class Trace
		{
		public:
			Trace(const char* name)
				:post_name(name)
			{}

			~Trace()
			{
				spdlog::info("Nanite {}: {} seconds", post_name, sw);
			}
		private:
			std::string post_name;
			spdlog::stopwatch sw;
		};
	}

}

#define NANITE_TRACE(x) Nanite::detail::Trace WPP_Join(trace_,__LINE__){#x}