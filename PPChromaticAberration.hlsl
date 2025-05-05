cbuffer externalData : register(b0)
{
    float3 offsets;
    float2 mousePos;
    float2 textureSize;
}
struct VertexToPixel
{
    float4 position       : SV_POSITION;
    float2 uv             : TEXCOORD0;
};
Texture2D Pixels          : register(t0);
SamplerState ClampSampler : register(s0);
float4 main(VertexToPixel input) : SV_TARGET
{
    float2 texCoord = input.uv;
    
    // get mouse pos and center it
    float2 mouseUV = (mousePos / textureSize) - 0.5;
    
    // Compute direction from mouse position (in UV space)
    float2 direction = texCoord - (mousePos/textureSize);
    
    // offset the sample in the direction away from the mouse
    float2 redUV = texCoord + direction * offsets.rr;
    float2 greenUV = texCoord + direction * offsets.gg;
    float2 blueUV = texCoord + direction * offsets.bb;
    
    float r = Pixels.Sample(ClampSampler, redUV).r;
    float g = Pixels.Sample(ClampSampler, greenUV).g;
    float b = Pixels.Sample(ClampSampler, blueUV).b;
    
    return float4(r, g, b, 1.0);
}