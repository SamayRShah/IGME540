struct VertexToPixel
{
    float4 screenPosition	: SV_POSITION; // XYZW position (System Value Position)
    float2 uv				: TEXCOORD;
    float3 normal			: NORMAL;
};

float4 main(VertexToPixel input) : SV_TARGET
{
    return float4(input.normal, 1.0f);
}