
#include "ShaderStructs.hlsli"

cbuffer ExternalData : register(b0)
{
    // material related
    float4 colorTint;
    float2 uvScale;
    float2 uvOffset;
}

// texture related resources
Texture2D SurfaceTexture : register(t0); // "t" registers for textures
SamplerState BasicSampler : register(s0); // "s" registers for samplers

float4 main(VertexToPixel input) : SV_TARGET
{
	// adjust uv coords
    input.uv = input.uv * uvScale + uvOffset;
    
    // sample texture and apply tint
    float4 surfaceColor = SurfaceTexture.Sample(BasicSampler, input.uv);
    surfaceColor *= colorTint;
    
    return surfaceColor;
}