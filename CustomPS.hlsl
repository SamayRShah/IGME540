
#include "ShaderStructs.hlsli"

cbuffer ExternalData : register(b0)
{
    float4 colorTint;
    float dt;
    float tt;
}

float4 main(VertexToPixel input) : SV_TARGET
{
    // center uv
    float2 uv = input.uv - 0.5;
    
    // split uv
    uv = frac(uv * sin(tt) * 2.0f + 0.5f) - 0.5f;
   
    // rings
    float d = length(uv);
    float3 color = float3(1, 1, 1);
    
    d = sin(d * 8 + tt) / 8;
    d = abs(d);
    d = 0.02/d;
        
    color *= d * (float3)colorTint + sin(tt) / 10.0f;
    
	return float4(color, 1.0f);
}