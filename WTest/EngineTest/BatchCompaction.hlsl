
#define COMPACTION_THREADS 256

RWBuffer<uint> IndrectDrawArgsBuffer;
RWBuffer<uint> IndrectMaterialsBuffer;

struct UncompactedDrawArguments
{
    uint startIndex;
    uint numIndices;
};

RWStructuredBuffer<UncompactedDrawArguments> UncompactedDrawArgs;
uint MaxDraws;

[numthreads(TRIANGLE_PER_CLUSTER, 1, 1)]
void BatchCompactionCS(uint3 DrawId: SV_DispatchThreadID)
{
    if(DrwaId.x >= MaxDraws)
        return;

    uint numIndices =  UncompactedDrawArgs[DrwaId.x].numIndices;

    if (numIndices == 0)
        return;

    uint slot = 0;

    InterlockedAdd(IndrectDrawArgsBuffer[0], 1, slot);

    IndrectDrawArgsBuffer[slot * INDIRECT_DRAW_ARGUMENTS_STRUCT_NUM_ELEMENTS + 0] =  numIndices;
    IndrectDrawArgsBuffer[slot * INDIRECT_DRAW_ARGUMENTS_STRUCT_NUM_ELEMENTS + 1] =  UncompactedDrawArgs[DrwaId.x].startIndex;
    IndrectMaterialsBuffer[slot] = 0;
}