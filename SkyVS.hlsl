#include "ShaderStructs.hlsli"

cbuffer ExternalData : register(b0)
{
    matrix mView;
    matrix mProj;
}

VertexToPixel_Sky main( VertexShaderInput input)
{
    // setup output struct
    VertexToPixel_Sky output;
    
    // remove translation from view
    matrix mViewNT = mView;
    mViewNT._14 = 0;
    mViewNT._24 = 0;
    mViewNT._34 = 0;
    
    // multiply view and projection
    matrix vp = mul(mProj, mViewNT);
    output.screenPosition = mul(vp, float4(input.localPosition, 1.0f));
    
    // set z to = w to set depth to 1
    output.screenPosition.z = output.screenPosition.w;
    
    // use position as sample direction (direction from origin to vertex pos)
    output.sampleDir = input.localPosition;
    
	return output;
}