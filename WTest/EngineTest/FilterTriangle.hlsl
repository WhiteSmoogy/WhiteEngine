#define TRIANGLE_PER_CLUSTER 64

struct FilterDispatchArgs
{
    uint IndexStart;
    uint VertexStart;

    uint DrawId;

    uint IndexEnd;

    uint OutpuIndexOffset;

	uint ClusterId;

    uint pading0;
    uint pading1;
};

struct BatchArgs
{
    FilterDispatchArgs Args[2048];
};

ConstantBuffer<BatchArgs> DispatchArgs;

struct Matrixs
{
    float4x4 mvp;
};

struct ViewArgs
{
     Matrixs matrixs;
};

ConstantBuffer<ViewArgs> View; 

ByteAddressBuffer IndexBuffer;
ByteAddressBuffer PositionBuffer;

RWByteAddressBuffer FliteredIndexBuffer;

struct UncompactedDrawArguments
{
    uint startIndex;
    uint numIndices;
};

RWStructuredBuffer<UncompactedDrawArguments> UncompactedDrawArgs;

uint3 Load3(ByteAddressBuffer InputBuffer,uint Index)
{
    return InputBuffer.Load3(Index*4);
}

float4 LoadVertex(uint Index)
{
    return float4(asfloat(PositionBuffer.Load4(Index*12)).xyz, 1);  
}

bool CullTriangle(float4 vertices[3])
{
#if CULL_BACKFACE
    float3x3 m = float3x3(vertices[0].xyw, vertices[1].xyw, vertices[2].xyw);
    // If the determinant is > 0, the triangle is backfacing.
	// If the determinant is = 0, the triangle is being viewed edge-on or is degenerate, and has 0 screen-space area.
	if (determinant(m) >= 0)
		return true;    
#endif

#if CULL_FRUSTUM
    int verticesInFrontOfNearPlane = 0;

    for (uint i = 0; i < 3; i++)
	{
		if (vertices[i].w < 0)
		{
			++verticesInFrontOfNearPlane;

			// Flip the w so that any triangle that straddles the plane won't be projected onto
			// two sides of the screen
			vertices[i].w *= (-1.0);
		}
		// Transform vertices[i].xy into the normalized 0..1 screen space
		// this is for the following stages ...
		vertices[i].xy /= vertices[i].w * 2;
		vertices[i].xy += float2(0.5, 0.5);
	}

    if (verticesInFrontOfNearPlane == 3)
		return true;

    float minx = min(min(vertices[0].x, vertices[1].x), vertices[2].x);
	float miny = min(min(vertices[0].y, vertices[1].y), vertices[2].y);
	float maxx = max(max(vertices[0].x, vertices[1].x), vertices[2].x);
	float maxy = max(max(vertices[0].y, vertices[1].y), vertices[2].y);

	if ((maxx < 0) || (maxy < 0) || (minx > 1) || (miny > 1))
		return true;
#endif

    return false;
}

groupshared uint  workGroupIndexCount;
groupshared uint  workGroupOutputSlot;

void StoreByte(RWByteAddressBuffer buffer,uint index,uint value)
{
    buffer.Store(index * 4, value);
}

[numthreads(TRIANGLE_PER_CLUSTER, 1, 1)]
void FilterTriangleCS( uint BatchIndex : SV_GroupID, uint3 TriId: SV_GroupThreadID)
{
	FilterDispatchArgs Args = DispatchArgs.Args[BatchIndex];

	uint ClusterOffsetIndex = Args.IndexStart + Args.ClusterId * TRIANGLE_PER_CLUSTER * 3;
    
	uint IndexOffset = ClusterOffsetIndex + TriId.x * 3;

	if (TriId.x == 0)
	{
		workGroupIndexCount = 0;
	}

	GroupMemoryBarrier();

	uint threadOutputSlot = 0;

	bool cull = true;
	uint3 Indices = 0;

    //last cluster's triangle count <= TRIANGLE_PER_CLUSTER
	if (IndexOffset < Args.IndexEnd)
	{
		Indices = Load3(IndexBuffer, IndexOffset);

		float4 v0 = LoadVertex(Args.VertexStart + Indices.x);
		float4 v1 = LoadVertex(Args.VertexStart + Indices.y);
		float4 v2 = LoadVertex(Args.VertexStart + Indices.z);

		float4x4 mvp = View.matrixs.mvp;

		float4 vertices[3] =
		{
			mul(v0, mvp),
			mul(v1, mvp),
			mul(v2, mvp)
		};

		cull = CullTriangle(vertices);
        cull = false;

		if (!cull)
		{
			InterlockedAdd(workGroupIndexCount, 3, threadOutputSlot);
		}
	}

	GroupMemoryBarrier();

	if (TriId.x == 0)
	{
		InterlockedAdd(UncompactedDrawArgs[Args.DrawId].numIndices, workGroupIndexCount, workGroupOutputSlot);
	}

	AllMemoryBarrier();

	if (!cull)
	{
		uint index = workGroupOutputSlot;
        
		StoreByte(FliteredIndexBuffer, index + Args.OutpuIndexOffset + threadOutputSlot + 0, Indices.x);
		StoreByte(FliteredIndexBuffer, index + Args.OutpuIndexOffset + threadOutputSlot + 1, Indices.y);
		StoreByte(FliteredIndexBuffer, index + Args.OutpuIndexOffset + threadOutputSlot + 2, Indices.z);
	}

	if (TriId.x == 0 && Args.ClusterId == 0)
	{
		UncompactedDrawArgs[Args.DrawId].startIndex = Args.OutpuIndexOffset;
	}
}
