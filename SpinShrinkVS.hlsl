cbuffer ExternalData : register(b0)
{
	matrix mWorld;
    matrix mProj;
    matrix mView;
    float dt;
    float tt;
}

struct VertexShaderInput
{ 
	float3 localPosition	: POSITION;     // XYZ position
    float2 uv				: TEXCOORD;     // Texture coordinate
    float3 normal			: NORMAL;		// normal coordinate
};

struct VertexToPixel
{
    float4 screenPosition	: SV_POSITION; // XYZW position (System Value Position)
    float2 uv				: TEXCOORD;
    float3 normal			: NORMAL;
};

VertexToPixel main( VertexShaderInput input )
{
	// Set up output struct
	VertexToPixel output;
	
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
    output.normal = input.normal;

	return output;
}