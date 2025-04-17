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

// ====== constants ===========================================================================

// A constant Fresnel value for non-metals (glass and plastic have values of about 0.04)
static const float F0_NON_METAL = 0.04f;

// Minimum roughness for when spec distribution function denominator goes to zero
static const float MIN_ROUGHNESS = 0.0000001f; // 6 zeros after decimal

// Handy to have this as a constant
static const float PI = 3.14159265359f;


// ====== PBR FUNCTIONS ========================================================================

// Lambert diffuse BRDF - Same as the basic lighting diffuse calculation!
// - NOTE: this function assumes the vectors are already NORMALIZED!
float DiffusePBR(float3 normal, float3 dirToLight)
{
    return saturate(dot(normal, dirToLight));
}


// Calculates diffuse amount based on energy conservation
//
// diffuse   - Diffuse amount
// F         - Fresnel result from microfacet BRDF
// metalness - surface metalness amount 
float3 DiffuseEnergyConserve(float3 diffuse, float3 F, float metalness)
{
    return diffuse * (1 - F) * (1 - metalness);
}
 
// Normal Distribution Function: GGX (Trowbridge-Reitz)
//
// a - Roughness
// h - Half vector
// n - Normal
// 
// D(h, n, a) = a^2 / pi * ((n dot h)^2 * (a^2 - 1) + 1)^2
float D_GGX(float3 n, float3 h, float roughness)
{
	// Pre-calculations
    float NdotH = saturate(dot(n, h));
    float NdotH2 = NdotH * NdotH;
    float a = roughness * roughness;
    float a2 = max(a * a, MIN_ROUGHNESS); // Applied after remap!

	// ((n dot h)^2 * (a^2 - 1) + 1)
	// Can go to zero if roughness is 0 and NdotH is 1
    float denomToSquare = NdotH2 * (a2 - 1) + 1;

	// Final value
    return a2 / (PI * denomToSquare * denomToSquare);
}

// Fresnel term - Schlick approx.
// 
// v - View vector
// h - Half vector
// f0 - Value when l = n
//
// F(v,h,f0) = f0 + (1-f0)(1 - (v dot h))^5
float3 F_Schlick(float3 v, float3 h, float3 f0)
{
	// Pre-calculations
    float VdotH = saturate(dot(v, h));

	// Final value
    return f0 + (1 - f0) * pow(1 - VdotH, 5);
}

// Geometric Shadowing - Schlick-GGX
// - k is remapped to a / 2, roughness remapped to (r+1)/2 before squaring!
//
// n - Normal
// v - View vector
//
// G_Schlick(n,v,a) = (n dot v) / ((n dot v) * (1 - k) * k)
//
// Full G(n,v,l,a) term = G_SchlickGGX(n,v,a) * G_SchlickGGX(n,l,a)
float G_SchlickGGX(float3 n, float3 v, float roughness)
{
	// End result of remapping:
    float k = pow(roughness + 1, 2) / 8.0f;
    float NdotV = saturate(dot(n, v));

	// Final value
	// Note: Numerator should be NdotV (or NdotL depending on parameters).
	// However, these are also in the BRDF's denominator, so they'll cancel!
	// We're leaving them out here AND in the BRDF function as the
	// dot products can get VERY small and cause rounding errors.
    return 1 / (NdotV * (1 - k) + k);
}
 
// Cook-Torrance Microfacet BRDF (Specular)
//
// f(l,v) = D(h)F(v,h)G(l,v,h) / 4(n dot l)(n dot v)
// - parts of the denominator are canceled out by numerator (see below)
//
// D() - Normal Distribution Function - Trowbridge-Reitz (GGX)
// F() - Fresnel - Schlick approx
// G() - Geometric Shadowing - Schlick-GGX
float3 MicrofacetBRDF(float3 n, float3 l, float3 v, float roughness, float3 f0, out float3 F_out)
{
	// Other vectors
    float3 h = normalize(v + l);

	// Run numerator functions
    float D = D_GGX(n, h, roughness);
    float3 F = F_Schlick(v, h, f0);
    float G = G_SchlickGGX(n, v, roughness) * G_SchlickGGX(n, l, roughness);
	
	// Pass F out of the function for diffuse balance
    F_out = F;

	// Final specular formula
	// Note: The denominator SHOULD contain (NdotV)(NdotL), but they'd be
	// canceled out by our G() term.  As such, they have been removed
	// from BOTH places to prevent floating point rounding errors.
    float3 specularResult = (D * F * G) / 4;

	// One last non-obvious requirement: According to the rendering equation,
	// specular must have the same NdotL applied as diffuse!  We'll apply
	// that here so that minimal changes are required elsewhere.
    return specularResult * max(dot(n, l), 0);
}

// ========= Lighting Helpers ===============================================================
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

// ====== light types (Basic) ====================================================================
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
    // return (surfaceColor * (diffuseTerm + spec)) 
    //    * light.Intensity * light.Color; // Tint specular
    return (surfaceColor * diffuseTerm + spec) 
        * light.Intensity * light.Color; // Don’t tint specular
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
   // return (surfaceColor * (diffuseTerm + spec)) 
   //   * atten * light.Intensity * light.Color; // Tint specular
    return (surfaceColor * diffuseTerm + spec) 
        * atten * light.Intensity * light.Color; // Don’t tint specular
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
    return PointLight(
        light, normal, worldPos, camPos, roughness, surfaceColor
    ) * spotTerm;
}

// ====== light types (PBR) ======================================================================

float3 DirectionalLightPBR(
    Light light, float3 normal, float3 worldPos,
    float3 camPos, float roughness, float metalness, 
    float3 surfaceColor, float3 specularColor)
{
    // Get normalize direction to the light
    float3 toLight = normalize(-light.Direction);
    float3 toCam = normalize(camPos - worldPos);

	// Calculate the light amounts
    float diff = DiffusePBR(normal, toLight);
    float3 F; // - Fresnel result from microfacet BRDF
    float3 spec = MicrofacetBRDF(normal, toLight, toCam, roughness, specularColor, F);
	
	// Calculate diffuse with energy conservation
	// (Reflected light doesn't get diffused)
    float3 balancedDiff = DiffuseEnergyConserve(diff, spec, metalness);

	// Combine amount with 
    return (balancedDiff * surfaceColor + spec) 
        * light.Intensity * light.Color;
}

float3 PointLightPBR(
    Light light, float3 normal, float3 worldPos,
    float3 camPos, float roughness, float metalness, 
    float3 surfaceColor, float3 specularColor)
{
    // Calc light direction
    float3 toLight = normalize(light.Position - worldPos);
    float3 toCam = normalize(camPos - worldPos);

	// Calculate the light amounts
    float atten = Attenuate(light, worldPos);
    float diff = DiffusePBR(normal, toLight);
    float3 F; // - Fresnel result from microfacet BRDF
    float3 spec = MicrofacetBRDF(normal, toLight, toCam, roughness, specularColor, F);

	// Calculate diffuse with energy conservation
	// (Reflected light doesn't diffuse)
    float3 balancedDiff = DiffuseEnergyConserve(diff, spec, metalness);

	// Combine
    return (balancedDiff * surfaceColor + spec)
        * atten * light.Intensity * light.Color;
}

float3 SpotLightPBR(
    Light light, float3 normal, float3 worldPos,
    float3 camPos, float roughness, float metalness,
    float3 surfaceColor, float3 specularColor)
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
    return PointLightPBR(
        light, normal, worldPos, camPos,
        roughness, metalness, surfaceColor, specularColor
    ) * spotTerm;
}

#endif