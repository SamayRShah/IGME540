#ifndef __GGP_LIGHTING__
#define __GGP_LIGHTING__

// basic light defines
#define MAX_LIGHTS              32
#define MAX_SPECULAR_EXPONENT   256.0f
#define LIGHT_TYPE_DIRECTIONAL	0
#define LIGHT_TYPE_POINT		1
#define LIGHT_TYPE_SPOT			2

// light struct
struct Light
{
    int Type;
    float3 Direction; // 16 bytes

    float Range;
    float3 Position; // 32 bytes

    float Intensity;
    float3 Color; // 48 bytes

    float SpotInnerAngle;
    float SpotOuterAngle;
    float2 Padding; // 64 bytes
};

// utility functions

// unpack normal map, and get in range
float3 SampleAndUnpackNormalMap(Texture2D map, SamplerState samp, float2 uv)
{
    return map.Sample(samp, uv).rgb * 2.0f - 1.0f;
}

// convert tangent-space normal map to world space normal
float3 NormalMapping(Texture2D map, SamplerState samp, float2 uv, float3 normal, float3 tangent)
{
	// get unpacked normal
    float3 unpackedNormal = normalize(SampleAndUnpackNormalMap(map, samp, uv));

	// Create TBN Matrix
    float3 N = normalize(normal);
    float3 T = normalize(tangent);
    T = normalize(T - N * dot(T, N));
    float3 B = cross(T, N);
    float3x3 TBN = float3x3(T, B, N);

	// Adjust the normal from the map and simply use the results
    return normalize(mul(unpackedNormal, TBN));
}

// lighting helpers
float Diffuse(float3 normal, float3 dirToLight)
{
    return saturate(dot(normal, dirToLight));
}

float SpecularPhong(float3 normal, float3 dirToLight, float3 dirToCam, float roughness, float3 diffuse)
{
    // convert roughness -> sensible exponent value
    float specExponent = (1.0f - roughness) * MAX_SPECULAR_EXPONENT;
    
    // view vector
    float3 V = normalize(dirToCam);
    
    // reflected light 
    float3 R = reflect(dirToLight, normal);
    
    // specular calculation
    float spec = pow(max(dot(R, V), 0.0f), specExponent);
    
    // Cut the specular if the diffuse contribution is zero
    // - any() returns 1 if any component of the param is non-zero
    // - In other words:
    // - If the diffuse amount is 0, any(diffuse) returns 0
    // - If the diffuse amount is != 0, any(diffuse) returns 1
    // - So when diffuse is 0, specular becomes 0
    spec *= any(diffuse);
    
    return spec;
}

float Attenuate(Light light, float3 worldPos)
{
    float dist = distance(light.Position, worldPos);
    float att = saturate(1.0f - (dist * dist / (light.Range * light.Range)));
    return att * att;
}

// light types
float3 DirectionalLight(
    Light light, float3 normal, float3 worldPos, 
    float3 camPos, float roughness, float3 surfaceColor)
{
    float3 toLight = normalize(-light.Direction);
    float3 toCam = normalize(camPos - worldPos);
    
    // calculate diffuse term
    float3 diffuseTerm = Diffuse(normal, toLight);
    
    // calculate specular
    float spec = SpecularPhong(normal, light.Direction, toCam, roughness, diffuseTerm);
    
    // total color
    // return (surfaceColor * (diffuseTerm + spec)) * light.Intensity * light.Color; // Tint specular
    return (surfaceColor * diffuseTerm + spec) * light.Intensity * light.Color; // Don’t tint specular
}

float3 PointLight(
    Light light, float3 normal, float3 worldPos,
    float3 camPos, float roughness, float3 surfaceColor)
{
    float3 toLight = normalize(light.Position - worldPos);
    float3 toCam = normalize(camPos - worldPos);
    
    // calculate attenuation
    float atten = Attenuate(light, worldPos);
    
    // calculate diffuse term
    float3 diffuseTerm = Diffuse(normal, toLight);
    
    // calculate specular
    float spec = SpecularPhong(normal, light.Direction, toCam, roughness, diffuseTerm);
    
   // total color
   // return (surfaceColor * (diffuseTerm + spec)) * atten * light.Intensity * light.Color; // Tint specular
    return (surfaceColor * diffuseTerm + spec) * atten * light.Intensity * light.Color; // Don’t tint specular
}

float3 SpotLight(
    Light light, float3 normal, float3 worldPos,
    float3 camPos, float roughness, float3 surfaceColor)
{
    float3 toLight = normalize(light.Position - worldPos);
    
    // Get cos(angle) between pixel and light direction
    float pixelAngle = saturate(dot(-toLight, light.Direction));
    
    // Get cosines of angles and calculate range
    float cosOuter = cos(light.SpotOuterAngle);
    float cosInner = cos(light.SpotInnerAngle);
    float falloffRange = cosOuter - cosInner;
	
	// Linear falloff over the range, clamp 0-1, apply to light calc
    float spotTerm = saturate((cosOuter - pixelAngle) / falloffRange);
    
    // return point light modified by spot light ring
    return PointLight(light, normal, worldPos, camPos, roughness, surfaceColor) * spotTerm;
}
#endif