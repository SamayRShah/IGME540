#include "ShaderStructs.hlsli"

cbuffer ExternalData : register(b0)
{
    // material related
    float4 colorTint;
    float2 uvScale;
    float2 uvOffset;
}

// texture related resources
Texture2D SurfaceTexture       : register(t0); // "t" registers for textures
Texture2D DecalTexture         : register(t1);
SamplerState BasicSampler      : register(s0); // "s" registers for samplers

float4 main(VertexToPixel input) : SV_TARGET
{
	// adjust uv coords
    input.uv = input.uv * uvScale + uvOffset;
    
    // Sample both textures
    float4 color1 = SurfaceTexture.Sample(BasicSampler, input.uv);
    float4 color2 = DecalTexture.Sample(BasicSampler, input.uv);
    
    // Blend both textures
    float4 finalColor = color1 * color2;

    // Apply tint
    return finalColor * colorTint;
}