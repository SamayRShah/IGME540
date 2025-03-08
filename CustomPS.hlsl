
cbuffer ExternalData : register(b0)
{
    float4 colorTint;
    float dt;
    float tt;
}

struct VertexToPixel
{
    float4 screenPosition	: SV_POSITION; // XYZW position (System Value Position)
    float2 uv				: TEXCOORD;
    float3 normal			: NORMAL;
};

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
        
    color *= d * colorTint + sin(tt)/10.0f;
    
	return float4(color, 1.0f);
}