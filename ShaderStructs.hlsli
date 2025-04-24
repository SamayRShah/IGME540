#ifndef __GGP_SHADER_STRUCTS__
#define __GGP_SHADER_STRUCTS__

// Struct representing a single vertex worth of data
// - This should match the vertex definition in our C++ code
// - By "match", I mean the size, order and number of members
// - The name of the struct itself is unimportant, but should be descriptive
// - Each variable must have a semantic, which defines its usage
struct VertexShaderInput
{ 
	float3 localPosition	: POSITION;     // XYZ position
    float2 uv				: TEXCOORD;     // Texture coordinate
    float3 normal			: NORMAL;		// normal coordinate
    float3 tangent          : TANGENT;      // tangent coordinate
};

// Struct representing the data we're sending down the pipeline
// - Should match our pixel shader's input (hence the name: Vertex to Pixel)
// - At a minimum, we need a piece of data defined tagged as SV_POSITION
// - The name of the struct itself is unimportant, but should be descriptive
// - Each variable must have a semantic, which defines its usage
struct VertexToPixel
{
    float4 screenPosition	: SV_POSITION; // XYZW position (System Value Position)
    float2 uv				: TEXCOORD;
    float3 normal			: NORMAL;
    float3 worldPosition    : POSITION;
    float3 tangent          : TANGENT;
    float4 shadowMapPos     : SHADOW_POSITION;
};

struct VertexToPixelBasic
{
    float4 screenPosition : SV_POSITION; // XYZW position (System Value Position)
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 worldPosition : POSITION;
    float3 tangent : TANGENT;
};

struct VertexToPixel_Sky
{
    float4 screenPosition   : SV_POSITION;
    float3 sampleDir        : DIRECTION;
};

#endif