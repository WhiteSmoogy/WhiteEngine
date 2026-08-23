
#define COMPACTION_THREADS 256

struct DrawIndexArguments
{
    uint IndexCountPerInstance;
    uint InstanceCount;
    uint StartIndexLocation;
    int BaseVertexLocation;
    uint StartInstanceLocation;
};

RWStructuredBuffer<DrawIndexArguments> IndrectDrawArgsBuffer;

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

    InterlockedAdd(IndrectDrawArgsBuffer[0].IndexCountPerInstance, 1, slot);

    DrawIndexArguments Args;
    Args.IndexCountPerInstance = numIndices;
    Args.InstanceCount = 1;
    Args.StartIndexLocation = DrawArg.startIndex;
    Args.BaseVertexLocation = 0;
    Args.StartInstanceLocation = 0;
    IndrectDrawArgsBuffer[slot + 1] = Args;

}
