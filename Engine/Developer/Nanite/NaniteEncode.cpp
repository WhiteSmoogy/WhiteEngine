#if defined(_MSC_VER) && !defined(__clang__)
#pragma warning(disable:4267)
#endif

#include "NaniteEncode.h"
#include "Runtime/Core/ParallelFor.h"
#include "Runtime/Core/Hash/CityHash.h"
#include "Runtime/Core/Compression.h"
#include "Runtime/Core/Container/map.hpp"
#include <WBase/wmemory.hpp>
using namespace white::inttype;
using white::uint32;

constexpr auto CONSTRAINED_CLUSTER_CACHE_SIZE = 32;
constexpr auto MAX_CLUSTER_MATERIALS = 64;

constexpr auto INVALID_PART_INDEX = 0xFFFFFFFFu;
constexpr auto INVALID_GROUP_INDEX = 0xFFFFFFFFu;
constexpr auto INVALID_PAGE_INDEX = 0xFFFFFFFFu;

namespace Nanite
{
	using WhiteEngine::ParallelFor;
	using white::Align;

	struct FClusterGroupPart					// Whole group or a part of a group that has been split.
	{
		std::vector<uint32>	Clusters;				// Can be reordered during page allocation, so we need to store a list here.
		FBounds			Bounds;
		uint32			PageIndex;
		uint32			GroupIndex;				// Index of group this is a part of.
		uint32			HierarchyNodeIndex;
		uint32			HierarchyChildIndex;
		uint32			PageClusterOffset;
	};

	struct FPageSections
	{
		uint32 Cluster = 0;
		uint32 MaterialTable = 0;
		uint32 DecodeInfo = 0;
		uint32 Index = 0;
		uint32 Position = 0;
		uint32 Attribute = 0;

		uint32 GetMaterialTableSize() const { return Align(MaterialTable, 16); }

		uint32 GetClusterOffset() const { return 0; }
		uint32 GetMaterialTableOffset() const { return Cluster; }
		uint32 GetDecodeInfoOffset() const { return Cluster + GetMaterialTableSize(); }
		uint32 GetIndexOffset() const { return Cluster + GetMaterialTableSize() + DecodeInfo; }
		uint32 GetPositionOffset() const { return Cluster + GetMaterialTableSize() + DecodeInfo + Index; }
		uint32 GetAttributeOffset() const { return Cluster + GetMaterialTableSize() + DecodeInfo + Index + Position; }
		uint32 GetTotal() const { return Cluster + GetMaterialTableSize() + DecodeInfo + Index + Position + Attribute; }

		FPageSections GetOffsets() const
		{
			return FPageSections{ GetClusterOffset(), GetMaterialTableOffset(), GetDecodeInfoOffset(), GetIndexOffset(), GetPositionOffset(), GetAttributeOffset() };
		}

		void operator+=(const FPageSections& Other)
		{
			Cluster += Other.Cluster;
			MaterialTable += Other.MaterialTable;
			DecodeInfo += Other.DecodeInfo;
			Index += Other.Index;
			Position += Other.Position;
			Attribute += Other.Attribute;
		}
	};

	struct FPage
	{
		uint32	PartsStartIndex = 0;
		uint32	PartsNum = 0;
		uint32	NumClusters = 0;

		FPageSections	GpuSizes;
	};

	struct FUVRange
	{
		wm::float2	Min;
		wm::float2	Scale;
		int32		GapStart[2];
		int32		GapLength[2];
	};

	struct FGeometryEncodingUVInfo
	{
		FUVRange	UVRange;
		wm::float2	UVDelta;
		wm::float2	UVRcpDelta;
		uint32		NU, NV;
	};

	struct FEncodingInfo
	{
		uint32 BitsPerIndex;
		uint32 BitsPerAttribute;
		uint32 UVPrec;

		uint32		ColorMode;
		wm::int4 ColorMin;
		wm::int4 ColorBits;

		FPageSections GpuSizes;

		FGeometryEncodingUVInfo UVInfos[MAX_NANITE_UVS];
	};

	// Naive bit writer for cooking purposes
	class FBitWriter
	{
	public:
		FBitWriter(std::vector<uint8>& Buffer) :
			Buffer(Buffer),
			PendingBits(0ull),
			NumPendingBits(0)
		{
		}

		void PutBits(uint32 Bits, uint32 NumBits)
		{
			wconstraint((uint64)Bits < (1ull << NumBits));
			PendingBits |= (uint64)Bits << NumPendingBits;
			NumPendingBits += NumBits;

			while (NumPendingBits >= 8)
			{
				Buffer.emplace_back((uint8)PendingBits);
				PendingBits >>= 8;
				NumPendingBits -= 8;
			}
		}

		void Flush(uint32 Alignment = 1)
		{
			if (NumPendingBits > 0)
				Buffer.emplace_back((uint8)PendingBits);
			while (Buffer.size() % Alignment != 0)
				Buffer.emplace_back(0);
			PendingBits = 0;
			NumPendingBits = 0;
		}

	private:
		std::vector<uint8>& Buffer;
		uint64 			PendingBits;
		int32 			NumPendingBits;
	};

	void BuildMaterialRanges(
		const std::vector<uint32>& TriangleIndices,
		const std::vector<int32>& MaterialIndices,
		std::vector<FMaterialTriangle>& MaterialTris,
		std::vector<FMaterialRange>& MaterialRanges)
	{
		wconstraint(MaterialTris.size() == 0);
		wconstraint(MaterialRanges.size() == 0);
		wconstraint(MaterialIndices.size() * 3 == TriangleIndices.size());

		const uint32 TriangleCount = MaterialIndices.size();

		std::vector<uint32> MaterialCounts;
		MaterialCounts.resize(64);

		// Tally up number tris per material index
		for (uint32 i = 0; i < TriangleCount; i++)
		{
			const uint32 MaterialIndex = MaterialIndices[i];
			++MaterialCounts[MaterialIndex];
		}

		for (uint32 i = 0; i < TriangleCount; i++)
		{
			FMaterialTriangle MaterialTri;
			MaterialTri.Index0 = TriangleIndices[(i * 3) + 0];
			MaterialTri.Index1 = TriangleIndices[(i * 3) + 1];
			MaterialTri.Index2 = TriangleIndices[(i * 3) + 2];
			MaterialTri.MaterialIndex = MaterialIndices[i];
			MaterialTri.RangeCount = MaterialCounts[MaterialTri.MaterialIndex];
			wconstraint(MaterialTri.RangeCount > 0);
			MaterialTris.emplace_back(MaterialTri);
		}

		// Sort by triangle range count descending, and material index ascending.
		// This groups the material ranges from largest to smallest, which is
		// more efficient for evaluating the sequences on the GPU, and also makes
		// the minus one encoding work (the first range must have more than 1 tri).
		std::sort(MaterialTris.begin(), MaterialTris.end(),
			[](const FMaterialTriangle& A, const FMaterialTriangle& B)
			{
				if (A.RangeCount != B.RangeCount)
				{
					return (A.RangeCount > B.RangeCount);
				}

				return (A.MaterialIndex < B.MaterialIndex);
			});

		FMaterialRange CurrentRange;
		CurrentRange.RangeStart = 0;
		CurrentRange.RangeLength = 0;
		CurrentRange.MaterialIndex = MaterialTris.size() > 0 ? MaterialTris[0].MaterialIndex : 0;

		for (int32 TriIndex = 0; TriIndex < MaterialTris.size(); ++TriIndex)
		{
			const FMaterialTriangle& Triangle = MaterialTris[TriIndex];

			// Material changed, so add current range and reset
			if (CurrentRange.RangeLength > 0 && Triangle.MaterialIndex != CurrentRange.MaterialIndex)
			{
				MaterialRanges.emplace_back(CurrentRange);

				CurrentRange.RangeStart = TriIndex;
				CurrentRange.RangeLength = 1;
				CurrentRange.MaterialIndex = Triangle.MaterialIndex;
			}
			else
			{
				++CurrentRange.RangeLength;
			}
		}

		// Add last triangle to range
		if (CurrentRange.RangeLength > 0)
		{
			MaterialRanges.emplace_back(CurrentRange);
		}

		wconstraint(MaterialTris.size() == TriangleCount);
	}

	void BuildMaterialRanges(FCluster& Cluster)
	{
		wconstraint(Cluster.MaterialRanges.size() == 0);
		wconstraint(Cluster.NumTris <= MAX_CLUSTER_TRIANGLES);
		wconstraint(Cluster.NumTris * 3 == Cluster.Indexes.size());

		std::vector<FMaterialTriangle> MaterialTris;

		BuildMaterialRanges(
			Cluster.Indexes,
			Cluster.MaterialIndexes,
			MaterialTris,
			Cluster.MaterialRanges);

		// Write indices back to clusters
		for (uint32 Triangle = 0; Triangle < Cluster.NumTris; ++Triangle)
		{
			Cluster.Indexes[Triangle * 3 + 0] = MaterialTris[Triangle].Index0;
			Cluster.Indexes[Triangle * 3 + 1] = MaterialTris[Triangle].Index1;
			Cluster.Indexes[Triangle * 3 + 2] = MaterialTris[Triangle].Index2;
			Cluster.MaterialIndexes[Triangle] = MaterialTris[Triangle].MaterialIndex;
		}
	}

	// Sort cluster triangles into material ranges. Add Material ranges to clusters.
	static void BuildMaterialRanges(std::vector<FCluster>& Clusters)
	{
		//const uint32 NumClusters = Clusters.Num();
		//for( uint32 ClusterIndex = 0; ClusterIndex < NumClusters; ClusterIndex++ )
		ParallelFor(Clusters.size(),
			[&](uint32 ClusterIndex)
			{
				BuildMaterialRanges(Clusters[ClusterIndex]);
			});
	}

	static uint32 SetCorner(uint32 Triangle, uint32 LocalCorner)
	{
		return (Triangle << 2) | LocalCorner;
	}

	static uint32 CornerToTriangle(uint32 Corner)
	{
		return Corner >> 2;
	}

	static uint32 NextCorner(uint32 Corner)
	{
		if ((Corner & 3) == 2)
			Corner &= ~3;
		else
			Corner++;
		return Corner;
	}

	static uint32 PrevCorner(uint32 Corner)
	{
		if ((Corner & 3) == 0)
			Corner |= 2;
		else
			Corner--;
		return Corner;
	}

	static uint32 CornerToIndex(uint32 Corner)
	{
		return (Corner >> 2) * 3 + (Corner & 3);
	}

	struct FStripifyWeights
	{
		int32 Weights[2][2][2][2][CONSTRAINED_CLUSTER_CACHE_SIZE];
	};

	static const FStripifyWeights DefaultStripifyWeights = {
		{
			{
				{
					{
						// IsStart=0, HasOpposite=0, HasLeft=0, HasRight=0
						{  142,  124,  131,  184,  138,  149,  148,  127,  154,  148,  152,  133,  133,  132,  170,  141,  109,  148,  138,  117,  126,  112,  144,  126,  116,  139,  122,  141,  122,  133,  134,  137 },
						// IsStart=0, HasOpposite=0, HasLeft=0, HasRight=1
						{  128,  144,  134,  122,  130,  133,  129,  122,  128,  107,  127,  126,   89,  135,   88,  130,   94,  134,  103,  118,  128,   96,   90,  139,   89,  139,  113,  100,  119,  131,  113,  121 },
					},
					{
						// IsStart=0, HasOpposite=0, HasLeft=1, HasRight=0
						{  128,  144,  134,  129,  110,  142,  111,  140,  116,  139,   98,  110,  125,  143,  122,  109,  127,  154,  113,  119,  126,  131,  123,  127,   93,  118,  101,   93,  131,  139,  130,  139 },
						// IsStart=0, HasOpposite=0, HasLeft=1, HasRight=1
						{  120,  128,  137,  105,  113,  121,  120,  120,  112,  117,  124,  129,  129,   98,  137,  133,  122,  159,  141,  104,  129,  119,   98,  111,  110,  115,  114,  125,  115,  140,  109,  137 },
					}
				},
				{
					{
						// IsStart=0, HasOpposite=1, HasLeft=0, HasRight=0
						{  128,  137,  154,  169,  140,  162,  156,  157,  164,  144,  171,  145,  148,  146,  124,  138,  144,  158,  140,  137,  141,  145,  140,  148,  110,  160,  128,  129,  144,  155,  125,  123 },
						// IsStart=0, HasOpposite=1, HasLeft=0, HasRight=1
						{  124,  115,  136,  131,  145,  143,  159,  144,  158,  165,  128,  191,  135,  173,  147,  137,  128,  163,  164,  151,  162,  178,  161,  143,  168,  166,  122,  160,  170,  175,  132,  109 },
					},
					{
						// IsStart=0, HasOpposite=1, HasLeft=1, HasRight=0
						{  134,  112,  132,  123,  126,  138,  148,  138,  145,  136,  146,  133,  141,  165,  139,  145,  119,  167,  135,  120,  146,  120,  117,  136,  102,  156,  128,  120,  132,  143,   91,  136 },
						// IsStart=0, HasOpposite=1, HasLeft=1, HasRight=1
						{  140,   95,  118,  117,  127,  102,  119,  119,  134,  107,  135,  128,  109,  133,  120,  122,  132,  150,  152,  119,  128,  137,  119,  128,  131,  165,  156,  143,  135,  134,  135,  154 },
					}
				}
			},
			{
				{
					{
						// IsStart=1, HasOpposite=0, HasLeft=0, HasRight=0
						{  139,  132,  139,  133,  130,  134,  135,  131,  133,  139,  141,  139,  132,  136,  139,  150,  140,  137,  143,  157,  149,  157,  168,  155,  159,  181,  176,  185,  219,  167,  133,  143 },
						// IsStart=1, HasOpposite=0, HasLeft=0, HasRight=1
						{  125,  127,  126,  131,  128,  114,  130,  126,  129,  131,  125,  127,  131,  126,  137,  129,  140,   99,  142,   99,  149,  121,  155,  118,  131,  156,  168,  144,  175,  155,  112,  129 },
					},
					{
						// IsStart=1, HasOpposite=0, HasLeft=1, HasRight=0
						{  129,  129,  128,  128,  128,  129,  128,  129,  130,  127,  131,  130,  131,  130,  134,  133,  136,  134,  134,  138,  144,  139,  137,  154,  147,  141,  175,  214,  140,  140,  130,  122 },
						// IsStart=1, HasOpposite=0, HasLeft=1, HasRight=1
						{  128,  128,  124,  123,  125,  107,  127,  128,  125,  128,  128,  128,  128,  128,  128,  130,  107,  124,  136,  119,  139,  127,  132,  140,  125,  150,  133,  150,  138,  130,  127,  127 },
					}
				},
				{
					{
						// IsStart=1, HasOpposite=1, HasLeft=0, HasRight=0
						{  104,  125,  126,  129,  126,  122,  128,  126,  126,  127,  125,  122,  130,  126,  130,  131,  130,  132,  118,  101,  119,  121,  143,  114,  122,  145,  132,  144,  116,  142,  114,  127 },
						// IsStart=1, HasOpposite=1, HasLeft=0, HasRight=1
						{  128,  124,   93,  126,  108,  128,  127,  122,  128,  126,  128,  123,   92,  125,   98,   99,  127,  131,  126,  128,  121,  133,  113,  121,  122,  137,  145,  138,  137,  109,  129,  100 },
					},
					{
						// IsStart=1, HasOpposite=1, HasLeft=1, HasRight=0
						{  119,  128,  122,  128,  127,  123,  126,  128,  126,  122,  120,  127,  128,  122,  130,  121,  138,  122,  136,  130,  133,  124,  139,  134,  138,  118,  139,  145,  132,  122,  124,   86 },
						// IsStart=1, HasOpposite=1, HasLeft=1, HasRight=1
						{  116,  124,  119,  126,  118,  113,  114,  125,  128,  111,  129,  122,  129,  129,  135,  130,  138,  132,  115,  138,  114,  119,  122,  136,  138,  128,  141,  119,  139,  119,  130,  128 },
					}
				}
			}
		}
	};

	static uint32 countbits(uint32 x)
	{
		return std::popcount(x);
	}

	static uint32 firstbithigh(uint32 x)
	{
		return wm::FloorLog2(x);
	}

	static int32 BitFieldExtractI32(int32 Data, int32 NumBits, int32 StartBit)
	{
		return (Data << (32 - StartBit - NumBits)) >> (32 - NumBits);
	}

	static uint32 BitFieldExtractU32(uint32 Data, int32 NumBits, int32 StartBit)
	{
		return (Data << (32 - StartBit - NumBits)) >> (32 - NumBits);
	}

	static uint32 ReadUnalignedDword(const uint8* SrcPtr, int32 BitOffset)	// Note: Only guarantees 25 valid bits
	{
		if (BitOffset < 0)
		{
			// Workaround for reading slightly out of bounds
			wconstraint(BitOffset > -8);
			return *(const uint32*)(SrcPtr) << (8 - (BitOffset & 7));
		}
		else
		{
			const uint32* DwordPtr = (const uint32*)(SrcPtr + (BitOffset >> 3));
			return *DwordPtr >> (BitOffset & 7);
		}
	}

	static void UnpackTriangleIndices(const FStripDesc& StripDesc, const uint8* StripIndexData, uint32 TriIndex, uint32* OutIndices)
	{
		const uint32 DwordIndex = TriIndex >> 5;
		const uint32 BitIndex = TriIndex & 31u;

		//Bitmask.x: bIsStart, Bitmask.y: bIsRight, Bitmask.z: bIsNewVertex
		const uint32 SMask = StripDesc.Bitmasks[DwordIndex][0];
		const uint32 LMask = StripDesc.Bitmasks[DwordIndex][1];
		const uint32 WMask = StripDesc.Bitmasks[DwordIndex][2];
		const uint32 SLMask = SMask & LMask;

		//const uint HeadRefVertexMask = ( SMask & LMask & WMask ) | ( ~SMask & WMask );
		const uint32 HeadRefVertexMask = (SLMask | ~SMask) & WMask;	// 1 if head of triangle is ref. S case with 3 refs or L/R case with 1 ref.

		const uint32 PrevBitsMask = (1u << BitIndex) - 1u;
		const uint32 NumPrevRefVerticesBeforeDword = DwordIndex ? BitFieldExtractU32(StripDesc.NumPrevRefVerticesBeforeDwords, 10u, DwordIndex * 10u - 10u) : 0u;
		const uint32 NumPrevNewVerticesBeforeDword = DwordIndex ? BitFieldExtractU32(StripDesc.NumPrevNewVerticesBeforeDwords, 10u, DwordIndex * 10u - 10u) : 0u;

		int32 CurrentDwordNumPrevRefVertices = (countbits(SLMask & PrevBitsMask) << 1) + countbits(WMask & PrevBitsMask);
		int32 CurrentDwordNumPrevNewVertices = (countbits(SMask & PrevBitsMask) << 1) + BitIndex - CurrentDwordNumPrevRefVertices;

		int32 NumPrevRefVertices = NumPrevRefVerticesBeforeDword + CurrentDwordNumPrevRefVertices;
		int32 NumPrevNewVertices = NumPrevNewVerticesBeforeDword + CurrentDwordNumPrevNewVertices;

		const int32 IsStart = BitFieldExtractI32(SMask, 1, BitIndex);		// -1: true, 0: false
		const int32 IsLeft = BitFieldExtractI32(LMask, 1, BitIndex);		// -1: true, 0: false
		const int32 IsRef = BitFieldExtractI32(WMask, 1, BitIndex);		// -1: true, 0: false

		const uint32 BaseVertex = NumPrevNewVertices - 1u;

		uint32 IndexData = ReadUnalignedDword(StripIndexData, (NumPrevRefVertices + ~IsStart) * 5);	// -1 if not Start

		if (IsStart)
		{
			const int32 MinusNumRefVertices = (IsLeft << 1) + IsRef;
			uint32 NextVertex = NumPrevNewVertices;

			if (MinusNumRefVertices <= -1) { OutIndices[0] = BaseVertex - (IndexData & 31u); IndexData >>= 5; }
			else { OutIndices[0] = NextVertex++; }
			if (MinusNumRefVertices <= -2) { OutIndices[1] = BaseVertex - (IndexData & 31u); IndexData >>= 5; }
			else { OutIndices[1] = NextVertex++; }
			if (MinusNumRefVertices <= -3) { OutIndices[2] = BaseVertex - (IndexData & 31u); }
			else { OutIndices[2] = NextVertex++; }
		}
		else
		{
			// Handle two first vertices
			const uint32 PrevBitIndex = BitIndex - 1u;
			const int32 IsPrevStart = BitFieldExtractI32(SMask, 1, PrevBitIndex);
			const int32 IsPrevHeadRef = BitFieldExtractI32(HeadRefVertexMask, 1, PrevBitIndex);
			//const int NumPrevNewVerticesInTriangle = IsPrevStart ? ( 3u - ( bfe_u32( /*SLMask*/ LMask, PrevBitIndex, 1 ) << 1 ) - bfe_u32( /*SMask &*/ WMask, PrevBitIndex, 1 ) ) : /*1u - IsPrevRefVertex*/ 0u;
			const int32 NumPrevNewVerticesInTriangle = IsPrevStart & (3u - ((BitFieldExtractU32( /*SLMask*/ LMask, 1, PrevBitIndex) << 1) | BitFieldExtractU32( /*SMask &*/ WMask, 1, PrevBitIndex)));

			//OutIndices[ 1 ] = IsPrevRefVertex ? ( BaseVertex - ( IndexData & 31u ) + NumPrevNewVerticesInTriangle ) : BaseVertex;	// BaseVertex = ( NumPrevNewVertices - 1 );
			OutIndices[1] = BaseVertex + (IsPrevHeadRef & (NumPrevNewVerticesInTriangle - (IndexData & 31u)));
			//OutIndices[ 2 ] = IsRefVertex ? ( BaseVertex - bfe_u32( IndexData, 5, 5 ) ) : NumPrevNewVertices;
			OutIndices[2] = NumPrevNewVertices + (IsRef & (-1 - BitFieldExtractU32(IndexData, 5, 5)));

			// We have to search for the third vertex. 
			// Left triangles search for previous Right/Start. Right triangles search for previous Left/Start.
			const uint32 SearchMask = SMask | (LMask ^ IsLeft);				// SMask | ( IsRight ? LMask : RMask );
			const uint32 FoundBitIndex = firstbithigh(SearchMask & PrevBitsMask);
			const int32 IsFoundCaseS = BitFieldExtractI32(SMask, 1, FoundBitIndex);		// -1: true, 0: false

			const uint32 FoundPrevBitsMask = (1u << FoundBitIndex) - 1u;
			int32 FoundCurrentDwordNumPrevRefVertices = (countbits(SLMask & FoundPrevBitsMask) << 1) + countbits(WMask & FoundPrevBitsMask);
			int32 FoundCurrentDwordNumPrevNewVertices = (countbits(SMask & FoundPrevBitsMask) << 1) + FoundBitIndex - FoundCurrentDwordNumPrevRefVertices;

			int32 FoundNumPrevNewVertices = NumPrevNewVerticesBeforeDword + FoundCurrentDwordNumPrevNewVertices;
			int32 FoundNumPrevRefVertices = NumPrevRefVerticesBeforeDword + FoundCurrentDwordNumPrevRefVertices;

			const uint32 FoundNumRefVertices = (BitFieldExtractU32(LMask, 1, FoundBitIndex) << 1) + BitFieldExtractU32(WMask, 1, FoundBitIndex);
			const uint32 IsBeforeFoundRefVertex = BitFieldExtractU32(HeadRefVertexMask, 1, FoundBitIndex - 1);

			// ReadOffset: Where is the vertex relative to triangle we searched for?
			const int32 ReadOffset = IsFoundCaseS ? IsLeft : 1;
			const uint32 FoundIndexData = ReadUnalignedDword(StripIndexData, (FoundNumPrevRefVertices - ReadOffset) * 5);
			const uint32 FoundIndex = (FoundNumPrevNewVertices - 1u) - BitFieldExtractU32(FoundIndexData, 5, 0);

			bool bCondition = IsFoundCaseS ? ((int32)FoundNumRefVertices >= 1 - IsLeft) : (IsBeforeFoundRefVertex != 0u);
			int32 FoundNewVertex = FoundNumPrevNewVertices + (IsFoundCaseS ? (IsLeft & (FoundNumRefVertices == 0)) : -1);
			OutIndices[0] = bCondition ? FoundIndex : FoundNewVertex;

			// Would it be better to code New verts instead of Ref verts?
			// HeadRefVertexMask would just be WMask?

			// TODO: could we do better with non-generalized strips?

			/*
			if( IsFoundCaseS )
			{
				if( IsRight )
				{
					OutIndices[ 0 ] = ( FoundNumRefVertices >= 1 ) ? FoundIndex : FoundNumPrevNewVertices;
					// OutIndices[ 0 ] = ( FoundNumRefVertices >= 1 ) ? ( FoundBaseVertex - Cluster.StripIndices[ FoundNumPrevRefVertices ] ) : FoundNumPrevNewVertices;
				}
				else
				{
					OutIndices[ 0 ] = ( FoundNumRefVertices >= 2 ) ? FoundIndex : ( FoundNumPrevNewVertices + ( FoundNumRefVertices == 0 ? 1 : 0 ) );
					// OutIndices[ 0 ] = ( FoundNumRefVertices >= 2 ) ? ( FoundBaseVertex - Cluster.StripIndices[ FoundNumPrevRefVertices + 1 ] ) : ( FoundNumPrevNewVertices + ( FoundNumRefVertices == 0 ? 1 : 0 ) );
				}
			}
			else
			{
				OutIndices[ 0 ] = IsBeforeFoundRefVertex ? FoundIndex : ( FoundNumPrevNewVertices - 1 );
				// OutIndices[ 0 ] = IsBeforeFoundRefVertex ? ( FoundBaseVertex - Cluster.StripIndices[ FoundNumPrevRefVertices - 1 ] ) : ( FoundNumPrevNewVertices - 1 );
			}
			*/

			if (IsLeft)
			{
				// swap
				std::swap(OutIndices[1], OutIndices[2]);
			}
			wconstraint(OutIndices[0] != OutIndices[1] && OutIndices[0] != OutIndices[2] && OutIndices[1] != OutIndices[2]);
		}
	}

	// Class to simultaneously constrain and stripify a cluster
	class FStripifier
	{
		static const uint32 MAX_CLUSTER_TRIANGLES_IN_DWORDS = (MAX_CLUSTER_TRIANGLES + 31) / 32;
		static const uint32 INVALID_INDEX = 0xFFFFu;
		static const uint32 INVALID_CORNER = 0xFFFFu;
		static const uint32 INVALID_NODE = 0xFFFFu;

		uint32 VertexToTriangleMasks[MAX_CLUSTER_TRIANGLES * 3][MAX_CLUSTER_TRIANGLES_IN_DWORDS];
		uint16 OppositeCorner[MAX_CLUSTER_TRIANGLES * 3];
		float TrianglePriorities[MAX_CLUSTER_TRIANGLES];

		class FContext
		{
		public:
			bool TriangleEnabled(uint32 TriangleIndex) const
			{
				return (TrianglesEnabled[TriangleIndex >> 5] & (1u << (TriangleIndex & 31u))) != 0u;
			}

			uint16 OldToNewVertex[MAX_CLUSTER_TRIANGLES * 3];
			uint16 NewToOldVertex[MAX_CLUSTER_TRIANGLES * 3];

			uint32 TrianglesEnabled[MAX_CLUSTER_TRIANGLES_IN_DWORDS];	// Enabled triangles are in the current material range and have not yet been visited.
			uint32 TrianglesTouched[MAX_CLUSTER_TRIANGLES_IN_DWORDS];	// Touched triangles have had at least one of their vertices visited.

			uint32 StripBitmasks[4][3];	// [4][Reset, IsLeft, IsRef]

			uint32 NumTriangles;
			uint32 NumVertices;
		};

		void BuildTables(const FCluster& Cluster)
		{
			struct FEdgeNode
			{
				uint16 Corner;	// (Triangle << 2) | LocalCorner
				uint16 NextNode;
			};

			FEdgeNode EdgeNodes[MAX_CLUSTER_INDICES];
			std::vector<uint16> EdgeNodeHeads;
			EdgeNodeHeads.resize(MAX_CLUSTER_INDICES * MAX_CLUSTER_INDICES, INVALID_NODE);	// Linked list per edge to support more than 2 triangles per edge.

			white::memset(VertexToTriangleMasks, 0);

			uint32 NumTriangles = Cluster.NumTris;
			uint32 NumVertices = Cluster.NumVerts;

			// Add triangles to edge lists and update valence
			for (uint32 i = 0; i < NumTriangles; i++)
			{
				uint32 i0 = Cluster.Indexes[i * 3 + 0];
				uint32 i1 = Cluster.Indexes[i * 3 + 1];
				uint32 i2 = Cluster.Indexes[i * 3 + 2];
				wconstraint(i0 != i1 && i1 != i2 && i2 != i0);
				wconstraint(i0 < NumVertices&& i1 < NumVertices&& i2 < NumVertices);

				VertexToTriangleMasks[i0][i >> 5] |= 1 << (i & 31);
				VertexToTriangleMasks[i1][i >> 5] |= 1 << (i & 31);
				VertexToTriangleMasks[i2][i >> 5] |= 1 << (i & 31);

				wm::float3 ScaledCenter = Cluster.GetPosition(i0) + Cluster.GetPosition(i1) + Cluster.GetPosition(i2);
				TrianglePriorities[i] = ScaledCenter.x;	//TODO: Find a good direction to sort by instead of just picking x?

				FEdgeNode& Node0 = EdgeNodes[i * 3 + 0];
				Node0.Corner = SetCorner(i, 0);
				Node0.NextNode = EdgeNodeHeads[i1 * MAX_CLUSTER_INDICES + i2];
				EdgeNodeHeads[i1 * MAX_CLUSTER_INDICES + i2] = i * 3 + 0;

				FEdgeNode& Node1 = EdgeNodes[i * 3 + 1];
				Node1.Corner = SetCorner(i, 1);
				Node1.NextNode = EdgeNodeHeads[i2 * MAX_CLUSTER_INDICES + i0];
				EdgeNodeHeads[i2 * MAX_CLUSTER_INDICES + i0] = i * 3 + 1;

				FEdgeNode& Node2 = EdgeNodes[i * 3 + 2];
				Node2.Corner = SetCorner(i, 2);
				Node2.NextNode = EdgeNodeHeads[i0 * MAX_CLUSTER_INDICES + i1];
				EdgeNodeHeads[i0 * MAX_CLUSTER_INDICES + i1] = i * 3 + 2;
			}

			// Gather adjacency from edge lists	
			for (uint32 i = 0; i < NumTriangles; i++)
			{
				uint32 i0 = Cluster.Indexes[i * 3 + 0];
				uint32 i1 = Cluster.Indexes[i * 3 + 1];
				uint32 i2 = Cluster.Indexes[i * 3 + 2];

				uint16& Node0 = EdgeNodeHeads[i2 * MAX_CLUSTER_INDICES + i1];
				uint16& Node1 = EdgeNodeHeads[i0 * MAX_CLUSTER_INDICES + i2];
				uint16& Node2 = EdgeNodeHeads[i1 * MAX_CLUSTER_INDICES + i0];
				if (Node0 != INVALID_NODE) { OppositeCorner[i * 3 + 0] = EdgeNodes[Node0].Corner; Node0 = EdgeNodes[Node0].NextNode; }
				else { OppositeCorner[i * 3 + 0] = INVALID_CORNER; }
				if (Node1 != INVALID_NODE) { OppositeCorner[i * 3 + 1] = EdgeNodes[Node1].Corner; Node1 = EdgeNodes[Node1].NextNode; }
				else { OppositeCorner[i * 3 + 1] = INVALID_CORNER; }
				if (Node2 != INVALID_NODE) { OppositeCorner[i * 3 + 2] = EdgeNodes[Node2].Corner; Node2 = EdgeNodes[Node2].NextNode; }
				else { OppositeCorner[i * 3 + 2] = INVALID_CORNER; }
			}

			// Generate vertex to triangle masks
			for (uint32 i = 0; i < NumTriangles; i++)
			{
				uint32 i0 = Cluster.Indexes[i * 3 + 0];
				uint32 i1 = Cluster.Indexes[i * 3 + 1];
				uint32 i2 = Cluster.Indexes[i * 3 + 2];
				wconstraint(i0 != i1 && i1 != i2 && i2 != i0);

				VertexToTriangleMasks[i0][i >> 5] |= 1 << (i & 31);
				VertexToTriangleMasks[i1][i >> 5] |= 1 << (i & 31);
				VertexToTriangleMasks[i2][i >> 5] |= 1 << (i & 31);
			}
		}

	public:
		void ConstrainAndStripifyCluster(FCluster& Cluster)
		{
			const FStripifyWeights& Weights = DefaultStripifyWeights;
			uint32 NumOldTriangles = Cluster.NumTris;
			uint32 NumOldVertices = Cluster.NumVerts;

			BuildTables(Cluster);

			uint32 NumStrips = 0;

			FContext Context = {};
			white::memset(Context.OldToNewVertex, -1);

			auto NewScoreVertex = [&Weights](const FContext& Context, uint32 OldVertex, bool bStart, bool bHasOpposite, bool bHasLeft, bool bHasRight)
			{
				uint16 NewIndex = Context.OldToNewVertex[OldVertex];

				int32 CacheScore = 0;
				if (NewIndex != INVALID_INDEX)
				{
					uint32 CachePosition = (Context.NumVertices - 1) - NewIndex;
					if (CachePosition < CONSTRAINED_CLUSTER_CACHE_SIZE)
						CacheScore = Weights.Weights[bStart][bHasOpposite][bHasLeft][bHasRight][CachePosition];
				}

				return CacheScore;
			};

			auto NewScoreTriangle = [&Cluster, &NewScoreVertex](const FContext& Context, uint32 TriangleIndex, bool bStart, bool bHasOpposite, bool bHasLeft, bool bHasRight)
			{
				const uint32 OldIndex0 = Cluster.Indexes[TriangleIndex * 3 + 0];
				const uint32 OldIndex1 = Cluster.Indexes[TriangleIndex * 3 + 1];
				const uint32 OldIndex2 = Cluster.Indexes[TriangleIndex * 3 + 2];

				return	NewScoreVertex(Context, OldIndex0, bStart, bHasOpposite, bHasLeft, bHasRight) +
					NewScoreVertex(Context, OldIndex1, bStart, bHasOpposite, bHasLeft, bHasRight) +
					NewScoreVertex(Context, OldIndex2, bStart, bHasOpposite, bHasLeft, bHasRight);
			};

			auto VisitTriangle = [this, &Cluster](FContext& Context, uint32 TriangleCorner, bool bStart, bool bRight)
			{
				const uint32 OldIndex0 = Cluster.Indexes[CornerToIndex(NextCorner(TriangleCorner))];
				const uint32 OldIndex1 = Cluster.Indexes[CornerToIndex(PrevCorner(TriangleCorner))];
				const uint32 OldIndex2 = Cluster.Indexes[CornerToIndex(TriangleCorner)];

				// Mark incident triangles
				for (uint32 i = 0; i < MAX_CLUSTER_TRIANGLES_IN_DWORDS; i++)
				{
					Context.TrianglesTouched[i] |= VertexToTriangleMasks[OldIndex0][i] | VertexToTriangleMasks[OldIndex1][i] | VertexToTriangleMasks[OldIndex2][i];
				}

				uint16& NewIndex0 = Context.OldToNewVertex[OldIndex0];
				uint16& NewIndex1 = Context.OldToNewVertex[OldIndex1];
				uint16& NewIndex2 = Context.OldToNewVertex[OldIndex2];

				uint32 OrgIndex0 = NewIndex0;
				uint32 OrgIndex1 = NewIndex1;
				uint32 OrgIndex2 = NewIndex2;

				uint32 NextVertexIndex = Context.NumVertices + (NewIndex0 == INVALID_INDEX) + (NewIndex1 == INVALID_INDEX) + (NewIndex2 == INVALID_INDEX);
				while (true)
				{
					if (NewIndex0 != INVALID_INDEX && NextVertexIndex - NewIndex0 >= CONSTRAINED_CLUSTER_CACHE_SIZE) { NewIndex0 = INVALID_INDEX; NextVertexIndex++; continue; }
					if (NewIndex1 != INVALID_INDEX && NextVertexIndex - NewIndex1 >= CONSTRAINED_CLUSTER_CACHE_SIZE) { NewIndex1 = INVALID_INDEX; NextVertexIndex++; continue; }
					if (NewIndex2 != INVALID_INDEX && NextVertexIndex - NewIndex2 >= CONSTRAINED_CLUSTER_CACHE_SIZE) { NewIndex2 = INVALID_INDEX; NextVertexIndex++; continue; }
					break;
				}

				uint32 NewTriangleIndex = Context.NumTriangles;
				uint32 NumNewVertices = (NewIndex0 == INVALID_INDEX) + (NewIndex1 == INVALID_INDEX) + (NewIndex2 == INVALID_INDEX);
				if (bStart)
				{
					wconstraint((NewIndex2 == INVALID_INDEX) >= (NewIndex1 == INVALID_INDEX));
					wconstraint((NewIndex1 == INVALID_INDEX) >= (NewIndex0 == INVALID_INDEX));


					uint32 NumWrittenIndices = 3u - NumNewVertices;
					uint32 LowBit = NumWrittenIndices & 1u;
					uint32 HighBit = (NumWrittenIndices >> 1) & 1u;

					Context.StripBitmasks[NewTriangleIndex >> 5][0] |= (1u << (NewTriangleIndex & 31u));
					Context.StripBitmasks[NewTriangleIndex >> 5][1] |= (HighBit << (NewTriangleIndex & 31u));
					Context.StripBitmasks[NewTriangleIndex >> 5][2] |= (LowBit << (NewTriangleIndex & 31u));
				}
				else
				{
					wconstraint(NewIndex0 != INVALID_INDEX);
					wconstraint(NewIndex1 != INVALID_INDEX);
					if (!bRight)
					{
						Context.StripBitmasks[NewTriangleIndex >> 5][1] |= (1 << (NewTriangleIndex & 31u));
					}

					if (NewIndex2 != INVALID_INDEX)
					{
						Context.StripBitmasks[NewTriangleIndex >> 5][2] |= (1 << (NewTriangleIndex & 31u));
					}
				}

				if (NewIndex0 == INVALID_INDEX) { NewIndex0 = Context.NumVertices++; Context.NewToOldVertex[NewIndex0] = OldIndex0; }
				if (NewIndex1 == INVALID_INDEX) { NewIndex1 = Context.NumVertices++; Context.NewToOldVertex[NewIndex1] = OldIndex1; }
				if (NewIndex2 == INVALID_INDEX) { NewIndex2 = Context.NumVertices++; Context.NewToOldVertex[NewIndex2] = OldIndex2; }

				// Output triangle
				Context.NumTriangles++;

				// Disable selected triangle
				const uint32 OldTriangleIndex = CornerToTriangle(TriangleCorner);
				Context.TrianglesEnabled[OldTriangleIndex >> 5] &= ~(1 << (OldTriangleIndex & 31u));
				return NumNewVertices;
			};

			Cluster.StripIndexData.clear();
			FBitWriter BitWriter(Cluster.StripIndexData);
			FStripDesc& StripDesc = Cluster.StripDesc;
			white::memset(StripDesc, 0);
			uint32 NumNewVerticesInDword[4] = {};
			uint32 NumRefVerticesInDword[4] = {};

			uint32 RangeStart = 0;
			for (const FMaterialRange& MaterialRange : Cluster.MaterialRanges)
			{
				wconstraint(RangeStart == MaterialRange.RangeStart);
				uint32 RangeLength = MaterialRange.RangeLength;

				// Enable triangles from current range
				for (uint32 i = 0; i < MAX_CLUSTER_TRIANGLES_IN_DWORDS; i++)
				{
					int32 RangeStartRelativeToDword = (int32)RangeStart - (int32)i * 32;
					int32 BitStart = std::max(RangeStartRelativeToDword, 0);
					int32 BitEnd = std::max(RangeStartRelativeToDword + (int32)RangeLength, 0);
					uint32 StartMask = BitStart < 32 ? ((1u << BitStart) - 1u) : 0xFFFFFFFFu;
					uint32 EndMask = BitEnd < 32 ? ((1u << BitEnd) - 1u) : 0xFFFFFFFFu;
					Context.TrianglesEnabled[i] |= StartMask ^ EndMask;
				}

				// While a strip can be started
				while (true)
				{
					// Pick a start location for the strip
					uint32 StartCorner = INVALID_CORNER;
					int32 BestScore = -1;
					float BestPriority = INT_MIN;
					{
						for (uint32 TriangleDwordIndex = 0; TriangleDwordIndex < MAX_CLUSTER_TRIANGLES_IN_DWORDS; TriangleDwordIndex++)
						{
							uint32 CandidateMask = Context.TrianglesEnabled[TriangleDwordIndex];
							while (CandidateMask)
							{
								uint32 TriangleIndex = (TriangleDwordIndex << 5) + std::countr_zero(CandidateMask);
								CandidateMask &= CandidateMask - 1u;

								for (uint32 Corner = 0; Corner < 3; Corner++)
								{
									uint32 TriangleCorner = SetCorner(TriangleIndex, Corner);

									{
										// Is it viable WRT the constraint that new vertices should always be at the end.
										uint32 OldIndex0 = Cluster.Indexes[CornerToIndex(NextCorner(TriangleCorner))];
										uint32 OldIndex1 = Cluster.Indexes[CornerToIndex(PrevCorner(TriangleCorner))];
										uint32 OldIndex2 = Cluster.Indexes[CornerToIndex(TriangleCorner)];

										uint32 NewIndex0 = Context.OldToNewVertex[OldIndex0];
										uint32 NewIndex1 = Context.OldToNewVertex[OldIndex1];
										uint32 NewIndex2 = Context.OldToNewVertex[OldIndex2];
										uint32 NumVerts = Context.NumVertices + (NewIndex0 == INVALID_INDEX) + (NewIndex1 == INVALID_INDEX) + (NewIndex2 == INVALID_INDEX);
										while (true)
										{
											if (NewIndex0 != INVALID_INDEX && NumVerts - NewIndex0 >= CONSTRAINED_CLUSTER_CACHE_SIZE) { NewIndex0 = INVALID_INDEX; NumVerts++; continue; }
											if (NewIndex1 != INVALID_INDEX && NumVerts - NewIndex1 >= CONSTRAINED_CLUSTER_CACHE_SIZE) { NewIndex1 = INVALID_INDEX; NumVerts++; continue; }
											if (NewIndex2 != INVALID_INDEX && NumVerts - NewIndex2 >= CONSTRAINED_CLUSTER_CACHE_SIZE) { NewIndex2 = INVALID_INDEX; NumVerts++; continue; }
											break;
										}

										uint32 Mask = (NewIndex0 == INVALID_INDEX ? 1u : 0u) | (NewIndex1 == INVALID_INDEX ? 2u : 0u) | (NewIndex2 == INVALID_INDEX ? 4u : 0u);

										if (Mask != 0u && Mask != 4u && Mask != 6u && Mask != 7u)
										{
											continue;
										}
									}


									uint32 Opposite = OppositeCorner[CornerToIndex(TriangleCorner)];
									uint32 LeftCorner = OppositeCorner[CornerToIndex(NextCorner(TriangleCorner))];
									uint32 RightCorner = OppositeCorner[CornerToIndex(PrevCorner(TriangleCorner))];

									bool bHasOpposite = Opposite != INVALID_CORNER && Context.TriangleEnabled(CornerToTriangle(Opposite));
									bool bHasLeft = LeftCorner != INVALID_CORNER && Context.TriangleEnabled(CornerToTriangle(LeftCorner));
									bool bHasRight = RightCorner != INVALID_CORNER && Context.TriangleEnabled(CornerToTriangle(RightCorner));

									int32 Score = NewScoreTriangle(Context, TriangleIndex, true, bHasOpposite, bHasLeft, bHasRight);
									if (Score > BestScore)
									{
										StartCorner = TriangleCorner;
										BestScore = Score;
									}
									else if (Score == BestScore)
									{
										float Priority = TrianglePriorities[TriangleIndex];
										if (Priority > BestPriority)
										{
											StartCorner = TriangleCorner;
											BestScore = Score;
											BestPriority = Priority;
										}
									}
								}
							}
						}

						if (StartCorner == INVALID_CORNER)
							break;
					}

					uint32 StripLength = 1;

					{
						uint32 TriangleDword = Context.NumTriangles >> 5;
						uint32 BaseVertex = Context.NumVertices - 1;
						uint32 NumNewVertices = VisitTriangle(Context, StartCorner, true, false);

						if (NumNewVertices < 3)
						{
							uint32 Index = Context.OldToNewVertex[Cluster.Indexes[CornerToIndex(NextCorner(StartCorner))]];
							BitWriter.PutBits(BaseVertex - Index, 5);
						}
						if (NumNewVertices < 2)
						{
							uint32 Index = Context.OldToNewVertex[Cluster.Indexes[CornerToIndex(PrevCorner(StartCorner))]];
							BitWriter.PutBits(BaseVertex - Index, 5);
						}
						if (NumNewVertices < 1)
						{
							uint32 Index = Context.OldToNewVertex[Cluster.Indexes[CornerToIndex(StartCorner)]];
							BitWriter.PutBits(BaseVertex - Index, 5);
						}
						NumNewVerticesInDword[TriangleDword] += NumNewVertices;
						NumRefVerticesInDword[TriangleDword] += 3u - NumNewVertices;
					}

					// Extend strip as long as we can
					uint32 CurrentCorner = StartCorner;
					while (true)
					{
						if ((Context.NumTriangles & 31u) == 0u)
							break;

						uint32 LeftCorner = OppositeCorner[CornerToIndex(NextCorner(CurrentCorner))];
						uint32 RightCorner = OppositeCorner[CornerToIndex(PrevCorner(CurrentCorner))];
						CurrentCorner = INVALID_CORNER;

						int32 LeftScore = INT_MIN;
						if (LeftCorner != INVALID_CORNER && Context.TriangleEnabled(CornerToTriangle(LeftCorner)))
						{
							uint32 LeftLeftCorner = OppositeCorner[CornerToIndex(NextCorner(LeftCorner))];
							uint32 LeftRightCorner = OppositeCorner[CornerToIndex(PrevCorner(LeftCorner))];
							bool bLeftLeftCorner = LeftLeftCorner != INVALID_CORNER && Context.TriangleEnabled(CornerToTriangle(LeftLeftCorner));
							bool bLeftRightCorner = LeftRightCorner != INVALID_CORNER && Context.TriangleEnabled(CornerToTriangle(LeftRightCorner));

							LeftScore = NewScoreTriangle(Context, CornerToTriangle(LeftCorner), false, true, bLeftLeftCorner, bLeftRightCorner);
							CurrentCorner = LeftCorner;
						}

						bool bIsRight = false;
						if (RightCorner != INVALID_CORNER && Context.TriangleEnabled(CornerToTriangle(RightCorner)))
						{
							uint32 RightLeftCorner = OppositeCorner[CornerToIndex(NextCorner(RightCorner))];
							uint32 RightRightCorner = OppositeCorner[CornerToIndex(PrevCorner(RightCorner))];
							bool bRightLeftCorner = RightLeftCorner != INVALID_CORNER && Context.TriangleEnabled(CornerToTriangle(RightLeftCorner));
							bool bRightRightCorner = RightRightCorner != INVALID_CORNER && Context.TriangleEnabled(CornerToTriangle(RightRightCorner));

							int32 Score = NewScoreTriangle(Context, CornerToTriangle(RightCorner), false, false, bRightLeftCorner, bRightRightCorner);
							if (Score > LeftScore)
							{
								CurrentCorner = RightCorner;
								bIsRight = true;
							}
						}

						if (CurrentCorner == INVALID_CORNER)
							break;

						{
							const uint32 OldIndex0 = Cluster.Indexes[CornerToIndex(NextCorner(CurrentCorner))];
							const uint32 OldIndex1 = Cluster.Indexes[CornerToIndex(PrevCorner(CurrentCorner))];
							const uint32 OldIndex2 = Cluster.Indexes[CornerToIndex(CurrentCorner)];

							const uint32 NewIndex0 = Context.OldToNewVertex[OldIndex0];
							const uint32 NewIndex1 = Context.OldToNewVertex[OldIndex1];
							const uint32 NewIndex2 = Context.OldToNewVertex[OldIndex2];

							wconstraint(NewIndex0 != INVALID_INDEX);
							wconstraint(NewIndex1 != INVALID_INDEX);
							const uint32 NextNumVertices = Context.NumVertices + ((NewIndex2 == INVALID_INDEX || Context.NumVertices - NewIndex2 >= CONSTRAINED_CLUSTER_CACHE_SIZE) ? 1u : 0u);

							if (NextNumVertices - NewIndex0 >= CONSTRAINED_CLUSTER_CACHE_SIZE ||
								NextNumVertices - NewIndex1 >= CONSTRAINED_CLUSTER_CACHE_SIZE)
								break;
						}

						{
							uint32 TriangleDword = Context.NumTriangles >> 5;
							uint32 BaseVertex = Context.NumVertices - 1;
							uint32 NumNewVertices = VisitTriangle(Context, CurrentCorner, false, bIsRight);
							wconstraint(NumNewVertices <= 1u);
							if (NumNewVertices == 0)
							{
								uint32 Index = Context.OldToNewVertex[Cluster.Indexes[CornerToIndex(CurrentCorner)]];
								BitWriter.PutBits(BaseVertex - Index, 5);
							}
							NumNewVerticesInDword[TriangleDword] += NumNewVertices;
							NumRefVerticesInDword[TriangleDword] += 1u - NumNewVertices;
						}

						StripLength++;
					}
				}
				RangeStart += RangeLength;
			}

			BitWriter.Flush(sizeof(uint32));

			// Reorder vertices
			const uint32 NumNewVertices = Context.NumVertices;

			std::vector< float > OldVertices;
			std::swap(OldVertices, Cluster.Verts);

			uint32 VertStride = Cluster.GetVertSize();
			Cluster.Verts.resize(NumNewVertices * VertStride);
			for (uint32 i = 0; i < NumNewVertices; i++)
			{
				std::memcpy(&Cluster.GetPosition(i), &OldVertices[Context.NewToOldVertex[i] * VertStride], VertStride * sizeof(float));
			}

			wconstraint(Context.NumTriangles == NumOldTriangles);

			Cluster.NumVerts = Context.NumVertices;

			uint32 NumPrevNewVerticesBeforeDwords1 = NumNewVerticesInDword[0];
			uint32 NumPrevNewVerticesBeforeDwords2 = NumNewVerticesInDword[1] + NumPrevNewVerticesBeforeDwords1;
			uint32 NumPrevNewVerticesBeforeDwords3 = NumNewVerticesInDword[2] + NumPrevNewVerticesBeforeDwords2;
			wconstraint(NumPrevNewVerticesBeforeDwords1 < 1024 && NumPrevNewVerticesBeforeDwords2 < 1024 && NumPrevNewVerticesBeforeDwords3 < 1024);
			StripDesc.NumPrevNewVerticesBeforeDwords = (NumPrevNewVerticesBeforeDwords3 << 20) | (NumPrevNewVerticesBeforeDwords2 << 10) | NumPrevNewVerticesBeforeDwords1;

			uint32 NumPrevRefVerticesBeforeDwords1 = NumRefVerticesInDword[0];
			uint32 NumPrevRefVerticesBeforeDwords2 = NumRefVerticesInDword[1] + NumPrevRefVerticesBeforeDwords1;
			uint32 NumPrevRefVerticesBeforeDwords3 = NumRefVerticesInDword[2] + NumPrevRefVerticesBeforeDwords2;
			wconstraint(NumPrevRefVerticesBeforeDwords1 < 1024 && NumPrevRefVerticesBeforeDwords2 < 1024 && NumPrevRefVerticesBeforeDwords3 < 1024);
			StripDesc.NumPrevRefVerticesBeforeDwords = (NumPrevRefVerticesBeforeDwords3 << 20) | (NumPrevRefVerticesBeforeDwords2 << 10) | NumPrevRefVerticesBeforeDwords1;

			static_assert(sizeof(StripDesc.Bitmasks) == sizeof(Context.StripBitmasks), "");
			std::memcpy(StripDesc.Bitmasks, Context.StripBitmasks, sizeof(StripDesc.Bitmasks));

			const uint32 PaddedSize = Cluster.StripIndexData.size() + 5;
			std::vector<uint8> PaddedStripIndexData;
			PaddedStripIndexData.reserve(PaddedSize);

			PaddedStripIndexData.emplace_back(0);	// TODO: Workaround for empty list and reading from negative offset
			PaddedStripIndexData.insert(PaddedStripIndexData.end(), Cluster.StripIndexData.begin(), Cluster.StripIndexData.end());

			// UnpackTriangleIndices is 1:1 with the GPU implementation.
			// It can end up over-fetching because it is branchless. The over-fetched data is never actually used.
			// On the GPU index data is followed by other page data, so it is safe.

			// Here we have to pad to make it safe to perform a DWORD read after the end.
			PaddedStripIndexData.resize(PaddedSize);

			// Unpack strip
			for (uint32 i = 0; i < NumOldTriangles; i++)
			{
				UnpackTriangleIndices(StripDesc, (const uint8*)(PaddedStripIndexData.data() + 1), i, &Cluster.Indexes[i * 3]);
			}
		}
	};

	void ConstrainCluster(FCluster& Cluster)
	{
#if USE_STRIP_INDICES
		FStripifier Stripifier;
		Stripifier.ConstrainAndStripifyCluster(Cluster);
#else
		ConstrainClusterFIFO(OutCluster);
#endif
	}

	static void BuildClusterFromClusterTriangleRange(const FCluster& InCluster, FCluster& OutCluster, uint32 StartTriangle, uint32 NumTriangles)
	{
		OutCluster = InCluster;
		OutCluster.Indexes.clear();
		OutCluster.MaterialIndexes.clear();
		OutCluster.MaterialRanges.clear();

		// Copy triangle indices and material indices.
		// Ignore that some of the vertices will no longer be referenced as that will be cleaned up in ConstrainCluster* pass
		OutCluster.Indexes.resize(NumTriangles * 3);
		OutCluster.MaterialIndexes.resize(NumTriangles);
		for (uint32 i = 0; i < NumTriangles; i++)
		{
			uint32 TriangleIndex = StartTriangle + i;

			OutCluster.MaterialIndexes[i] = InCluster.MaterialIndexes[TriangleIndex];
			OutCluster.Indexes[i * 3 + 0] = InCluster.Indexes[TriangleIndex * 3 + 0];
			OutCluster.Indexes[i * 3 + 1] = InCluster.Indexes[TriangleIndex * 3 + 1];
			OutCluster.Indexes[i * 3 + 2] = InCluster.Indexes[TriangleIndex * 3 + 2];
		}

		OutCluster.NumTris = NumTriangles;

		// Rebuild material range and reconstrain 
		BuildMaterialRanges(OutCluster);
		ConstrainCluster(OutCluster);
	}



	static void ConstrainClusters(std::vector< FClusterGroup >& ClusterGroups, std::vector< FCluster >& Clusters)
	{
		ParallelFor(Clusters.size(),
			[&](uint32 i)
			{
				ConstrainCluster(Clusters[i]);
			});

		const uint32 NumOldClusters = Clusters.size();
		for (uint32 i = 0; i < NumOldClusters; i++)
		{
			// Split clusters with too many verts
			if (Clusters[i].NumVerts > 256)
			{
				FCluster ClusterA, ClusterB;
				uint32 NumTrianglesA = Clusters[i].NumTris / 2;
				uint32 NumTrianglesB = Clusters[i].NumTris - NumTrianglesA;
				BuildClusterFromClusterTriangleRange(Clusters[i], ClusterA, 0, NumTrianglesA);
				BuildClusterFromClusterTriangleRange(Clusters[i], ClusterB, NumTrianglesA, NumTrianglesB);
				Clusters[i] = ClusterA;
				ClusterGroups[ClusterB.GroupIndex].Children.emplace_back(Clusters.size());
				Clusters.emplace_back(ClusterB);
			}
		}
	}

	static int32 CalculateQuantizedPositionsUniformGrid(std::vector< FCluster >& Clusters, const FBounds& MeshBounds, const Settings& Settings)
	{
		// Simple global quantization for EA
		const int32 MaxPositionQuantizedValue = (1 << MAX_POSITION_QUANTIZATION_BITS) - 1;

		int32 PositionPrecision = Settings.PositionPrecision;
		if (PositionPrecision == std::numeric_limits<int32>::min())
		{
			// Auto: Guess required precision from bounds at leaf level
			const float MaxSize = std::max(MeshBounds.GetExtent());

			// Heuristic: We want higher resolution if the mesh is denser.
			// Use geometric average of cluster size as a proxy for density.
			// Alternative interpretation: Bit precision is average of what is needed by the clusters.
			// For roughly uniformly sized clusters this gives results very similar to the old quantization code.
			double TotalLogSize = 0.0;
			int32 TotalNum = 0;
			for (const FCluster& Cluster : Clusters)
			{
				if (Cluster.MipLevel == 0)
				{
					float ExtentSize = wm::length(Cluster.Bounds.GetExtent());
					if (ExtentSize > 0.0)
					{
						TotalLogSize += std::log2(ExtentSize);
						TotalNum++;
					}
				}
			}
			double AvgLogSize = TotalNum > 0 ? TotalLogSize / TotalNum : 0.0;
			PositionPrecision = 7 - std::lround(AvgLogSize);

			// Clamp precision. The user now needs to explicitly opt-in to the lowest precision settings.
			// These settings are likely to cause issues and contribute little to disk size savings (~0.4% on test project),
			// so we shouldn't pick them automatically.
			// Example: A very low resolution road or building frame that needs little precision to look right in isolation,
			// but still requires fairly high precision in a scene because smaller meshes are placed on it or in it.
			const int32 AUTO_MIN_PRECISION = 4;	// 32/8
			PositionPrecision = std::max(PositionPrecision, AUTO_MIN_PRECISION);
		}

		float QuantizationScale = std::exp2((float)PositionPrecision);

		// Make sure all clusters are encodable. A large enough cluster could hit the 21bpc limit. If it happens scale back until it fits.
		for (const FCluster& Cluster : Clusters)
		{
			const FBounds& Bounds = Cluster.Bounds;

			int32 Iterations = 0;
			while (true)
			{
				float MinX = std::round(Bounds.Min.x * QuantizationScale);
				float MinY = std::round(Bounds.Min.y * QuantizationScale);
				float MinZ = std::round(Bounds.Min.z * QuantizationScale);

				float MaxX = std::round(Bounds.Max.x * QuantizationScale);
				float MaxY = std::round(Bounds.Max.y * QuantizationScale);
				float MaxZ = std::round(Bounds.Max.z * QuantizationScale);

				if (MinX >= (double)wm::MIN_int32 && MinY >= (double)wm::MIN_int32 && MinZ >= (double)wm::MIN_int32 &&	// MIN_int32/MAX_int32 is not representable in float
					MaxX <= (double)wm::MAX_int32 && MaxY <= (double)wm::MAX_int32 && MaxZ <= (double)wm::MAX_int32 &&
					((int32)MaxX - (int32)MinX) <= MaxPositionQuantizedValue && ((int32)MaxY - (int32)MinY) <= MaxPositionQuantizedValue && ((int32)MaxZ - (int32)MinZ) <= MaxPositionQuantizedValue)
				{
					break;
				}

				QuantizationScale *= 0.5f;
				PositionPrecision--;
				wconstraint(++Iterations < 100);	// Endless loop?
			}
		}

		const float RcpQuantizationScale = 1.0f / QuantizationScale;

		ParallelFor(Clusters.size(), [&](uint32 ClusterIndex)
			{
				FCluster& Cluster = Clusters[ClusterIndex];

				const uint32 NumClusterVerts = Cluster.NumVerts;
				const uint32 ClusterShift = Cluster.QuantizedPosShift;

				Cluster.QuantizedPositions.resize(NumClusterVerts);

				// Quantize positions
				wm::int3 IntClusterMax = { wm::MIN_int32,	wm::MIN_int32, wm::MIN_int32 };
				wm::int3 IntClusterMin = { wm::MAX_int32,	wm::MAX_int32, wm::MAX_int32 };

				for (uint32 i = 0; i < NumClusterVerts; i++)
				{
					const auto Position = Cluster.GetPosition(i);

					auto& IntPosition = Cluster.QuantizedPositions[i];
					float PosX = std::round(Position.x * QuantizationScale);
					float PosY = std::round(Position.y * QuantizationScale);
					float PosZ = std::round(Position.z * QuantizationScale);

					IntPosition = wm::int3((int32)PosX, (int32)PosY, (int32)PosZ);

					IntClusterMax.x = std::max(IntClusterMax.x, IntPosition.x);
					IntClusterMax.y = std::max(IntClusterMax.y, IntPosition.y);
					IntClusterMax.z = std::max(IntClusterMax.z, IntPosition.z);
					IntClusterMin.x = std::min(IntClusterMin.x, IntPosition.x);
					IntClusterMin.y = std::min(IntClusterMin.y, IntPosition.y);
					IntClusterMin.z = std::min(IntClusterMin.z, IntPosition.z);
				}

				// Store in minimum number of bits
				const uint32 NumBitsX = wm::CeilLog2(IntClusterMax.x - IntClusterMin.x + 1);
				const uint32 NumBitsY = wm::CeilLog2(IntClusterMax.y - IntClusterMin.y + 1);
				const uint32 NumBitsZ = wm::CeilLog2(IntClusterMax.z - IntClusterMin.z + 1);
				wconstraint(NumBitsX <= MAX_POSITION_QUANTIZATION_BITS);
				wconstraint(NumBitsY <= MAX_POSITION_QUANTIZATION_BITS);
				wconstraint(NumBitsZ <= MAX_POSITION_QUANTIZATION_BITS);

				for (uint32 i = 0; i < NumClusterVerts; i++)
				{
					auto& IntPosition = Cluster.QuantizedPositions[i];

					// Update float position with quantized data
					Cluster.GetPosition(i) = wm::float3(IntPosition.x * RcpQuantizationScale, IntPosition.y * RcpQuantizationScale, IntPosition.z * RcpQuantizationScale);

					IntPosition.x -= IntClusterMin.x;
					IntPosition.y -= IntClusterMin.y;
					IntPosition.z -= IntClusterMin.z;
					wconstraint(IntPosition.x >= 0 && IntPosition.x < (1 << NumBitsX));
					wconstraint(IntPosition.y >= 0 && IntPosition.y < (1 << NumBitsY));
					wconstraint(IntPosition.z >= 0 && IntPosition.z < (1 << NumBitsZ));
				}


				// Update bounds
				Cluster.Bounds.Min = wm::float3(IntClusterMin.x * RcpQuantizationScale, IntClusterMin.y * RcpQuantizationScale, IntClusterMin.z * RcpQuantizationScale);
				Cluster.Bounds.Max = wm::float3(IntClusterMax.x * RcpQuantizationScale, IntClusterMax.y * RcpQuantizationScale, IntClusterMax.z * RcpQuantizationScale);

				Cluster.MeshBoundsMin = {};
				Cluster.MeshBoundsDelta = wm::float3(RcpQuantizationScale, RcpQuantizationScale, RcpQuantizationScale);

				Cluster.QuantizedPosBits = wm::int3(NumBitsX, NumBitsY, NumBitsZ);
				Cluster.QuantizedPosStart = IntClusterMin;
				Cluster.QuantizedPosShift = 0;

			});
		return PositionPrecision;
	}

	static uint32 CalcMaterialTableSize(const Nanite::FCluster& InCluster)
	{
		uint32 NumMaterials = InCluster.MaterialRanges.size();
		return NumMaterials > 3 ? NumMaterials : 0;
	}

	static void CalculateEncodingInfo(FEncodingInfo& Info, const Nanite::FCluster& Cluster, bool bHasColors, uint32 NumTexCoords)
	{
		const uint32 NumClusterVerts = Cluster.NumVerts;
		const uint32 NumClusterTris = Cluster.NumTris;

		white::memset(Info, 0);

		// Write triangles indices. Indices are stored in a dense packed bitstream using ceil(log2(NumClusterVerices)) bits per index. The shaders implement unaligned bitstream reads to support this.
		const uint32 BitsPerIndex = NumClusterVerts > 1 ? (wm::FloorLog2(NumClusterVerts - 1) + 1) : 0;
		const uint32 BitsPerTriangle = BitsPerIndex + 2 * 5;	// Base index + two 5-bit offsets
		Info.BitsPerIndex = BitsPerIndex;

		FPageSections& GpuSizes = Info.GpuSizes;
		GpuSizes.Cluster = sizeof(FPackedCluster);
		GpuSizes.MaterialTable = CalcMaterialTableSize(Cluster) * sizeof(uint32);
		GpuSizes.DecodeInfo = NumTexCoords * sizeof(FUVRange);
		GpuSizes.Index = (NumClusterTris * BitsPerTriangle + 31) / 32 * 4;

		Info.BitsPerAttribute = 2 * NORMAL_QUANTIZATION_BITS;

		wconstraint(NumClusterVerts > 0);
		const bool bIsLeaf = (Cluster.GeneratingGroupIndex == INVALID_GROUP_INDEX);

		// Vertex colors
		Info.ColorMode = VERTEX_COLOR_MODE_WHITE;
		Info.ColorMin = wm::int4(255, 255, 255, 255);
		if (bHasColors)
		{
			auto ColorMin = wm::int4(255, 255, 255, 255);
			auto ColorMax = wm::int4(0, 0, 0, 0);
			for (uint32 i = 0; i < NumClusterVerts; i++)
			{
				FColor Color = as_color(Cluster.GetColor(i));
				ColorMin.x = std::min<int32>(ColorMin.x, (int32)Color.R);
				ColorMin.y = std::min<int32>(ColorMin.y, (int32)Color.G);
				ColorMin.z = std::min<int32>(ColorMin.z, (int32)Color.B);
				ColorMin.w = std::min<int32>(ColorMin.w, (int32)Color.A);
				ColorMax.x = std::max<int32>(ColorMax.x, (int32)Color.R);
				ColorMax.y = std::max<int32>(ColorMax.y, (int32)Color.G);
				ColorMax.z = std::max<int32>(ColorMax.z, (int32)Color.B);
				ColorMax.w = std::max<int32>(ColorMax.w, (int32)Color.A);
			}

			const wm::int4 ColorDelta = ColorMax - ColorMin;
			const int32 R_Bits = wm::CeilLog2(ColorDelta.x + 1);
			const int32 G_Bits = wm::CeilLog2(ColorDelta.y + 1);
			const int32 B_Bits = wm::CeilLog2(ColorDelta.z + 1);
			const int32 A_Bits = wm::CeilLog2(ColorDelta.w + 1);

			uint32 NumColorBits = R_Bits + G_Bits + B_Bits + A_Bits;
			Info.BitsPerAttribute += NumColorBits;
			Info.ColorMin = ColorMin;
			Info.ColorBits = wm::int4(R_Bits, G_Bits, B_Bits, A_Bits);
			if (NumColorBits > 0)
			{
				Info.ColorMode = VERTEX_COLOR_MODE_VARIABLE;
			}
			else
			{
				if (ColorMin.x == 255 && ColorMin.y == 255 && ColorMin.z == 255 && ColorMin.w == 255)
					Info.ColorMode = VERTEX_COLOR_MODE_WHITE;
				else
					Info.ColorMode = VERTEX_COLOR_MODE_CONSTANT;
			}
		}

		for (uint32 UVIndex = 0; UVIndex < NumTexCoords; UVIndex++)
		{
			FGeometryEncodingUVInfo& UVInfo = Info.UVInfos[UVIndex];
			// Block compress texture coordinates
			// Texture coordinates are stored relative to the clusters min/max UV coordinates.
			// UV seams result in very large sparse bounding rectangles. To mitigate this the largest gap in U and V of the bounding rectangle are excluded from the coding space.
			// Decoding this is very simple: UV += (UV >= GapStart) ? GapRange : 0;

			// Generate sorted U and V arrays.
			std::vector<float> UValues;
			std::vector<float> VValues;
			UValues.resize(NumClusterVerts);
			VValues.resize(NumClusterVerts);
			for (uint32 i = 0; i < NumClusterVerts; i++)
			{
				const auto& UV = Cluster.GetUVs(i)[UVIndex];
				UValues[i] = UV.x;
				VValues[i] = UV.y;
			}

			std::sort(UValues.begin(), UValues.end());
			std::sort(VValues.begin(), VValues.end());

			// Find largest gap between sorted UVs
			wm::float2 LargestGapStart{ UValues[0], VValues[0] };
			wm::float2 LargestGapEnd{ UValues[0], VValues[0] };
			for (uint32 i = 0; i < NumClusterVerts - 1; i++)
			{
				if (UValues[i + 1] - UValues[i] > LargestGapEnd.x - LargestGapStart.x)
				{
					LargestGapStart.x = UValues[i];
					LargestGapEnd.x = UValues[i + 1];
				}
				if (VValues[i + 1] - VValues[i] > LargestGapEnd.y - LargestGapStart.y)
				{
					LargestGapStart.y = VValues[i];
					LargestGapEnd.y = VValues[i + 1];
				}
			}

			const wm::float2 UVMin = wm::float2(UValues[0], VValues[0]);
			const wm::float2 UVMax = wm::float2(UValues[NumClusterVerts - 1], VValues[NumClusterVerts - 1]);
			const wm::float2 UVDelta = UVMax - UVMin;

			const wm::float2 UVRcpDelta = wm::float2(UVDelta.x > wm::SmallNumber ? 1.0f / UVDelta.x : 0.0f,
				UVDelta.y > wm::SmallNumber ? 1.0f / UVDelta.y : 0.0f);

			const wm::float2 NonGapLength = wm::max(UVDelta - (LargestGapEnd - LargestGapStart), wm::float2(0.0f, 0.0f));
			const wm::float2 NormalizedGapStart = (LargestGapStart - UVMin) * UVRcpDelta;
			const wm::float2 NormalizedGapEnd = (LargestGapEnd - UVMin) * UVRcpDelta;

			const wm::float2 NormalizedNonGapLength = NonGapLength * UVRcpDelta;

			const float TexCoordUnitPrecision = (1 << 14);	// TODO: Implement UI + 'Auto' mode that decides when this is necessary.

			int32 TexCoordBitsU = 0;
			if (UVDelta.x > 0)
			{
				int32 NumValues = std::max(wm::CeilToInt(NonGapLength.x * TexCoordUnitPrecision), 2);	// Even when NonGapLength=0, UVDelta is non-zero, so we need at least 2 values (1bit) to distinguish between high and low.
				TexCoordBitsU = std::min((int32)wm::CeilLog2(NumValues), 12);						// Limit to 12 bits which we know is good enough from temp hack below.
			}

			int32 TexCoordBitsV = 0;
			if (UVDelta.y > 0)
			{
				int32 NumValues = std::max(wm::CeilToInt(NonGapLength.y * TexCoordUnitPrecision), 2);
				TexCoordBitsV = std::min((int32)wm::CeilLog2(NumValues), 12);
			}


			Info.UVPrec |= ((TexCoordBitsV << 4) | TexCoordBitsU) << (UVIndex * 8);

			const int32 TexCoordMaxValueU = (1 << TexCoordBitsU) - 1;
			const int32 TexCoordMaxValueV = (1 << TexCoordBitsV) - 1;

			const int32 NU = (int32)white::clamp(NormalizedNonGapLength.x > wm::SmallNumber ? (TexCoordMaxValueU - 2) / NormalizedNonGapLength.x : 0.0f, (float)TexCoordMaxValueU, (float)0xFFFF);
			const int32 NV = (int32)white::clamp(NormalizedNonGapLength.y > wm::SmallNumber ? (TexCoordMaxValueV - 2) / NormalizedNonGapLength.y : 0.0f, (float)TexCoordMaxValueV, (float)0xFFFF);

			int32 GapStartU = TexCoordMaxValueU + 1;
			int32 GapStartV = TexCoordMaxValueV + 1;
			int32 GapLengthU = 0;
			int32 GapLengthV = 0;
			if (NU > TexCoordMaxValueU)
			{
				GapStartU = int32(NormalizedGapStart.x * NU + 0.5f) + 1;
				const int32 GapEndU = int32(NormalizedGapEnd.x * NU + 0.5f);
				GapLengthU = std::max(GapEndU - GapStartU, 0);
			}
			if (NV > TexCoordMaxValueV)
			{
				GapStartV = int32(NormalizedGapStart.y * NV + 0.5f) + 1;
				const int32 GapEndV = int32(NormalizedGapEnd.y * NV + 0.5f);
				GapLengthV = std::max(GapEndV - GapStartV, 0);
			}

			UVInfo.UVRange.Min = UVMin;
			UVInfo.UVRange.Scale = wm::float2(NU > 0 ? UVDelta.x / NU : 0.0f, NV > 0 ? UVDelta.y / NV : 0.0f);

			wconstraint(GapStartU >= 0);
			wconstraint(GapStartV >= 0);
			UVInfo.UVRange.GapStart[0] = GapStartU;
			UVInfo.UVRange.GapStart[1] = GapStartV;
			UVInfo.UVRange.GapLength[0] = GapLengthU;
			UVInfo.UVRange.GapLength[1] = GapLengthV;

			UVInfo.UVDelta = UVDelta;
			UVInfo.UVRcpDelta = UVRcpDelta;
			UVInfo.NU = NU;
			UVInfo.NV = NV;

			Info.BitsPerAttribute += TexCoordBitsU + TexCoordBitsV;
		}

		const uint32 PositionBitsPerVertex = Cluster.QuantizedPosBits.x + Cluster.QuantizedPosBits.y + Cluster.QuantizedPosBits.z;
		GpuSizes.Position = (NumClusterVerts * PositionBitsPerVertex + 31) / 32 * 4;
		GpuSizes.Attribute = (NumClusterVerts * Info.BitsPerAttribute + 31) / 32 * 4;

	}

	static void CalculateEncodingInfos(std::vector<FEncodingInfo>& EncodingInfos, const std::vector<Nanite::FCluster>& Clusters, bool bHasColors, uint32 NumTexCoords)
	{
		uint32 NumClusters = Clusters.size();
		EncodingInfos.resize(NumClusters);

		for (uint32 i = 0; i < NumClusters; i++)
		{
			CalculateEncodingInfo(EncodingInfos[i], Clusters[i], bHasColors, NumTexCoords);
		}
	}

	// Generate a permutation of cluster groups that is sorted first by mip level and then by Morton order x, y and z.
	// Sorting by mip level first ensure that there can be no cyclic dependencies between formed pages.
	static std::vector<uint32> CalculateClusterGroupPermutation(const std::vector< FClusterGroup >& ClusterGroups)
	{
		struct FClusterGroupSortEntry {
			int32	MipLevel;
			uint32	MortonXYZ;
			uint32	OldIndex;
		};

		uint32 NumClusterGroups = ClusterGroups.size();
		std::vector< FClusterGroupSortEntry > ClusterGroupSortEntries;
		ClusterGroupSortEntries.resize(NumClusterGroups);

		wm::float3 MinCenter{ FLT_MAX, FLT_MAX, FLT_MAX };
		wm::float3 MaxCenter{ -FLT_MAX, -FLT_MAX, -FLT_MAX };
		for (const FClusterGroup& ClusterGroup : ClusterGroups)
		{
			const auto& Center = ClusterGroup.LODBounds.Center;
			MinCenter = wm::min(MinCenter, Center);
			MaxCenter = wm::max(MaxCenter, Center);
		}

		for (uint32 i = 0; i < NumClusterGroups; i++)
		{
			const FClusterGroup& ClusterGroup = ClusterGroups[i];
			FClusterGroupSortEntry& SortEntry = ClusterGroupSortEntries[i];
			const auto& Center = ClusterGroup.LODBounds.Center;
			const auto ScaledCenter = (Center - MinCenter) / (MaxCenter - MinCenter) * 1023.0f + wm::float3(0.5f);
			uint32 X = wm::clamp((int32)ScaledCenter.x, 0, 1023);
			uint32 Y = wm::clamp((int32)ScaledCenter.y, 0, 1023);
			uint32 Z = wm::clamp((int32)ScaledCenter.z, 0, 1023);

			SortEntry.MipLevel = ClusterGroup.MipLevel;
			SortEntry.MortonXYZ = (wm::MortonCode3(Z) << 2) | (wm::MortonCode3(Y) << 1) | wm::MortonCode3(X);
			SortEntry.OldIndex = i;
		}

		std::sort(ClusterGroupSortEntries.begin(), ClusterGroupSortEntries.end(), [](const FClusterGroupSortEntry& A, const FClusterGroupSortEntry& B) {
			if (A.MipLevel != B.MipLevel)
				return A.MipLevel > B.MipLevel;
			return A.MortonXYZ < B.MortonXYZ;
			});

		std::vector<uint32> Permutation;
		Permutation.resize(NumClusterGroups);
		for (uint32 i = 0; i < NumClusterGroups; i++)
			Permutation[i] = ClusterGroupSortEntries[i].OldIndex;
		return Permutation;
	}

	static void SortGroupClusters(std::vector<FClusterGroup>& ClusterGroups, const std::vector<FCluster>& Clusters)
	{
		for (FClusterGroup& Group : ClusterGroups)
		{
			wm::float3 SortDirection = { 1.0f, 1.0f, 1.0f };
			std::sort(Group.Children.begin(), Group.Children.end(), [&Clusters, SortDirection](uint32 ClusterIndexA, uint32 ClusterIndexB) {
				const FCluster& ClusterA = Clusters[ClusterIndexA];
				const FCluster& ClusterB = Clusters[ClusterIndexB];
				float DotA = wm::dot(ClusterA.SphereBounds.Center, SortDirection);
				float DotB = wm::dot(ClusterB.SphereBounds.Center, SortDirection);
				return DotA < DotB;
				});
		}
	}

	/*
	Build streaming pages
	Page layout:
		Fixup Chunk (Only loaded to CPU memory)
		FPackedCluster
		MaterialRangeTable
		GeometryData
	*/

	static void AssignClustersToPages(
		std::vector< FClusterGroup >& ClusterGroups,
		std::vector< FCluster >& Clusters,
		const std::vector< FEncodingInfo >& EncodingInfos,
		std::vector<FPage>& Pages,
		std::vector<FClusterGroupPart>& Parts
	)
	{
		wconstraint(Pages.empty());
		wconstraint(Parts.empty());

		const uint32 NumClusterGroups = ClusterGroups.size();
		Pages.push_back({});

		SortGroupClusters(ClusterGroups, Clusters);
		std::vector<uint32> ClusterGroupPermutation = CalculateClusterGroupPermutation(ClusterGroups);

		for (uint32 i = 0; i < NumClusterGroups; i++)
		{
			// Pick best next group			// TODO
			uint32 GroupIndex = ClusterGroupPermutation[i];
			FClusterGroup& Group = ClusterGroups[GroupIndex];
			uint32 GroupStartPage = INVALID_PAGE_INDEX;

			for (uint32 ClusterIndex : Group.Children)
			{
				// Pick best next cluster		// TODO
				FCluster& Cluster = Clusters[ClusterIndex];
				const FEncodingInfo& EncodingInfo = EncodingInfos[ClusterIndex];

				// Add to page
				FPage* Page = &Pages.back();
				if (Page->GpuSizes.GetTotal() + EncodingInfo.GpuSizes.GetTotal() > CLUSTER_PAGE_GPU_SIZE || Page->NumClusters + 1 > MAX_CLUSTERS_PER_PAGE)
				{
					// Page is full. Need to start a new one
					Pages.push_back({});
					Page = &Pages.back();
				}

				// Start a new part?
				if (Page->PartsNum == 0 || Parts[Page->PartsStartIndex + Page->PartsNum - 1].GroupIndex != GroupIndex)
				{
					if (Page->PartsNum == 0)
					{
						Page->PartsStartIndex = Parts.size();
					}
					Page->PartsNum++;

					FClusterGroupPart& Part = Parts.emplace_back();
					Part.GroupIndex = GroupIndex;
				}

				// Add cluster to page
				uint32 PageIndex = Pages.size() - 1;
				uint32 PartIndex = Parts.size() - 1;

				FClusterGroupPart& Part = Parts.back();
				if (Part.Clusters.empty())
				{
					Part.PageClusterOffset = Page->NumClusters;
					Part.PageIndex = PageIndex;
				}
				Part.Clusters.emplace_back(ClusterIndex);
				wconstraint(Part.Clusters.size() <= MAX_CLUSTERS_PER_GROUP);

				Cluster.GroupPartIndex = PartIndex;

				if (GroupStartPage == INVALID_PAGE_INDEX)
				{
					GroupStartPage = PageIndex;
				}

				Page->GpuSizes += EncodingInfo.GpuSizes;
				Page->NumClusters++;
			}

			Group.PageIndexStart = GroupStartPage;
			Group.PageIndexNum = Pages.size() - GroupStartPage;
			wconstraint(Group.PageIndexNum >= 1);
			wconstraint(Group.PageIndexNum <= MAX_GROUP_PARTS_MASK);
		}

		// Recalculate bounds for group parts
		for (FClusterGroupPart& Part : Parts)
		{
			wconstraint(Part.Clusters.size() <= MAX_CLUSTERS_PER_GROUP);
			wconstraint(Part.PageIndex < (uint32)Pages.size());

			FBounds Bounds;
			for (uint32 ClusterIndex : Part.Clusters)
			{
				Bounds += Clusters[ClusterIndex].Bounds;
			}
			Part.Bounds = Bounds;
		}
	}

	struct FIntermediateNode
	{
		uint32				PartIndex = wm::MAX_uint32;
		uint32				MipLevel = wm::MAX_int32;
		bool				bLeaf = false;

		FBounds				Bound;
		std::vector< uint32 >	Children;
	};

	static float BVH_Cost(const std::vector<FIntermediateNode>& Nodes, white::span<uint32> NodeIndices)
	{
		FBounds Bound;
		for (uint32 NodeIndex : NodeIndices)
		{
			Bound += Nodes[NodeIndex].Bound;
		}
		return Bound.GetSurfaceArea();
	}

	static void BVH_SortNodes(const std::vector<FIntermediateNode>& Nodes, white::span<uint32> NodeIndices, std::vector<uint32>& ChildSizes)
	{
		// Perform MAX_BVH_NODE_FANOUT_BITS binary splits
		for (uint32 Level = 0; Level < MAX_BVH_NODE_FANOUT_BITS; Level++)
		{
			const uint32 NumBuckets = 1 << Level;
			const uint32 NumChildrenPerBucket = MAX_BVH_NODE_FANOUT >> Level;
			const uint32 NumChildrenPerBucketHalf = NumChildrenPerBucket >> 1;

			uint32 BucketStartIndex = 0;
			for (uint32 BucketIndex = 0; BucketIndex < NumBuckets; BucketIndex++)
			{
				const uint32 FirstChild = NumChildrenPerBucket * BucketIndex;

				uint32 Sizes[2] = {};
				for (uint32 i = 0; i < NumChildrenPerBucketHalf; i++)
				{
					Sizes[0] += ChildSizes[FirstChild + i];
					Sizes[1] += ChildSizes[FirstChild + i + NumChildrenPerBucketHalf];
				}
				auto NodeIndices01 = NodeIndices.subspan(BucketStartIndex, Sizes[0] + Sizes[1]);
				auto NodeIndices0 = NodeIndices.subspan(BucketStartIndex, Sizes[0]);
				auto NodeIndices1 = NodeIndices.subspan(BucketStartIndex + Sizes[0], Sizes[1]);

				BucketStartIndex += Sizes[0] + Sizes[1];

				auto SortByAxis = [&](uint32 AxisIndex)
				{
					if (AxisIndex == 0)
						std::sort(NodeIndices01.begin(), NodeIndices01.end(), [&Nodes](uint32 A, uint32 B) { return Nodes[A].Bound.GetCenter().x < Nodes[B].Bound.GetCenter().x; });
					else if (AxisIndex == 1)
						std::sort(NodeIndices01.begin(), NodeIndices01.end(), [&Nodes](uint32 A, uint32 B) { return Nodes[A].Bound.GetCenter().y < Nodes[B].Bound.GetCenter().y; });
					else if (AxisIndex == 2)
						std::sort(NodeIndices01.begin(), NodeIndices01.end(), [&Nodes](uint32 A, uint32 B) { return Nodes[A].Bound.GetCenter().z < Nodes[B].Bound.GetCenter().z; });
					else
						wconstraint(false);
				};

				float BestCost = wm::MAX_flt;
				uint32 BestAxisIndex = 0;

				// Try sorting along different axes and pick the best one
				const uint32 NumAxes = 3;
				for (uint32 AxisIndex = 0; AxisIndex < NumAxes; AxisIndex++)
				{
					SortByAxis(AxisIndex);

					float Cost = BVH_Cost(Nodes, NodeIndices0) + BVH_Cost(Nodes, NodeIndices1);
					if (Cost < BestCost)
					{
						BestCost = Cost;
						BestAxisIndex = AxisIndex;
					}
				}

				// Resort if we the best one wasn't the last one
				if (BestAxisIndex != NumAxes - 1)
				{
					SortByAxis(BestAxisIndex);
				}
			}
		}
	}

	// Build hierarchy using a top-down splitting approach.
	// WIP:	So far it just focuses on minimizing worst-case tree depth/latency.
	//		It does this by building a complete tree with at most one partially filled level.
	//		At most one node is partially filled.
	//TODO:	Experiment with sweeping, even if it results in more total nodes and/or makes some paths slightly longer.
	static uint32 BuildHierarchyTopDown(std::vector<FIntermediateNode>& Nodes, white::span<uint32> NodeIndices, bool bSort)
	{
		const uint32 N = NodeIndices.size();
		if (N == 1)
		{
			return NodeIndices[0];
		}

		const uint32 NewRootIndex = Nodes.size();
		Nodes.emplace_back();

		if (N <= MAX_BVH_NODE_FANOUT)
		{
			Nodes[NewRootIndex].Children.assign(NodeIndices.begin(), NodeIndices.end());
			return NewRootIndex;
		}

		// Where does the last (incomplete) level start
		uint32 TopSize = MAX_BVH_NODE_FANOUT;
		while (TopSize * MAX_BVH_NODE_FANOUT <= N)
		{
			TopSize *= MAX_BVH_NODE_FANOUT;
		}

		const uint32 LargeChildSize = TopSize;
		const uint32 SmallChildSize = TopSize / MAX_BVH_NODE_FANOUT;
		const uint32 MaxExcessPerChild = LargeChildSize - SmallChildSize;

		std::vector<uint32> ChildSizes;
		ChildSizes.resize(MAX_BVH_NODE_FANOUT);

		uint32 Excess = N - TopSize;
		for (int32 i = MAX_BVH_NODE_FANOUT - 1; i >= 0; i--)
		{
			const uint32 ChildExcess = std::min(Excess, MaxExcessPerChild);
			ChildSizes[i] = SmallChildSize + ChildExcess;
			Excess -= ChildExcess;
		}
		wconstraint(Excess == 0);

		if (bSort)
		{
			BVH_SortNodes(Nodes, NodeIndices, ChildSizes);
		}

		uint32 Offset = 0;
		for (uint32 i = 0; i < MAX_BVH_NODE_FANOUT; i++)
		{
			uint32 ChildSize = ChildSizes[i];
			Nodes[NewRootIndex].Children.emplace_back(BuildHierarchyTopDown(Nodes, NodeIndices.subspan(Offset, ChildSize), bSort));
			Offset += ChildSize;
		}

		return NewRootIndex;
	}

	struct FHierarchyNode
	{
		Sphere			LODBounds[MAX_BVH_NODE_FANOUT];
		FBounds			Bounds[MAX_BVH_NODE_FANOUT];
		float			MinLODErrors[MAX_BVH_NODE_FANOUT];
		float			MaxParentLODErrors[MAX_BVH_NODE_FANOUT];
		uint32			ChildrenStartIndex[MAX_BVH_NODE_FANOUT];
		uint32			NumChildren[MAX_BVH_NODE_FANOUT];
		uint32			ClusterGroupPartIndex[MAX_BVH_NODE_FANOUT];
	};

	static uint32 BuildHierarchyRecursive(std::vector<FHierarchyNode>& HierarchyNodes, const std::vector<FIntermediateNode>& Nodes, const std::vector<FClusterGroup>& Groups, std::vector<Nanite::FClusterGroupPart>& Parts, uint32 CurrentNodeIndex)
	{
		const FIntermediateNode& INode = Nodes[CurrentNodeIndex];
		wconstraint(INode.PartIndex == wm::MAX_uint32);
		wconstraint(!INode.bLeaf);

		uint32 HNodeIndex = HierarchyNodes.size();
		white::memset(HierarchyNodes.emplace_back(), 0);

		uint32 NumChildren = INode.Children.size();
		wconstraint(NumChildren > 0 && NumChildren <= MAX_BVH_NODE_FANOUT);
		for (uint32 ChildIndex = 0; ChildIndex < NumChildren; ChildIndex++)
		{
			uint32 ChildNodeIndex = INode.Children[ChildIndex];
			const FIntermediateNode& ChildNode = Nodes[ChildNodeIndex];
			if (ChildNode.bLeaf)
			{
				// Cluster Group
				wconstraint(ChildNode.bLeaf);
				FClusterGroupPart& Part = Parts[ChildNode.PartIndex];
				const FClusterGroup& Group = Groups[Part.GroupIndex];

				FHierarchyNode& HNode = HierarchyNodes[HNodeIndex];
				HNode.Bounds[ChildIndex] = Part.Bounds;
				HNode.LODBounds[ChildIndex] = Group.LODBounds;
				HNode.MinLODErrors[ChildIndex] = Group.MinLODError;
				HNode.MaxParentLODErrors[ChildIndex] = Group.MaxParentLODError;
				HNode.ChildrenStartIndex[ChildIndex] = 0xFFFFFFFFu;
				HNode.NumChildren[ChildIndex] = Part.Clusters.size();
				HNode.ClusterGroupPartIndex[ChildIndex] = ChildNode.PartIndex;

				wconstraint(HNode.NumChildren[ChildIndex] <= MAX_CLUSTERS_PER_GROUP);
				Part.HierarchyNodeIndex = HNodeIndex;
				Part.HierarchyChildIndex = ChildIndex;
			}
			else
			{

				// Hierarchy node
				uint32 ChildHierarchyNodeIndex = BuildHierarchyRecursive(HierarchyNodes, Nodes, Groups, Parts, ChildNodeIndex);

				const Nanite::FHierarchyNode& ChildHNode = HierarchyNodes[ChildHierarchyNodeIndex];

				FBounds Bounds;
				std::vector<Sphere> LODBoundSpheres;
				float MinLODError = wm::MAX_flt;
				float MaxParentLODError = 0.0f;
				for (uint32 GrandChildIndex = 0; GrandChildIndex < MAX_BVH_NODE_FANOUT && ChildHNode.NumChildren[GrandChildIndex] != 0; GrandChildIndex++)
				{
					Bounds += ChildHNode.Bounds[GrandChildIndex];
					LODBoundSpheres.emplace_back(ChildHNode.LODBounds[GrandChildIndex]);
					MinLODError = std::min(MinLODError, ChildHNode.MinLODErrors[GrandChildIndex]);
					MaxParentLODError = std::max(MaxParentLODError, ChildHNode.MaxParentLODErrors[GrandChildIndex]);
				}

				Sphere LODBounds{ LODBoundSpheres.data(),static_cast<int32>(LODBoundSpheres.size()) };

				Nanite::FHierarchyNode& HNode = HierarchyNodes[HNodeIndex];
				HNode.Bounds[ChildIndex] = Bounds;
				HNode.LODBounds[ChildIndex] = LODBounds;
				HNode.MinLODErrors[ChildIndex] = MinLODError;
				HNode.MaxParentLODErrors[ChildIndex] = MaxParentLODError;
				HNode.ChildrenStartIndex[ChildIndex] = ChildHierarchyNodeIndex;
				HNode.NumChildren[ChildIndex] = MAX_CLUSTERS_PER_GROUP;
				HNode.ClusterGroupPartIndex[ChildIndex] = INVALID_GROUP_INDEX;
			}
		}

		return HNodeIndex;
	}

	void RemoveRootPagesFromRange(uint32& StartPage, uint32& NumPages)
	{
		uint32_t NumRootPages = StartPage < NUM_ROOT_PAGES ? (NUM_ROOT_PAGES - StartPage) : 0u;
		if (NumRootPages > 0u)
		{
			NumRootPages = std::min(NumRootPages, NumPages);
			StartPage += NumRootPages;
			NumPages -= NumRootPages;
		}

		if (NumPages == 0)
		{
			StartPage = 0;
		}
	}

	static void PackHierarchyNode(Nanite::FPackedHierarchyNode& OutNode, const FHierarchyNode& InNode, const std::vector<FClusterGroup>& Groups, const std::vector<FClusterGroupPart>& GroupParts)
	{
		static_assert(MAX_RESOURCE_PAGES_BITS + MAX_CLUSTERS_PER_GROUP_BITS + MAX_GROUP_PARTS_BITS <= 32, "");
		for (uint32 i = 0; i < MAX_BVH_NODE_FANOUT; i++)
		{
			OutNode.LODBounds[i] = InNode.LODBounds[i];

			const FBounds& Bounds = InNode.Bounds[i];
			OutNode.Misc0[i].BoxBoundsCenter = Bounds.GetCenter();
			OutNode.Misc1[i].BoxBoundsExtent = Bounds.GetExtent();

			wconstraint(InNode.NumChildren[i] <= MAX_CLUSTERS_PER_GROUP);
			OutNode.Misc0[i].MinLODError_MaxParentLODError = wm::half(InNode.MinLODErrors[i]).data | (wm::half(InNode.MaxParentLODErrors[i]).data << 16);
			OutNode.Misc1[i].ChildStartReference = InNode.ChildrenStartIndex[i];

			uint32 ResourcePageIndex_NumPages_GroupPartSize = 0;
			if (InNode.NumChildren[i] > 0)
			{
				if (InNode.ClusterGroupPartIndex[i] != INVALID_PART_INDEX)
				{
					// Leaf node
					const FClusterGroup& Group = Groups[GroupParts[InNode.ClusterGroupPartIndex[i]].GroupIndex];
					uint32 GroupPartSize = InNode.NumChildren[i];

					// If group spans multiple pages, request all of them, except the root pages
					uint32 PageIndexStart = Group.PageIndexStart;
					uint32 PageIndexNum = Group.PageIndexNum;
					RemoveRootPagesFromRange(PageIndexStart, PageIndexNum);
					ResourcePageIndex_NumPages_GroupPartSize = (PageIndexStart << (MAX_CLUSTERS_PER_GROUP_BITS + MAX_GROUP_PARTS_BITS)) | (PageIndexNum << MAX_CLUSTERS_PER_GROUP_BITS) | GroupPartSize;
				}
				else
				{
					// Hierarchy node. No resource page or group size.
					ResourcePageIndex_NumPages_GroupPartSize = 0xFFFFFFFFu;
				}
			}
			OutNode.Misc2[i].ResourcePageIndex_NumPages_GroupPartSize = ResourcePageIndex_NumPages_GroupPartSize;
		}
	}

	static void BuildHierarchies(Resources& Resources, const std::vector<FClusterGroup>& Groups, std::vector<FClusterGroupPart>& Parts, uint32 NumMeshes)
	{
		std::vector<std::vector<uint32>> PartsByMesh;
		PartsByMesh.resize(NumMeshes);

		// Assign group parts to the meshes they belong to
		const uint32 NumTotalParts = Parts.size();
		for (uint32 PartIndex = 0; PartIndex < NumTotalParts; PartIndex++)
		{
			FClusterGroupPart& Part = Parts[PartIndex];
			PartsByMesh[Groups[Part.GroupIndex].MeshIndex].emplace_back(PartIndex);
		}

		for (uint32 MeshIndex = 0; MeshIndex < NumMeshes; MeshIndex++)
		{
			const std::vector<uint32>& PartIndices = PartsByMesh[MeshIndex];
			const uint32 NumParts = PartIndices.size();

			int32 MaxMipLevel = 0;
			for (uint32 i = 0; i < NumParts; i++)
			{
				MaxMipLevel = std::max(MaxMipLevel, Groups[Parts[PartIndices[i]].GroupIndex].MipLevel);
			}

			std::vector< FIntermediateNode >	Nodes;
			Nodes.resize(NumParts);

			// Build leaf nodes for each LOD level of the mesh
			std::vector<std::vector<uint32>> NodesByMip;
			NodesByMip.resize(MaxMipLevel + 1);
			for (uint32 i = 0; i < NumParts; i++)
			{
				const uint32 PartIndex = PartIndices[i];
				const FClusterGroupPart& Part = Parts[PartIndex];
				const FClusterGroup& Group = Groups[Part.GroupIndex];

				const int32 MipLevel = Group.MipLevel;
				FIntermediateNode& Node = Nodes[i];
				Node.Bound = Part.Bounds;
				Node.PartIndex = PartIndex;
				Node.MipLevel = Group.MipLevel;
				Node.bLeaf = true;
				NodesByMip[Group.MipLevel].emplace_back(i);
			}


			uint32 RootIndex = 0;
			if (Nodes.size() == 1)
			{
				// Just a single leaf.
				// Needs to be special-cased as root should always be an inner node.
				FIntermediateNode& Node = Nodes.emplace_back();
				Node.Children.emplace_back(0);
				Node.Bound = Nodes[0].Bound;
				RootIndex = 1;
			}
			else
			{
				// Build hierarchy:
				// Nanite meshes contain cluster data for many levels of detail. Clusters from different levels
				// of detail can vary wildly in size, which can already be challenge for building a good hierarchy. 
				// Apart from the visibility bounds, the hierarchy also tracks conservative LOD error metrics for the child nodes.
				// The runtime traversal descends into children as long as they are visible and the conservative LOD error is not
				// more detailed than what we are looking for. We have to be very careful when mixing clusters from different LODs
				// as less detailed clusters can easily end up bloating both bounds and error metrics.

				// We have experimented with a bunch of mixed LOD approached, but currently, it seems, building separate hierarchies
				// for each LOD level and then building a hierarchy of those hierarchies gives the best and most predictable results.

				// TODO: The roots of these hierarchies all share the same visibility and LOD bounds, or at least close enough that we could
				//       make a shared conservative bound without losing much. This makes a lot of the work around the root node fairly
				//       redundant. Perhaps we should consider evaluating a shared root during instance cull instead and enable/disable
				//       the per-level hierarchies based on 1D range tests for LOD error.

				std::vector<uint32> LevelRoots;
				for (int32 MipLevel = 0; MipLevel <= MaxMipLevel; MipLevel++)
				{
					if (NodesByMip[MipLevel].size() > 0)
					{
						// Build a hierarchy for the mip level
						uint32 NodeIndex = BuildHierarchyTopDown(Nodes, white::make_span(NodesByMip[MipLevel]), true);

						if (Nodes[NodeIndex].bLeaf || Nodes[NodeIndex].Children.size() == MAX_BVH_NODE_FANOUT)
						{
							// Leaf or filled node. Just add it.
							LevelRoots.emplace_back(NodeIndex);
						}
						else
						{
							// Incomplete node. Discard the code and add the children as roots instead.
							LevelRoots.insert(LevelRoots.end(), Nodes[NodeIndex].Children.begin(), Nodes[NodeIndex].Children.end());
						}
					}
				}
				// Build top hierarchy. A hierarchy of MIP hierarchies.
				RootIndex = BuildHierarchyTopDown(Nodes, white::make_span(LevelRoots), false);
			}

			wconstraint(Nodes.size() > 0);

#if BVH_BUILD_WRITE_GRAPHVIZ
			WriteDotGraph(Nodes);
#endif

			std::vector< FHierarchyNode > HierarchyNodes;
			BuildHierarchyRecursive(HierarchyNodes, Nodes, Groups, Parts, RootIndex);

			// Convert hierarchy to packed format
			const uint32 NumHierarchyNodes = HierarchyNodes.size();
			const uint32 PackedBaseIndex = Resources.HierarchyNodes.size();
			Resources.HierarchyRootOffsets.emplace_back(PackedBaseIndex);
			Resources.HierarchyNodes.resize(Resources.HierarchyNodes.size() + NumHierarchyNodes);
			for (uint32 i = 0; i < NumHierarchyNodes; i++)
			{
				PackHierarchyNode(Resources.HierarchyNodes[PackedBaseIndex + i], HierarchyNodes[i], Groups, Parts);
			}
		}
	}

	bool IsRootPage(uint32 PageIndex)
	{
		return PageIndex < NUM_ROOT_PAGES;
	}

	struct FVariableVertex
	{
		const float* Data;
		uint32			SizeInBytes;

		bool operator==(FVariableVertex Other) const
		{
			return 0 == std::memcmp(Data, Other.Data, SizeInBytes);
		}
	};

	struct FVariableVertexHash
	{
		std::size_t operator()(FVariableVertex Vert) const
		{
			return CityHash32((const char*)Vert.Data, Vert.SizeInBytes);
		}
	};

	static void PackCluster(Nanite::FPackedCluster& OutCluster, const Nanite::FCluster& InCluster, const FEncodingInfo& EncodingInfo, uint32 NumTexCoords)
	{
		white::memset(OutCluster, 0);

		// 0
		OutCluster.QuantizedPosStart = InCluster.QuantizedPosStart;
		OutCluster.SetNumVerts(InCluster.NumVerts);
		OutCluster.SetPositionOffset(0);

		// 1
		OutCluster.MeshBoundsMin = InCluster.MeshBoundsMin;
		OutCluster.SetNumTris(InCluster.NumTris);
		OutCluster.SetIndexOffset(0);

		// 2
		OutCluster.MeshBoundsDelta = InCluster.MeshBoundsDelta;
		OutCluster.SetBitsPerIndex(EncodingInfo.BitsPerIndex);
		OutCluster.SetQuantizedPosShift(InCluster.QuantizedPosShift);
		OutCluster.SetPosBitsX(InCluster.QuantizedPosBits.x);
		OutCluster.SetPosBitsY(InCluster.QuantizedPosBits.y);
		OutCluster.SetPosBitsZ(InCluster.QuantizedPosBits.z);

		// 3
		OutCluster.LODBounds = InCluster.LODBounds;

		// 4
		OutCluster.BoxBoundsCenter = (InCluster.Bounds.Min + InCluster.Bounds.Max) * 0.5f;
		OutCluster.LODErrorAndEdgeLength = wm::half(InCluster.LODError).data | (wm::half(InCluster.EdgeLength).data << 16);

		// 5
		OutCluster.BoxBoundsExtent = (InCluster.Bounds.Max - InCluster.Bounds.Min) * 0.5f;
		OutCluster.Flags = NANITE_CLUSTER_FLAG_LEAF;

		// 6
		wconstraint(NumTexCoords <= MAX_NANITE_UVS);
		static_assert(MAX_NANITE_UVS <= 4, "UV_Prev encoding only supports up to 4 channels");

		OutCluster.SetBitsPerAttribute(EncodingInfo.BitsPerAttribute);
		OutCluster.SetNumUVs(NumTexCoords);
		OutCluster.SetColorMode(EncodingInfo.ColorMode);
		OutCluster.UV_Prec = EncodingInfo.UVPrec;
		OutCluster.PackedMaterialInfo = 0;	// Filled out by WritePages

		// 7
		OutCluster.ColorMin = EncodingInfo.ColorMin.x | (EncodingInfo.ColorMin.y << 8) | (EncodingInfo.ColorMin.z << 16) | (EncodingInfo.ColorMin.w << 24);
		OutCluster.ColorBits = EncodingInfo.ColorBits.x | (EncodingInfo.ColorBits.y << 4) | (EncodingInfo.ColorBits.z << 8) | (EncodingInfo.ColorBits.w << 12);
		OutCluster.GroupIndex = InCluster.GroupIndex;
		OutCluster.Pad0 = 0;
	}

	static uint32 PackMaterialTableRange(uint32 TriStart, uint32 TriLength, uint32 MaterialIndex)
	{
		uint32 Packed = 0x00000000;
		// uint32 TriStart      :  8; // max 128 triangles
		// uint32 TriLength     :  8; // max 128 triangles
		// uint32 MaterialIndex :  6; // max  64 materials
		// uint32 Padding       : 10;
		wconstraint(TriStart <= 128);
		wconstraint(TriLength <= 128);
		wconstraint(MaterialIndex < 64);
		Packed |= TriStart;
		Packed |= TriLength << 8;
		Packed |= MaterialIndex << 16;
		return Packed;
	}

	static uint32 PackMaterialFastPath(uint32 Material0Length, uint32 Material0Index, uint32 Material1Length, uint32 Material1Index, uint32 Material2Index)
	{
		uint32 Packed = 0x00000000;
		// Material Packed Range - Fast Path (32 bits)
		// uint Material0Index  : 6;   // max  64 materials (0:Material0Length)
		// uint Material1Index  : 6;   // max  64 materials (Material0Length:Material1Length)
		// uint Material2Index  : 6;   // max  64 materials (remainder)
		// uint Material0Length : 7;   // max 128 triangles (num minus one)
		// uint Material1Length : 7;   // max  64 triangles (materials are sorted, so at most 128/2)
		wconstraint(Material0Index < 64);
		wconstraint(Material1Index < 64);
		wconstraint(Material2Index < 64);
		wconstraint(Material0Length >= 1);
		wconstraint(Material0Length <= 128);
		wconstraint(Material1Length <= 64);
		wconstraint(Material1Length <= Material0Length);
		Packed |= Material0Index;
		Packed |= Material1Index << 6;
		Packed |= Material2Index << 12;
		Packed |= (Material0Length - 1u) << 18;
		Packed |= Material1Length << 25;
		return Packed;
	}

	static uint32 PackMaterialSlowPath(uint32 MaterialTableOffset, uint32 MaterialTableLength)
	{
		// Material Packed Range - Slow Path (32 bits)
		// uint BufferIndex     : 19; // 2^19 max value (tons, it's per prim)
		// uint BufferLength	: 6;  // max 64 materials, so also at most 64 ranges (num minus one)
		// uint Padding			: 7;  // always 127 for slow path. corresponds to Material1Length=127 in fast path
		wconstraint(MaterialTableOffset < 524288); // 2^19 - 1
		wconstraint(MaterialTableLength > 0); // clusters with 0 materials use fast path
		wconstraint(MaterialTableLength <= 64);
		uint32 Packed = MaterialTableOffset;
		Packed |= (MaterialTableLength - 1u) << 19;
		Packed |= (0xFE000000u);
		return Packed;
	}

	static uint32 PackMaterialInfo(const Nanite::FCluster& InCluster, std::vector<uint32>& OutMaterialTable, uint32 MaterialTableStartOffset)
	{
		// Encode material ranges
		uint32 NumMaterialTriangles = 0;
		for (int32 RangeIndex = 0; RangeIndex < InCluster.MaterialRanges.size(); ++RangeIndex)
		{
			wconstraint(InCluster.MaterialRanges[RangeIndex].RangeLength <= 128);
			wconstraint(InCluster.MaterialRanges[RangeIndex].RangeLength > 0);
			wconstraint(InCluster.MaterialRanges[RangeIndex].MaterialIndex < MAX_CLUSTER_MATERIALS);
			NumMaterialTriangles += InCluster.MaterialRanges[RangeIndex].RangeLength;
		}

		// All triangles accounted for in material ranges?
		wconstraint(NumMaterialTriangles == InCluster.NumTris);

		uint32 PackedMaterialInfo = 0x00000000;

		// The fast inline path can encode up to 3 materials
		if (InCluster.MaterialRanges.size() <= 3)
		{
			uint32 Material0Length = 0;
			uint32 Material0Index = 0;
			uint32 Material1Length = 0;
			uint32 Material1Index = 0;
			uint32 Material2Index = 0;

			if (InCluster.MaterialRanges.size() > 0)
			{
				const FMaterialRange& Material0 = InCluster.MaterialRanges[0];
				wconstraint(Material0.RangeStart == 0);
				Material0Length = Material0.RangeLength;
				Material0Index = Material0.MaterialIndex;
			}

			if (InCluster.MaterialRanges.size() > 1)
			{
				const FMaterialRange& Material1 = InCluster.MaterialRanges[1];
				wconstraint(Material1.RangeStart == InCluster.MaterialRanges[0].RangeLength);
				Material1Length = Material1.RangeLength;
				Material1Index = Material1.MaterialIndex;
			}

			if (InCluster.MaterialRanges.size() > 2)
			{
				const FMaterialRange& Material2 = InCluster.MaterialRanges[2];
				wconstraint(Material2.RangeStart == Material0Length + Material1Length);
				wconstraint(Material2.RangeLength == InCluster.NumTris - Material0Length - Material1Length);
				Material2Index = Material2.MaterialIndex;
			}

			PackedMaterialInfo = PackMaterialFastPath(Material0Length, Material0Index, Material1Length, Material1Index, Material2Index);
		}
		// Slow global table search path
		else
		{
			uint32 MaterialTableOffset = OutMaterialTable.size() + MaterialTableStartOffset;
			uint32 MaterialTableLength = InCluster.MaterialRanges.size();
			wconstraint(MaterialTableLength > 0);

			for (int32 RangeIndex = 0; RangeIndex < InCluster.MaterialRanges.size(); ++RangeIndex)
			{
				const FMaterialRange& Material = InCluster.MaterialRanges[RangeIndex];
				OutMaterialTable.emplace_back(PackMaterialTableRange(Material.RangeStart, Material.RangeLength, Material.MaterialIndex));
			}

			PackedMaterialInfo = PackMaterialSlowPath(MaterialTableOffset, MaterialTableLength);
		}

		return PackedMaterialInfo;
	}

	class FBlockPointer
	{
		uint8* StartPtr;
		uint8* EndPtr;
		uint8* Ptr;
	public:
		FBlockPointer(uint8* Ptr, uint32 SizeInBytes) :
			StartPtr(Ptr), EndPtr(Ptr + SizeInBytes), Ptr(Ptr)
		{
		}

		template<typename T>
		T* Advance(uint32 Num)
		{
			T* Result = (T*)Ptr;
			Ptr += Num * sizeof(T);
			wconstraint(Ptr <= EndPtr);
			return Result;
		}

		template<typename T>
		T* GetPtr() const { return (T*)Ptr; }

		uint32 Offset() const
		{
			return uint32(Ptr - StartPtr);
		}

		void Align(uint32 Alignment)
		{
			while (Offset() % Alignment)
			{
				*Advance<uint8>(1) = 0;
			}
		}
	};

	static wm::float2 OctahedronEncode(wm::float3 N)
	{
		auto AbsN = wm::abs(N);
		N /= (AbsN.x + AbsN.y + AbsN.z);

		if (N.z < 0.0)
		{
			AbsN = wm::abs(N);
			N.x = (N.x >= 0.0f) ? (1.0f - AbsN.y) : (AbsN.y - 1.0f);
			N.y = (N.y >= 0.0f) ? (1.0f - AbsN.x) : (AbsN.x - 1.0f);
		}

		return { N.x, N.y };
	}

	FORCEINLINE static wm::float3 OctahedronDecode(int32 X, int32 Y, int32 QuantizationBits)
	{
		const int32 QuantizationMaxValue = (1 << QuantizationBits) - 1;
		float fx = X * (2.0f / QuantizationMaxValue) - 1.0f;
		float fy = Y * (2.0f / QuantizationMaxValue) - 1.0f;
		float fz = 1.0f - std::abs(fx) - std::abs(fy);
		float t = std::clamp(-fz, 0.0f, 1.0f);
		fx += (fx >= 0.0f ? -t : t);
		fy += (fy >= 0.0f ? -t : t);

		return wm::normalize(wm::float3(fx, fy, fz));
	}

	static void OctahedronEncodePrecise(wm::float3 N, int32& X, int32& Y, int32 QuantizationBits)
	{
		const int32 QuantizationMaxValue = (1 << QuantizationBits) - 1;
		auto Coord = OctahedronEncode(N);

		const float Scale = 0.5f * QuantizationMaxValue;
		const float Bias = 0.5f * QuantizationMaxValue;
		int32 NX = wm::clamp(0, QuantizationMaxValue, int32(Coord.x * Scale + Bias));
		int32 NY = wm::clamp(0, QuantizationMaxValue, int32(Coord.y * Scale + Bias));

		float MinError = 1.0f;
		int32 BestNX = 0;
		int32 BestNY = 0;
		for (int32 OffsetY = 0; OffsetY < 2; OffsetY++)
		{
			for (int32 OffsetX = 0; OffsetX < 2; OffsetX++)
			{
				int32 TX = NX + OffsetX;
				int32 TY = NY + OffsetY;
				if (TX <= QuantizationMaxValue && TY <= QuantizationMaxValue)
				{
					auto RN = OctahedronDecode(TX, TY, QuantizationBits);
					float Error = std::abs(1.0f - wm::dot(RN , N));
					if (Error < MinError)
					{
						MinError = Error;
						BestNX = TX;
						BestNY = TY;
					}
				}
			}
		}

		X = BestNX;
		Y = BestNY;
	}

	static uint32 PackNormal(wm::float3 Normal, uint32 QuantizationBits)
	{
		int32 X2, Y2;
		OctahedronEncodePrecise(Normal, X2, Y2, QuantizationBits);

		return (Y2 << QuantizationBits) | X2;
	}

	static void EncodeGeometryData(const uint32 LocalClusterIndex, const FCluster& Cluster, const FEncodingInfo& EncodingInfo, uint32 NumTexCoords,
		std::vector<uint32>& StripBitmask, std::vector<uint8>& IndexData,
		std::vector<uint32>& VertexRefBitmask, std::vector<uint32>& VertexRefData, std::vector<uint8>& PositionData, std::vector<uint8>& AttributeData,
		std::unordered_map<FVariableVertex, uint32, FVariableVertexHash>& UniqueVertices, uint32& NumCodedVertices)
	{
		const uint32 NumClusterVerts = Cluster.NumVerts;
		const uint32 NumClusterTris = Cluster.NumTris;

		const uint32 ClusterShift = Cluster.QuantizedPosShift;

		VertexRefBitmask.resize(VertexRefBitmask.size()+MAX_CLUSTER_VERTICES / 32);
#if USE_UNCOMPRESSED_VERTEX_DATA
		// Disable vertex references in uncompressed mode
		NumCodedVertices = NumClusterVerts;
#else
		// Find vertices from same page we can reference instead of storing duplicates
		NumCodedVertices = 0;
		std::vector<uint32> UniqueToVertexIndex;
		for (uint32 VertexIndex = 0; VertexIndex < NumClusterVerts; VertexIndex++)
		{
			FVariableVertex Vertex;
			Vertex.Data = &Cluster.Verts[VertexIndex * Cluster.GetVertSize()];
			Vertex.SizeInBytes = Cluster.GetVertSize() * sizeof(float);

			uint32* VertexPtr = white::find(UniqueVertices, Vertex);

			if (VertexPtr)
			{
				int32 ClusterIndexDelta = LocalClusterIndex - (*VertexPtr >> 8);
				wconstraint(ClusterIndexDelta >= 0);
				int32 Code = (ClusterIndexDelta << 8) | (*VertexPtr & 0xFF);
				VertexRefData.emplace_back(Code);

				uint32 BitIndex = LocalClusterIndex * MAX_CLUSTER_VERTICES + VertexIndex;
				VertexRefBitmask[BitIndex >> 5] |= 1u << (BitIndex & 31);
			}
			else
			{
				uint32 Val = (LocalClusterIndex << 8) | NumCodedVertices;
				UniqueVertices.emplace(Vertex, Val);
				UniqueToVertexIndex.emplace_back(VertexIndex);
				NumCodedVertices++;
			}
		}
#endif

		const uint32 BitsPerIndex = EncodingInfo.BitsPerIndex;

		// Write triangle indices
#if USE_STRIP_INDICES
		for (uint32 i = 0; i < MAX_CLUSTER_TRIANGLES / 32; i++)
		{
			StripBitmask.emplace_back(Cluster.StripDesc.Bitmasks[i][0]);
			StripBitmask.emplace_back(Cluster.StripDesc.Bitmasks[i][1]);
			StripBitmask.emplace_back(Cluster.StripDesc.Bitmasks[i][2]);
		}
		IndexData.insert(IndexData.end(), Cluster.StripIndexData.begin(), Cluster.StripIndexData.end());
#else
		for (uint32 i = 0; i < NumClusterTris * 3; i++)
		{
			uint32 Index = Cluster.Indexes[i];
			IndexData.Add(Cluster.Indexes[i]);
		}
#endif

		wconstraint(NumClusterVerts > 0);

		FBitWriter BitWriter_Position(PositionData);
		FBitWriter BitWriter_Attribute(AttributeData);

#if USE_UNCOMPRESSED_VERTEX_DATA
		for (uint32 VertexIndex = 0; VertexIndex < NumClusterVerts; VertexIndex++)
		{
			const FVector& Position = Cluster.GetPosition(VertexIndex);
			BitWriter_Position.PutBits(*(uint32*)&Position.X, 32);
			BitWriter_Position.PutBits(*(uint32*)&Position.Y, 32);
			BitWriter_Position.PutBits(*(uint32*)&Position.Z, 32);
		}
		BitWriter_Position.Flush(sizeof(uint32));

		for (uint32 VertexIndex = 0; VertexIndex < NumClusterVerts; VertexIndex++)
		{
			// Normal
			const FVector& Normal = Cluster.GetNormal(VertexIndex);
			BitWriter_Attribute.PutBits(*(uint32*)&Normal.X, 32);
			BitWriter_Attribute.PutBits(*(uint32*)&Normal.Y, 32);
			BitWriter_Attribute.PutBits(*(uint32*)&Normal.Z, 32);

			// Color
			uint32 ColorDW = Cluster.bHasColors ? Cluster.GetColor(VertexIndex).ToFColor(false).DWColor() : 0xFFFFFFFFu;
			BitWriter_Attribute.PutBits(ColorDW, 32);

			// UVs
			const FVector2D* UVs = Cluster.GetUVs(VertexIndex);
			for (uint32 TexCoordIndex = 0; TexCoordIndex < NumTexCoords; TexCoordIndex++)
			{
				const FVector2D& UV = UVs[TexCoordIndex];
				BitWriter_Attribute.PutBits(*(uint32*)&UV.X, 32);
				BitWriter_Attribute.PutBits(*(uint32*)&UV.Y, 32);
			}
		}
		BitWriter_Attribute.Flush(sizeof(uint32));
#else

		// Generate quantized texture coordinates
		std::vector<uint32> PackedUVs;
		PackedUVs.resize(NumClusterVerts * NumTexCoords);

		uint32 TexCoordBits[MAX_NANITE_UVS] = {};
		for (uint32 UVIndex = 0; UVIndex < NumTexCoords; UVIndex++)
		{
			const int32 TexCoordBitsU = (EncodingInfo.UVPrec >> (UVIndex * 8 + 0)) & 15;
			const int32 TexCoordBitsV = (EncodingInfo.UVPrec >> (UVIndex * 8 + 4)) & 15;
			const int32 TexCoordMaxValueU = (1 << TexCoordBitsU) - 1;
			const int32 TexCoordMaxValueV = (1 << TexCoordBitsV) - 1;

			const FGeometryEncodingUVInfo& UVInfo = EncodingInfo.UVInfos[UVIndex];
			const auto UVMin = UVInfo.UVRange.Min;
			const auto UVDelta = UVInfo.UVDelta;
			const auto UVRcpDelta = UVInfo.UVRcpDelta;
			const int32 NU = UVInfo.NU;
			const int32 NV = UVInfo.NV;
			const int32 GapStartU = UVInfo.UVRange.GapStart[0];
			const int32 GapStartV = UVInfo.UVRange.GapStart[1];
			const int32 GapLengthU = UVInfo.UVRange.GapLength[0];
			const int32 GapLengthV = UVInfo.UVRange.GapLength[1];

			for (uint32 i : UniqueToVertexIndex)
			{
				const wm::float2& UV = Cluster.GetUVs(i)[UVIndex];
				const wm::float2 NormalizedUV = wm::clamp({}, wm::float2(1, 1), ((UV - UVMin) * UVRcpDelta));

				int32 U = int32(NormalizedUV.x * NU + 0.5f);
				int32 V = int32(NormalizedUV.y * NV + 0.5f);
				if (U >= GapStartU)
				{
					wconstraint(U >= GapStartU + GapLengthU);
					U -= GapLengthU;
				}
				if (V >= GapStartV)
				{
					wconstraint(V >= GapStartV + GapLengthV);
					V -= GapLengthV;
				}
				wconstraint(U >= 0 && U <= TexCoordMaxValueU);
				wconstraint(V >= 0 && V <= TexCoordMaxValueV);
				PackedUVs[NumClusterVerts * UVIndex + i] = (V << TexCoordBitsU) | U;
			}
			TexCoordBits[UVIndex] = TexCoordBitsU + TexCoordBitsV;
		}

		// Quantize and write positions
		for (uint32 VertexIndex : UniqueToVertexIndex)
		{
			const auto& QuantizedPosition = Cluster.QuantizedPositions[VertexIndex];
			BitWriter_Position.PutBits(QuantizedPosition.x, Cluster.QuantizedPosBits.x);
			BitWriter_Position.PutBits(QuantizedPosition.y, Cluster.QuantizedPosBits.y);
			BitWriter_Position.PutBits(QuantizedPosition.z, Cluster.QuantizedPosBits.z);
			BitWriter_Position.Flush(1);
		}
		BitWriter_Position.Flush(sizeof(uint32));

		// Quantize and write remaining shading attributes
		for (uint32 VertexIndex : UniqueToVertexIndex)
		{
			// Normal
			uint32 PackedNormal = PackNormal(Cluster.GetNormal(VertexIndex), NORMAL_QUANTIZATION_BITS);
			BitWriter_Attribute.PutBits(PackedNormal, 2 * NORMAL_QUANTIZATION_BITS);

			// Color
			if (EncodingInfo.ColorMode == VERTEX_COLOR_MODE_VARIABLE)
			{
				FColor Color = as_color(Cluster.GetColor(VertexIndex));

				int32 R = Color.R - EncodingInfo.ColorMin.x;
				int32 G = Color.G - EncodingInfo.ColorMin.y;
				int32 B = Color.B - EncodingInfo.ColorMin.z;
				int32 A = Color.A - EncodingInfo.ColorMin.w;
				BitWriter_Attribute.PutBits(R, EncodingInfo.ColorBits.x);
				BitWriter_Attribute.PutBits(G, EncodingInfo.ColorBits.y);
				BitWriter_Attribute.PutBits(B, EncodingInfo.ColorBits.z);
				BitWriter_Attribute.PutBits(A, EncodingInfo.ColorBits.w);
			}

			// UVs
			for (uint32 TexCoordIndex = 0; TexCoordIndex < NumTexCoords; TexCoordIndex++)
			{
				uint32 PackedUV = PackedUVs[NumClusterVerts * TexCoordIndex + VertexIndex];
				BitWriter_Attribute.PutBits(PackedUV, TexCoordBits[TexCoordIndex]);
			}
			BitWriter_Attribute.Flush(1);
		}
		BitWriter_Attribute.Flush(sizeof(uint32));
#endif
	}

	static void WritePages(Resources& Resources,
		std::vector<FPage>& Pages,
		const std::vector<FClusterGroup>& Groups,
		const std::vector<FClusterGroupPart>& Parts,
		const std::vector<FCluster>& Clusters,
		const std::vector<FEncodingInfo>& EncodingInfos,
		uint32 NumTexCoords)
	{
		wconstraint(Resources.PageStreamingStates.size() == 0);

		const bool bLZCompress = true;

		std::vector< uint8 > StreamableBulkData;

		const uint32 NumPages = Pages.size();
		const uint32 NumClusters = Clusters.size();
		Resources.PageStreamingStates.resize(NumPages);

		uint32 TotalGPUSize = 0;
		std::vector<FFixupChunk> FixupChunks;
		FixupChunks.resize(NumPages);
		for (uint32 PageIndex = 0; PageIndex < NumPages; PageIndex++)
		{
			const FPage& Page = Pages[PageIndex];
			FFixupChunk& FixupChunk = FixupChunks[PageIndex];
			FixupChunk.Header.NumClusters = Page.NumClusters;

			uint32 NumHierarchyFixups = 0;
			for (uint32 i = 0; i < Page.PartsNum; i++)
			{
				const FClusterGroupPart& Part = Parts[Page.PartsStartIndex + i];
				NumHierarchyFixups += Groups[Part.GroupIndex].PageIndexNum;
			}

			FixupChunk.Header.NumHierachyFixups = NumHierarchyFixups;	// NumHierarchyFixups must be set before writing cluster fixups
			TotalGPUSize += Page.GpuSizes.GetTotal();
		}

		// Add external fixups to pages
		for (const FClusterGroupPart& Part : Parts)
		{
			wconstraint(Part.PageIndex < NumPages);

			const FClusterGroup& Group = Groups[Part.GroupIndex];
			for (uint32 ClusterPositionInPart = 0; ClusterPositionInPart < (uint32)Part.Clusters.size(); ClusterPositionInPart++)
			{
				const FCluster& Cluster = Clusters[Part.Clusters[ClusterPositionInPart]];
				if (Cluster.GeneratingGroupIndex != INVALID_GROUP_INDEX)
				{
					const FClusterGroup& GeneratingGroup = Groups[Cluster.GeneratingGroupIndex];
					wconstraint(GeneratingGroup.PageIndexNum >= 1);

					if (GeneratingGroup.PageIndexStart == Part.PageIndex && GeneratingGroup.PageIndexNum == 1)
						continue;	// Dependencies already met by current page. Fixup directly instead.

					uint32 PageDependencyStart = GeneratingGroup.PageIndexStart;
					uint32 PageDependencyNum = GeneratingGroup.PageIndexNum;
					RemoveRootPagesFromRange(PageDependencyStart, PageDependencyNum);	// Root page should never be a dependency

					const FClusterFixup ClusterFixup = FClusterFixup(Part.PageIndex, Part.PageClusterOffset + ClusterPositionInPart, PageDependencyStart, PageDependencyNum);
					for (uint32 i = 0; i < GeneratingGroup.PageIndexNum; i++)
					{
						//TODO: Implement some sort of FFixupPart to not redundantly store PageIndexStart/PageIndexNum?
						FFixupChunk& FixupChunk = FixupChunks[GeneratingGroup.PageIndexStart + i];
						FixupChunk.GetClusterFixup(FixupChunk.Header.NumClusterFixups++) = ClusterFixup;
					}
				}
			}
		}

		// Generate page dependencies
		for (uint32 PageIndex = 0; PageIndex < NumPages; PageIndex++)
		{
			const FFixupChunk& FixupChunk = FixupChunks[PageIndex];
			FPageStreamingState& PageStreamingState = Resources.PageStreamingStates[PageIndex];
			PageStreamingState.DependenciesStart = Resources.PageDependencies.size();

			for (uint32 i = 0; i < FixupChunk.Header.NumClusterFixups; i++)
			{
				uint32 FixupPageIndex = FixupChunk.GetClusterFixup(i).GetPageIndex();
				wconstraint(FixupPageIndex < NumPages);
				if (IsRootPage(FixupPageIndex) || FixupPageIndex == PageIndex)	// Never emit dependencies to ourselves or a root page.
					continue;

				// Only add if not already in the set.
				// O(n^2), but number of dependencies should be tiny in practice.
				bool bFound = false;
				for (uint32 j = PageStreamingState.DependenciesStart; j < (uint32)Resources.PageDependencies.size(); j++)
				{
					if (Resources.PageDependencies[j] == FixupPageIndex)
					{
						bFound = true;
						break;
					}
				}

				if (bFound)
					continue;

				Resources.PageDependencies.emplace_back(FixupPageIndex);
			}
			PageStreamingState.DependenciesNum = Resources.PageDependencies.size() - PageStreamingState.DependenciesStart;
		}

		// Process pages
		struct FPageResult
		{
			std::vector<uint8> Data;
			uint32 UncompressedSize;
		};
		std::vector< FPageResult > PageResults;
		PageResults.resize(NumPages);

		ParallelFor(NumPages, [&Resources, &Pages, &Groups, &Parts, &Clusters, &EncodingInfos, &FixupChunks, &PageResults, NumTexCoords, bLZCompress](int32 PageIndex)
			{
				const FPage& Page = Pages[PageIndex];
				FFixupChunk& FixupChunk = FixupChunks[PageIndex];

				// Add hierarchy fixups
				{
					// Parts include the hierarchy fixups for all the other parts of the same group.
					uint32 NumHierarchyFixups = 0;
					for (uint32 i = 0; i < Page.PartsNum; i++)
					{
						const FClusterGroupPart& Part = Parts[Page.PartsStartIndex + i];
						const FClusterGroup& Group = Groups[Part.GroupIndex];
						const uint32 HierarchyRootOffset = Resources.HierarchyRootOffsets[Group.MeshIndex];

						uint32 PageDependencyStart = Group.PageIndexStart;
						uint32 PageDependencyNum = Group.PageIndexNum;
						RemoveRootPagesFromRange(PageDependencyStart, PageDependencyNum);

						// Add fixups to all parts of the group
						for (uint32 j = 0; j < Group.PageIndexNum; j++)
						{
							const FPage& Page2 = Pages[Group.PageIndexStart + j];
							for (uint32 k = 0; k < Page2.PartsNum; k++)
							{
								const FClusterGroupPart& Part2 = Parts[Page2.PartsStartIndex + k];
								if (Part2.GroupIndex == Part.GroupIndex)
								{
									const uint32 GlobalHierarchyNodeIndex = HierarchyRootOffset + Part2.HierarchyNodeIndex;
									FixupChunk.GetHierarchyFixup(NumHierarchyFixups++) = FHierarchyFixup(Part2.PageIndex, GlobalHierarchyNodeIndex, Part2.HierarchyChildIndex, Part2.PageClusterOffset, PageDependencyStart, PageDependencyNum);
									break;
								}
							}
						}
					}
					wconstraint(NumHierarchyFixups == FixupChunk.Header.NumHierachyFixups);
				}

				// Pack clusters and generate material range data
				std::vector<uint32>				CombinedStripBitmaskData;
				std::vector<uint32>				CombinedVertexRefBitmaskData;
				std::vector<uint32>				CombinedVertexRefData;
				std::vector<uint8>				CombinedIndexData;
				std::vector<uint8>				CombinedPositionData;
				std::vector<uint8>				CombinedAttributeData;
				std::vector<uint32>				MaterialRangeData;
				std::vector<uint16>				CodedVerticesPerCluster;
				std::vector<uint32>				NumVertexBytesPerCluster;
				std::vector<FPackedCluster>		PackedClusters;

				PackedClusters.resize(Page.NumClusters);
				CodedVerticesPerCluster.resize(Page.NumClusters);
				NumVertexBytesPerCluster.resize(Page.NumClusters);

				const uint32 NumPackedClusterDwords = Page.NumClusters * sizeof(FPackedCluster) / sizeof(uint32);

				FPageSections GpuSectionOffsets = Page.GpuSizes.GetOffsets();
				std::unordered_map<FVariableVertex, uint32, FVariableVertexHash> UniqueVertices;

				for (uint32 i = 0; i < Page.PartsNum; i++)
				{
					const FClusterGroupPart& Part = Parts[Page.PartsStartIndex + i];
					for (uint32 j = 0; j < (uint32)Part.Clusters.size(); j++)
					{
						const uint32 ClusterIndex = Part.Clusters[j];
						const FCluster& Cluster = Clusters[ClusterIndex];
						const FEncodingInfo& EncodingInfo = EncodingInfos[ClusterIndex];

						const uint32 LocalClusterIndex = Part.PageClusterOffset + j;
						FPackedCluster& PackedCluster = PackedClusters[LocalClusterIndex];
						PackCluster(PackedCluster, Cluster, EncodingInfos[ClusterIndex], NumTexCoords);

						PackedCluster.PackedMaterialInfo = PackMaterialInfo(Cluster, MaterialRangeData, NumPackedClusterDwords);
						wconstraint((GpuSectionOffsets.Index & 3) == 0);
						wconstraint((GpuSectionOffsets.Position & 3) == 0);
						wconstraint((GpuSectionOffsets.Attribute & 3) == 0);
						PackedCluster.SetIndexOffset(GpuSectionOffsets.Index);
						PackedCluster.SetPositionOffset(GpuSectionOffsets.Position);
						PackedCluster.SetAttributeOffset(GpuSectionOffsets.Attribute);
						PackedCluster.SetDecodeInfoOffset(GpuSectionOffsets.DecodeInfo);

						GpuSectionOffsets += EncodingInfo.GpuSizes;

						const uint32 PrevVertexBytes = CombinedPositionData.size();
						uint32 NumCodedVertices = 0;
						EncodeGeometryData(LocalClusterIndex, Cluster, EncodingInfo, NumTexCoords,
							CombinedStripBitmaskData, CombinedIndexData,
							CombinedVertexRefBitmaskData, CombinedVertexRefData, CombinedPositionData, CombinedAttributeData,
							UniqueVertices, NumCodedVertices);

						NumVertexBytesPerCluster[LocalClusterIndex] = CombinedPositionData.size() - PrevVertexBytes;
						CodedVerticesPerCluster[LocalClusterIndex] = NumCodedVertices;
					}
				}
				wconstraint(GpuSectionOffsets.Cluster == Page.GpuSizes.GetMaterialTableOffset());
				wconstraint(Align(GpuSectionOffsets.MaterialTable, 16) == Page.GpuSizes.GetDecodeInfoOffset());
				wconstraint(GpuSectionOffsets.DecodeInfo == Page.GpuSizes.GetIndexOffset());
				wconstraint(GpuSectionOffsets.Index == Page.GpuSizes.GetPositionOffset());
				wconstraint(GpuSectionOffsets.Position == Page.GpuSizes.GetAttributeOffset());
				wconstraint(GpuSectionOffsets.Attribute == Page.GpuSizes.GetTotal());

				// Dword align index data
				CombinedIndexData.resize((CombinedIndexData.size() + 3) & -4);

				// Perform page-internal fix up directly on PackedClusters
				for (uint32 LocalPartIndex = 0; LocalPartIndex < Page.PartsNum; LocalPartIndex++)
				{
					const FClusterGroupPart& Part = Parts[Page.PartsStartIndex + LocalPartIndex];
					const FClusterGroup& Group = Groups[Part.GroupIndex];
					uint32 GeneratingGroupIndex = wm::MAX_uint32;
					for (uint32 ClusterPositionInPart = 0; ClusterPositionInPart < (uint32)Part.Clusters.size(); ClusterPositionInPart++)
					{
						const FCluster& Cluster = Clusters[Part.Clusters[ClusterPositionInPart]];
						if (Cluster.GeneratingGroupIndex != INVALID_GROUP_INDEX)
						{
							const FClusterGroup& GeneratingGroup = Groups[Cluster.GeneratingGroupIndex];
							uint32 PageDependencyStart = Group.PageIndexStart;
							uint32 PageDependencyNum = Group.PageIndexNum;
							RemoveRootPagesFromRange(PageDependencyStart, PageDependencyNum);

							if (GeneratingGroup.PageIndexStart == PageIndex && GeneratingGroup.PageIndexNum == 1)
							{
								// Dependencies already met by current page. Fixup directly.
								PackedClusters[Part.PageClusterOffset + ClusterPositionInPart].Flags &= ~NANITE_CLUSTER_FLAG_LEAF;	// Mark parent as no longer leaf
							}
						}
					}
				}

				// Begin page
				FPageResult& PageResult = PageResults[PageIndex];
				PageResult.Data.resize(CLUSTER_PAGE_DISK_SIZE);
				FBlockPointer PagePointer(PageResult.Data.data(), PageResult.Data.size());

				// Disk header
				FPageDiskHeader* PageDiskHeader = PagePointer.Advance<FPageDiskHeader>(1);

				// 16-byte align material range data to make it easy to copy during GPU transcoding
				MaterialRangeData.resize(Align(MaterialRangeData.size(), 4));

				static_assert(sizeof(FUVRange) % 16 == 0, "sizeof(FUVRange) must be a multiple of 16");
				static_assert(sizeof(FPackedCluster) % 16 == 0, "sizeof(FPackedCluster) must be a multiple of 16");
				PageDiskHeader->NumClusters = Page.NumClusters;
				PageDiskHeader->GpuSize = Page.GpuSizes.GetTotal();
				PageDiskHeader->NumRawFloat4s = Page.NumClusters * (sizeof(FPackedCluster) + NumTexCoords * sizeof(FUVRange)) / 16 + MaterialRangeData.size() / 4;
				PageDiskHeader->NumTexCoords = NumTexCoords;

				// Cluster headers
				FClusterDiskHeader* ClusterDiskHeaders = PagePointer.Advance<FClusterDiskHeader>(Page.NumClusters);

				// Write clusters in SOA layout
				{
					const uint32 NumClusterFloat4Propeties = sizeof(FPackedCluster) / 16;
					for (uint32 float4Index = 0; float4Index < NumClusterFloat4Propeties; float4Index++)
					{
						for (const FPackedCluster& PackedCluster : PackedClusters)
						{
							uint8* Dst = PagePointer.Advance<uint8>(16);
							std::memcpy(Dst, (uint8*)&PackedCluster + float4Index * 16, 16);
						}
					}
				}

				// Material table
				uint32 MaterialTableSize = MaterialRangeData.size() * sizeof(MaterialRangeData[0]);
				uint8* MaterialTable = PagePointer.Advance<uint8>(MaterialTableSize);
				std::memcpy(MaterialTable, MaterialRangeData.data(), MaterialTableSize);
				wconstraint(MaterialTableSize == Page.GpuSizes.GetMaterialTableSize());

				// Decode information
				PageDiskHeader->DecodeInfoOffset = PagePointer.Offset();
				for (uint32 i = 0; i < Page.PartsNum; i++)
				{
					const FClusterGroupPart& Part = Parts[Page.PartsStartIndex + i];
					for (uint32 j = 0; j < (uint32)Part.Clusters.size(); j++)
					{
						const uint32 ClusterIndex = Part.Clusters[j];
						FUVRange* DecodeInfo = PagePointer.Advance<FUVRange>(NumTexCoords);
						for (uint32 k = 0; k < NumTexCoords; k++)
						{
							DecodeInfo[k] = EncodingInfos[ClusterIndex].UVInfos[k].UVRange;
						}
					}
				}

				// Index data
				{
					uint8* IndexData = PagePointer.GetPtr<uint8>();
#if USE_STRIP_INDICES
					for (uint32 i = 0; i < Page.PartsNum; i++)
					{
						const FClusterGroupPart& Part = Parts[Page.PartsStartIndex + i];
						for (uint32 j = 0; j < (uint32)Part.Clusters.size(); j++)
						{
							const uint32 LocalClusterIndex = Part.PageClusterOffset + j;
							const uint32 ClusterIndex = Part.Clusters[j];
							const FCluster& Cluster = Clusters[ClusterIndex];

							ClusterDiskHeaders[LocalClusterIndex].IndexDataOffset = PagePointer.Offset();
							ClusterDiskHeaders[LocalClusterIndex].NumPrevNewVerticesBeforeDwords = Cluster.StripDesc.NumPrevNewVerticesBeforeDwords;
							ClusterDiskHeaders[LocalClusterIndex].NumPrevRefVerticesBeforeDwords = Cluster.StripDesc.NumPrevRefVerticesBeforeDwords;

							PagePointer.Advance<uint8>(Cluster.StripIndexData.size());
						}
					}

					uint32 IndexDataSize = CombinedIndexData.size() * sizeof(CombinedIndexData[0]);
					std::memcpy(IndexData, CombinedIndexData.data(), IndexDataSize);
					PagePointer.Align(sizeof(uint32));

					PageDiskHeader->StripBitmaskOffset = PagePointer.Offset();
					uint32 StripBitmaskDataSize = CombinedStripBitmaskData.size() * sizeof(CombinedStripBitmaskData[0]);
					uint8* StripBitmaskData = PagePointer.Advance<uint8>(StripBitmaskDataSize);
					std::memcpy(StripBitmaskData, CombinedStripBitmaskData.data(), StripBitmaskDataSize);

#else
					for (uint32 i = 0; i < Page.NumClusters; i++)
					{
						ClusterDiskHeaders[i].IndexDataOffset = PagePointer.Offset();
						PagePointer.Advance<uint8>(PackedClusters[i].GetNumTris() * 3);
					}
					PagePointer.Align(sizeof(uint32));

					uint32 IndexDataSize = CombinedIndexData.Num() * CombinedIndexData.GetTypeSize();
					std::memcpy(IndexData, CombinedIndexData.GetData(), IndexDataSize);
#endif
				}

				// Write Vertex Reference Bitmask
				{
					PageDiskHeader->VertexRefBitmaskOffset = PagePointer.Offset();
					const uint32 VertexRefBitmaskSize = Page.NumClusters * (MAX_CLUSTER_VERTICES / 8);
					uint8* VertexRefBitmask = PagePointer.Advance<uint8>(VertexRefBitmaskSize);
					std::memcpy(VertexRefBitmask, CombinedVertexRefBitmaskData.data(), VertexRefBitmaskSize);
					wconstraint(CombinedVertexRefBitmaskData.size() * sizeof(CombinedVertexRefBitmaskData[0]) == VertexRefBitmaskSize);
				}

				// Write Vertex References
				{
					uint8* VertexRefs = PagePointer.GetPtr<uint8>();
					for (uint32 i = 0; i < Page.NumClusters; i++)
					{
						ClusterDiskHeaders[i].VertexRefDataOffset = PagePointer.Offset();
						uint32 NumVertexRefs = PackedClusters[i].GetNumVerts() - CodedVerticesPerCluster[i];
						PagePointer.Advance<uint32>(NumVertexRefs);
					}
					std::memcpy(VertexRefs, CombinedVertexRefData.data(), CombinedVertexRefData.size() * sizeof(CombinedVertexRefData[0]));
				}

				// Write Positions
				{
					uint8* PositionData = PagePointer.GetPtr<uint8>();
					for (uint32 i = 0; i < Page.NumClusters; i++)
					{
						ClusterDiskHeaders[i].PositionDataOffset = PagePointer.Offset();
						PagePointer.Advance<uint8>(NumVertexBytesPerCluster[i]);
					}
					wconstraint((PagePointer.GetPtr<uint8>() - PositionData) == CombinedPositionData.size() * sizeof(CombinedPositionData[0]));

					std::memcpy(PositionData, CombinedPositionData.data(), CombinedPositionData.size() * sizeof(CombinedPositionData[0]));
				}

				// Write Attributes
				{
					uint8* AttribData = PagePointer.GetPtr<uint8>();
					for (uint32 i = 0; i < Page.NumClusters; i++)
					{
						const uint32 BytesPerAttribute = (PackedClusters[i].GetBitsPerAttribute() + 7) / 8;
						ClusterDiskHeaders[i].AttributeDataOffset = PagePointer.Offset();
						PagePointer.Advance<uint8>(Align(CodedVerticesPerCluster[i] * BytesPerAttribute, 4));
					}
					wconstraint((uint32)(PagePointer.GetPtr<uint8>() - AttribData) == CombinedAttributeData.size() * sizeof(CombinedAttributeData[0]));
					std::memcpy(AttribData, CombinedAttributeData.data(), CombinedAttributeData.size() * sizeof(CombinedAttributeData[0]));
				}

				if (bLZCompress)
				{
					std::vector<uint8> DataCopy(PageResult.Data.data(), PageResult.Data.data() + PagePointer.Offset());
					PageResult.UncompressedSize = DataCopy.size();

					int32 CompressedSize = PageResult.Data.size();
					wconstraint(Compression::CompressMemory(NAME_LZ4, PageResult.Data.data(), CompressedSize, DataCopy.data(), DataCopy.size()));

					PageResult.Data.resize(CompressedSize, false);
				}
				else
				{
					PageResult.Data.resize(PagePointer.Offset(), false);
					PageResult.UncompressedSize = PageResult.Data.size();
				}
			});

		// Write pages
		uint32 TotalUncompressedSize = 0;
		uint32 TotalCompressedSize = 0;
		uint32 TotalFixupSize = 0;
		for (uint32 PageIndex = 0; PageIndex < NumPages; PageIndex++)
		{
			const FPage& Page = Pages[PageIndex];

			FFixupChunk& FixupChunk = FixupChunks[PageIndex];
			std::vector<uint8>& BulkData = IsRootPage(PageIndex) ? Resources.RootClusterPage : StreamableBulkData;

			FPageStreamingState& PageStreamingState = Resources.PageStreamingStates[PageIndex];
			PageStreamingState.BulkOffset = BulkData.size();

			// Write fixup chunk
			uint32 FixupChunkSize = FixupChunk.GetSize();
			wconstraint(FixupChunk.Header.NumHierachyFixups < MAX_CLUSTERS_PER_PAGE);
			wconstraint(FixupChunk.Header.NumClusterFixups < MAX_CLUSTERS_PER_PAGE);
			BulkData.insert(BulkData.end(), (uint8*)&FixupChunk, (uint8*)&FixupChunk + FixupChunkSize);
			TotalFixupSize += FixupChunkSize;

			// Copy page to BulkData
			std::vector<uint8>& PageData = PageResults[PageIndex].Data;
			BulkData.insert(BulkData.end(), PageData.begin(), PageData.end());
			TotalUncompressedSize += PageResults[PageIndex].UncompressedSize;
			TotalCompressedSize += PageData.size();

			PageStreamingState.BulkSize = BulkData.size() - PageStreamingState.BulkOffset;
			PageStreamingState.PageUncompressedSize = PageResults[PageIndex].UncompressedSize;
		}

		uint32 TotalDiskSize = Resources.RootClusterPage.size() + StreamableBulkData.size();
		spdlog::info("WritePages:");
		spdlog::info("  {} pages written.", NumPages);
		spdlog::info("  GPU size: {} bytes. {:.3f} bytes per page. {:.3f}%% utilization.", TotalGPUSize, TotalGPUSize / float(NumPages), TotalGPUSize / (float(NumPages) * CLUSTER_PAGE_GPU_SIZE) * 100.0f);
		spdlog::info("  Uncompressed page data: {} bytes. Compressed page data: {} bytes. Fixup data: {} bytes.", TotalUncompressedSize, TotalCompressedSize, TotalFixupSize);
		spdlog::info("  Total disk size: {} bytes. {:.3f} bytes per page.", TotalDiskSize, TotalDiskSize / float(NumPages));

		// Store PageData
		Resources.StreamableClusterPages.Lock(LOCK_READ_WRITE);
		uint8* Ptr = (uint8*)Resources.StreamableClusterPages.Realloc(StreamableBulkData.size());
		std::memcpy(Ptr, StreamableBulkData.data(), StreamableBulkData.size());
		Resources.StreamableClusterPages.Unlock();
		Resources.StreamableClusterPages.SetBulkDataFlags(BULKDATA_Force_NOT_InlinePayload);
		Resources.bLZCompressed = bLZCompress;
	}



	void Encode(Resources& Resources, const Settings& Settings, std::vector< FCluster >& Clusters,
		std::vector< FClusterGroup >& Groups, const FBounds& MeshBounds,
		uint32 NumMeshes,
		uint32 NumTexCoords,
		bool bHasColors)
	{
		{
			NANITE_TRACE(Build::BuildMaterialRanges);
			BuildMaterialRanges(Clusters);
		}
		{
			NANITE_TRACE(Build::ConstrainClusters);
			ConstrainClusters(Groups, Clusters);
		}
		{
			NANITE_TRACE(Build::CalculateQuantizedPositions);
			Resources.PositionPrecision = CalculateQuantizedPositionsUniformGrid(Clusters, MeshBounds, Settings);	// Needs to happen after clusters have been constrained and split.
		}
		std::vector<FPage> Pages;
		std::vector<FClusterGroupPart> GroupParts;
		std::vector<FEncodingInfo> EncodingInfos;

		{
			NANITE_TRACE(Build::CalculateEncodingInfos);
			CalculateEncodingInfos(EncodingInfos, Clusters, bHasColors, NumTexCoords);
		}

		{
			NANITE_TRACE(Build::AssignClustersToPages);
			AssignClustersToPages(Groups, Clusters, EncodingInfos, Pages, GroupParts);
		}

		{
			NANITE_TRACE(Build::BuildHierarchyNodes);
			BuildHierarchies(Resources, Groups, GroupParts, NumMeshes);
		}

		{
			NANITE_TRACE(Build::WritePages);
			WritePages(Resources, Pages, Groups, GroupParts, Clusters, EncodingInfos, NumTexCoords);
		}
	}

}