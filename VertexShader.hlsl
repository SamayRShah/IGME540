
#include "ShaderStructs.hlsli"

cbuffer ExternalData : register(b0)
{
	matrix mWorld;
    matrix mWorldIT;
    matrix mProj;
    matrix mView;
}

// --------------------------------------------------------
// The entry point (main method) for our vertex shader
// 
// - Input is exactly one vertex worth of data (defined by a struct)
// - Output is a single struct of data to pass down the pipeline
// - Named "main" because that's the default the shader compiler looks for
// --------------------------------------------------------
VertexToPixel main( VertexShaderInput input )
{
	// Set up output struct
	VertexToPixel output;
	
    matrix wvp = mul(mProj, mul(mView, mWorld));
    output.screenPosition = mul(wvp, float4(input.localPosition, 1.0f));

	// pass through other data
    output.uv = input.uv;
    output.normal = mul((float3x3) mWorldIT, input.normal); 
    output.worldPosition = mul(mWorld, float4(input.localPosition, 1)).xyz;
    
	// Whatever we return will make its way through the pipeline to the
	// next programmable stage we're using (the pixel shader for now)
	return output;
}