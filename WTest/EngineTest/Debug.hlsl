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
Texture2D NormalTexture;
Texture2D AlbedoTexture;
SamplerState GBufferSampler;

float4 LightDirectionAndIntensity;
float4 LightColorAndAmbient;

void VisibilityLightingPS(
	in noperspective float2 UV : TEXCOORD0,
	out float4 OutColor : SV_Target0)
{
    float deviceDepth = DepthTexture.SampleLevel(GBufferSampler, UV, 0).r;
    if (deviceDepth >= 0.999999f)
    {
        float horizon = saturate(1.0f - UV.y);
        float3 sky = lerp(float3(0.055f, 0.07f, 0.10f), float3(0.28f, 0.42f, 0.62f), horizon);
        OutColor = float4(sky, 1.0f);
        return;
    }

    float3 normal = normalize(NormalTexture.SampleLevel(GBufferSampler, UV, 0).xyz * 2.0f - 1.0f);
    float3 albedo = AlbedoTexture.SampleLevel(GBufferSampler, UV, 0).rgb;

    float3 lightDirection = normalize(LightDirectionAndIntensity.xyz);
    float direct = saturate(dot(normal, lightDirection)) * LightDirectionAndIntensity.w;
    float hemisphere = 0.5f + 0.5f * normal.y;
    float3 ambientColor = lerp(float3(0.08f, 0.065f, 0.055f), float3(0.24f, 0.32f, 0.45f), hemisphere);
    float3 hdrColor = albedo * (ambientColor * LightColorAndAmbient.w + LightColorAndAmbient.rgb * direct);

    // Filmic-style compression and display transfer keep the lit scene useful
    // on the UNORM swapchain without requiring a separate HDR target yet.
    float3 mapped = hdrColor / (1.0f + hdrColor);
    mapped = pow(saturate(mapped), 1.0f / 2.2f);
    OutColor = float4(mapped, 1.0f);
}
