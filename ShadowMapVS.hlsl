#include "ShaderStructs.hlsli"

// Constant Buffer for external (C++) data
cbuffer externalData : register(b0)
{
    matrix view;
    matrix projection;
    matrix world;
};
// --------------------------------------------------------
// A simplified vertex shader for rendering to a shadow map
// --------------------------------------------------------
float4 main(VertexShaderInput input) : SV_POSITION
{
    matrix wvp = mul(projection, mul(view, world));
    return mul(wvp, float4(input.localPosition, 1.0f));
}