
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
    DrawIndexArguments Args = (DrawIndexArguments)0;
    Args.IndexCountPerInstance = DrawArg.numIndices;
    Args.InstanceCount = DrawArg.numIndices > 0 ? 1 : 0;
    Args.StartIndexLocation = DrawArg.startIndex;
    Args.BaseVertexLocation = 0;
    Args.StartInstanceLocation = 0;
    IndrectDrawArgsBuffer[DrawId.x] = Args;

}
