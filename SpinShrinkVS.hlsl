#include "ShaderStructs.hlsli"

cbuffer ExternalData : register(b0)
{
	matrix mWorld;
    matrix mWorldIT;
    matrix mView;
    matrix mProj;
    float dt;
    float tt;
}

VertexToPixelBasic main( VertexShaderInput input )
{
	// Set up output struct
	VertexToPixelBasic output;
	
    float scale = 1.0f + 0.5f * sin(tt * 5.0f);
    
    float3 scaledPosition = input.localPosition * scale;
    
    float angle = tt * 5.0f; // Rotate over time
    float cosA = cos(angle);
    float sinA = sin(angle);
    matrix rotationMatrix =
    {
        cosA, 0, -sinA, 0,
        0, 1, 0, 0,
        sinA, 0, cosA, 0,
        0, 0, 0, 1
    };

    float4 rotatedPosition = mul(rotationMatrix, float4(scaledPosition, 1.0f));
    
    matrix wvp = mul(mProj, mul(mView, mWorld));
    output.screenPosition = mul(wvp, rotatedPosition);

    output.uv = input.uv;
    output.normal = mul((float3x3) mWorldIT, input.normal);
    output.tangent = mul((float3x3) mWorld, input.tangent);
    output.uv = input.uv;
    output.worldPosition = mul(mWorld, float4(input.localPosition, 1)).xyz;

	return output;
}