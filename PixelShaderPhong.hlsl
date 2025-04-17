
#include "ShaderStructs.hlsli"
#include "Lighting.hlsli"

cbuffer ExternalData : register(b0)
{
    // scene related
    Light lights[MAX_LIGHTS];
    uint nLights;
    float3 ambient;
    
    // camera related
    float3 v3CamPos;
    
    // material related
    float3 colorTint;
    float roughness;
    float2 uvScale;
    float2 uvOffset;
}

// texture related resources
Texture2D Albedo          : register(t0); // "t" registers for textures
Texture2D NormalMap       : register(t1);
SamplerState BasicSampler : register(s0); // "s" registers for samplers

float4 main(VertexToPixel input) : SV_TARGET
{
	// adjust uv coords
    input.normal = normalize(input.normal);
    input.uv = input.uv * uvScale + uvOffset;
    
    // sample texture, uncorrect color, and apply tint
    float3 surfaceColor = pow(Albedo.Sample(BasicSampler, input.uv).rgb, 2.2f);
    float3 albedoColor = surfaceColor * colorTint;
    
    // apply normal map
    input.normal = NormalMapping(NormalMap, BasicSampler, input.uv, input.normal, input.tangent);
    
    // lighting
    float3 totalLight = ambient * surfaceColor;
    for (uint i = 0; i < nLights; i++)
    {
        Light light = lights[i];
        light.Direction = normalize(light.Direction);
        
        switch (light.Type)
        {
            case LIGHT_TYPE_DIRECTIONAL:
                totalLight += DirectionalLight(
                    light, input.normal, input.worldPosition, v3CamPos,
                    roughness, surfaceColor);
                break;
            case LIGHT_TYPE_POINT:
                totalLight += PointLight(
                    light, input.normal, input.worldPosition, v3CamPos,
                    roughness, surfaceColor);
                break;
            case LIGHT_TYPE_SPOT:
                totalLight += SpotLight(
                    light, input.normal, input.worldPosition, v3CamPos,
                    roughness, surfaceColor);
                break;
        }
    }
    //return float4(totalLight, 1);
    return float4(pow(totalLight, 1.0f / 2.2f), 1);
}