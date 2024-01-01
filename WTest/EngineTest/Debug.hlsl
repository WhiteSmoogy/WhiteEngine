#include "Common.h"

void FullScreenVS(
	in float4 InPosition : POSITION,
	in float2 InTexCoord : TEXCOORD0,
	out noperspective float2 OutTexCoord : TEXCOORD0,
	out float4 OutPosition : SV_POSITION
	)
{
    DrawRectangle(InPosition, InTexCoord, OutPosition, OutTexCoord);
}

Texture2D DepthTexture;
SamplerState DepthSampler;
float2 NearFar;

void LinearDepthPS(
	in noperspective float2 UV : TEXCOORD0,
	out float4 OutColor : SV_Target0)
{
    float r = DepthTexture.SampleLevel(DepthSampler, UV, 0);
	
    float depth = ConvertFromDeviceZ(r);
	
    OutColor = (depth - NearFar.r) * NearFar.g;
}