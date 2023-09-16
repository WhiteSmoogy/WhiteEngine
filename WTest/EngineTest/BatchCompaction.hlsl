
#define COMPACTION_THREADS 256

RWBuffer<uint> IndrectDrawArgsBuffer;

struct UncompactedDrawArguments
{
    uint startIndex;
    uint numIndices;
};

StructuredBuffer<UncompactedDrawArguments> UncompactedDrawArgs;
uint MaxDraws;

[numthreads(COMPACTION_THREADS, 1, 1)]
void BatchCompactionCS(uint3 DrawId: SV_DispatchThreadID)
{
    if (DrawId.x >= MaxDraws)
        return;

    UncompactedDrawArguments DrawArg = UncompactedDrawArgs[DrawId.x];
    uint numIndices = DrawArg.numIndices;

    if (numIndices == 0)
        return;

    uint slot = 0;

    InterlockedAdd(IndrectDrawArgsBuffer[0], 1, slot);

    IndrectDrawArgsBuffer[(slot + 1) * INDIRECT_DRAW_ARGUMENTS_STRUCT_NUM_ELEMENTS + 0] = numIndices;
    IndrectDrawArgsBuffer[(slot + 1) * INDIRECT_DRAW_ARGUMENTS_STRUCT_NUM_ELEMENTS + 1] = 1;
    IndrectDrawArgsBuffer[(slot + 1) * INDIRECT_DRAW_ARGUMENTS_STRUCT_NUM_ELEMENTS + 2] = DrawArg.startIndex;
    IndrectDrawArgsBuffer[(slot + 1) * INDIRECT_DRAW_ARGUMENTS_STRUCT_NUM_ELEMENTS + 3] = 0;
    IndrectDrawArgsBuffer[(slot + 1) * INDIRECT_DRAW_ARGUMENTS_STRUCT_NUM_ELEMENTS + 4] = 0;

}
