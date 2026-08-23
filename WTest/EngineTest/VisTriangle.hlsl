struct Matrixs
{
    float4x4 mvp;
};

struct ViewArgs
{
    Matrixs matrixs;
};

ConstantBuffer<ViewArgs> View;
uint DrawId;

struct VisVertexOutput
{
    float4 Position : SV_Position;
    float3 WorldNormal : TEXCOORD0;
    float2 TexCoord : TEXCOORD1;
    nointerpolation uint DrawId : TEXCOORD2;
};

float3 TransformQuat(float3 v, float4 q)
{
    return v + 2.0f * cross(q.xyz, cross(q.xyz, v) + q.w * v);
}

VisVertexOutput VisTriangleVS(
    float3 position : POSITION,
    float4 tangentQuat : TANGENT,
    float2 texCoord : TEXCOORD)
{
    VisVertexOutput output;
    output.Position = mul(float4(position, 1), View.matrixs.mvp);
    tangentQuat = tangentQuat * 2.0f - 1.0f;
    output.WorldNormal = normalize(TransformQuat(float3(0, 0, 1), tangentQuat));
    output.TexCoord = texCoord;
    output.DrawId = DrawId;
    return output;
}

float Hash(uint value)
{
    value ^= value >> 16;
    value *= 0x7feb352du;
    value ^= value >> 15;
    value *= 0x846ca68bu;
    value ^= value >> 16;
    return (value & 0xffffu) / 65535.0f;
}

struct VisibilityOutput
{
    uint Visibility : SV_Target0;
    float4 Normal : SV_Target1;
    float4 Albedo : SV_Target2;
};

VisibilityOutput VisTrianglePS(VisVertexOutput input, uint primitiveId : SV_PrimitiveID)
{
    VisibilityOutput output;
    output.Visibility = (input.DrawId << 20) | (primitiveId & 0xfffffu);
    output.Normal = float4(input.WorldNormal * 0.5f + 0.5f, 1.0f);

    // Trinf currently carries geometry streams but no material table. Keep a
    // stable per-draw base colour so the lighting pass can still distinguish
    // Sponza's submeshes until bindless material resolve is added.
    float tint = Hash(input.DrawId);
    float3 coolStone = float3(0.38f, 0.43f, 0.50f);
    float3 warmStone = float3(0.72f, 0.55f, 0.36f);
    float3 wallColor = lerp(coolStone, warmStone, tint);
    float upFacing = smoothstep(0.35f, 0.85f, abs(input.WorldNormal.y));
    float3 horizontalColor = input.WorldNormal.y >= 0.0f
        ? float3(0.58f, 0.43f, 0.27f)
        : float3(0.30f, 0.33f, 0.38f);
    float3 baseColor = lerp(wallColor, horizontalColor, upFacing);
    output.Albedo = float4(baseColor, 1.0f);
    return output;
}
