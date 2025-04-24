
#include "ShaderStructs.hlsli"
#include "Lighting.hlsli"

cbuffer ExternalData : register(b0)
{
    // scene related
    Light lights[MAX_LIGHTS];
    uint nLights;
    
    // camera related
    float3 v3CamPos;
    
    // material related
    float3 colorTint;
    //float roughness;
    float2 uvScale;
    float2 uvOffset;
}

// texture related resources
Texture2D Albedo                     : register(t0); // "t" registers for textures
Texture2D NormalMap                  : register(t1);
Texture2D RoughnessMap               : register(t2);
Texture2D MetalnessMap               : register(t3);
Texture2D ShadowMap                  : register(t4);
SamplerState BasicSampler            : register(s0); // "s" registers for samplers
SamplerComparisonState ShadowSampler : register(s1);

float4 main(VertexToPixel input) : SV_TARGET
{
    // perspective divide
    input.shadowMapPos /= input.shadowMapPos.w;
    
    // convert normalize coords for sampling
    float2 shadowUV = input.shadowMapPos.xy * 0.5f + 0.5f;
    shadowUV.y = 1 - shadowUV.y; // flip y
    
    // grab distances
    float distToLight = input.shadowMapPos.z;
    float shadowAmount = ShadowMap.SampleCmpLevelZero(
    ShadowSampler, shadowUV, distToLight).r;
    
	// adjust uv coords
    input.normal = normalize(input.normal);
    input.uv = input.uv * uvScale + uvOffset;
    
    // sample texture, uncorrect color, and apply tint
    float3 albedoColor = pow(Albedo.Sample(BasicSampler, input.uv).rgb, 2.2f);
    float3 surfaceColor = albedoColor * colorTint;
    
    //  roughness and metalness maps (single values)
    float roughness = RoughnessMap.Sample(BasicSampler, input.uv).r;
    float metalness = MetalnessMap.Sample(BasicSampler, input.uv).r;
    
    // Specular color determination -----------------
    // Assume albedo texture is actually holding specular color where metalness == 1
    // Note the use of lerp here - metal is generally 0 or 1, but might be in between
    // because of linear texture sampling, so we lerp the specular color to match
    float3 specularColor = lerp(F0_NON_METAL, surfaceColor.rgb, metalness);
    
    // apply normal map
    input.normal = NormalMapping(NormalMap, BasicSampler, input.uv, input.normal, input.tangent);
    
    // lighting
    float3 totalLight;
    for (uint i = 0; i < nLights; i++)
    {
        Light light = lights[i];
        light.Direction = normalize(light.Direction);
        
        switch (light.Type)
        {
            case LIGHT_TYPE_DIRECTIONAL:
                totalLight += DirectionalLightPBR(
                    light, input.normal, input.worldPosition, v3CamPos, 
                    roughness, metalness, surfaceColor, specularColor);
                break;
            case LIGHT_TYPE_POINT:
                totalLight += PointLightPBR(
                    light, input.normal, input.worldPosition, v3CamPos, 
                    roughness, metalness, surfaceColor, specularColor);
                break;
            case LIGHT_TYPE_SPOT:
                totalLight += SpotLightPBR(
                    light, input.normal, input.worldPosition, v3CamPos, 
                    roughness, metalness, surfaceColor, specularColor);
                break;
        }
        
        // If this is the first light, apply the shadowing result
        if (i == 0)
        {
            totalLight *= shadowAmount;
        }
    }
    return float4(pow(totalLight, 1.0f/2.2f), 1);
}