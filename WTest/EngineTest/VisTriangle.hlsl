struct Matrixs
{
    float4x4 mvp;
};

struct ViewArgs
{
    Matrixs matrixs;
};

ConstantBuffer<ViewArgs> View;

float4 VisTriangleVS(float3 postion : POSITION) : SV_Position
{
    float4 Position = mul(float4(postion, 1), View.matrixs.mvp);

    return Position;
}

uint VisTrianglePS(float4 position :SV_Position, uint primitiveId : SV_PrimitiveID):SV_Target
{
    return primitiveId;
}