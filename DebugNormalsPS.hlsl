#include "ShaderStructs.hlsli"

float4 main(VertexToPixel input) : SV_TARGET
{
    return float4(input.normal, 1.0f);
}