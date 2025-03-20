#include "ShaderStructs.hlsli"

float4 main(VertexToPixel input) : SV_TARGET
{
	return float4(input.uv, 0.0f, 1.0f);
}