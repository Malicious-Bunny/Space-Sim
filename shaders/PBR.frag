#version 450
layout(location = 0) out vec4 outFragColor;
//layout(location = 1) out vec4 outBrightColor;

layout(location = 0) in vec2 inTexCoords;
layout(location = 1) in vec3 inWorldPos;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec4 inPosLightSpace;

// material parameters
layout(set = 2, binding = 0) uniform sampler2D uAlbedoMap;
layout(set = 2, binding = 1) uniform sampler2D uNormalMap;
layout(set = 2, binding = 2) uniform sampler2D uMetallicMap;
layout(set = 2, binding = 3) uniform sampler2D uRoughnessMap;
// layout(set = 3, binding = 4) uniform sampler2D uShadowMap;

const int MAX_LIGHTS = 2;

layout(set = 1, binding = 0) uniform lightsUbo
{
	vec3 lightPositions[MAX_LIGHTS];
	vec4 lightColors[MAX_LIGHTS];
	float numberOfLights;
} lights;


const float PI = 3.14159265359;

// float isInShadow(vec4 fragPosLightSpace)
// {
// 	float planeSize = inFarPlane - inNearPlane;
// 	// perspective divide
//     vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
//     // transform to 0-1 range
//     projCoords = projCoords * 0.5 + 0.5;
//     float closestDepth = texture(uShadowMap, projCoords.xy).r; 
//     float currentDepth = projCoords.z;
//     // calculate bias (based on depth map resolution and slope)
//     vec3 normal = normalize(inNormal);
//     vec3 lightDir = normalize(lights.lightPositions[0] - inWorldPos);
// 	float bias = 0.25 / planeSize;
// 	//float bias = max(0.020f * (1.0f - dot(normal, lightDir)), 0.001f);
//     // check whether current frag pos is in shadow
//     // PCF
//     float shadow = 0.0;
//     vec2 texelSize = 1.0 / textureSize(uShadowMap, 0);
// 	int sampleRadius = 2;
//     for(int x = -sampleRadius; x <= sampleRadius; ++x)
//     {
//         for(int y = -sampleRadius; y <= sampleRadius; ++y)
//         {
//             float pcfDepth = texture(uShadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
//             shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;
//         }    
//     }
//     shadow /= pow(sampleRadius * 2 + 1, 2);
        
//     return shadow;
// }

vec3 getNormalFromMap() {
	vec3 tangentNormal = texture(uNormalMap, inTexCoords).xyz * 2.0 - 1.0;

	vec3 Q1  = dFdx(inWorldPos);
	vec3 Q2  = dFdy(inWorldPos);
	vec2 st1 = dFdx(inTexCoords);
	vec2 st2 = dFdy(inTexCoords);

	vec3 N   = normalize(inNormal);
	vec3 T   = normalize(Q1 * st2.t - Q2 * st1.t);
	vec3 B   = -normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);

	return normalize(TBN * tangentNormal);
}

// Normal distribution
// When the roughness is low (thus the surface is smooth), a highly
// concentrated number of microfacets are aligned to halfway vectors over a small radius.
// On a rough surface however, where the microfacets are aligned in much more random directions
// you'll find a much larger number of halfway vectors
float DistributionGGX(vec3 N, vec3 H, float roughness) {
	float a      = roughness * roughness;
	float a2     = a * a;
	float NdotH  = max(dot(N, H), 0.0);
	float NdotH2 = NdotH * NdotH;

	float nom   = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom       = PI * denom * denom;

	return nom / denom;
}

// The geometry function statistically approximates the relative
// surface area where its micro surface-details overshadow each other, causing light rays to be occluded.
// Geometry function takes a material's roughness parameter as input with rougher surfaces having a higher
// probability of overshadowing microfacets
// To effectively approximate the geometry we need to take account of both the view direction (geometry obstruction)
// and the light direction vector (geometry shadowing).
float GeometrySchlickGGX(float NdotV, float roughness) {
	float r = (roughness + 1.0);
	float k = (r * r) / 8.0;

	float nom   = NdotV;
	float denom = NdotV * (1.0 - k) + k;

	return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	float ggx2  = GeometrySchlickGGX(NdotV, roughness);
	float ggx1  = GeometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}

// The Fresnel Schlick equation is approximation of Fresnel equation which describes the ratio of light that
// gets reflected over the light that gets refracted, which varies over the angle we're looking at a surface.
// the Fresnel equation basically tells us the percentage of light that gets reflected.
// F0 represents the base reflectivity of the surface, which we calculate
// using something called the indices of refraction or IOR
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
	return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
	vec3 albedo     = pow(texture(uAlbedoMap, inTexCoords).rgb, vec3(2.2));
	float metallic   = texture(uMetallicMap, inTexCoords).r;
	float roughness = texture(uRoughnessMap, inTexCoords).r;

	vec3 normal  = getNormalFromMap();
	vec3 viewDir = normalize(vec3(0.0, 0.0, 0.0) - inWorldPos); // {0.0, 0.0, 0.0} is camera position which is always zero

	// Fresnel-Schlick approximation is only really defined for dielectric or
	// non-metal surfaces. For conductor surfaces (metals), calculating the base reflectivity
	// with indices of refraction doesn't properly hold and we need to use a different Fresnel
	// equation for conductors altogether
	// As this is inconvenient, we further approximate by pre-computing the surface's response at
	// normal incidence (F0). By pre-computing F0 for both dielectrics and conductors we can
	// use the same Fresnel-Schlick approximation for both types of surfaces
	// but we do have to tint the base reflectivity if we have a metallic surface.
	// We generally accomplish this as follows
	vec3 F0 = vec3(0.04);                  // We define a base reflectivity that is approximated for most dielectric surfaces
	F0      = mix(F0, albedo, metallic);    // based on if surface is metallic, we either
										   // take the dielectric base reflectivity (by mixing with albedo) or take F0 as surface color

	//  ------------------------- reflectance equation ------------------------------
	vec3 Lo = vec3(0.0);
	for(int i = 0; i < lights.numberOfLights; ++i)    // loop over light sources in this case we have only one so this for loop isn't even needed
	{
		// calculate per-light radiance
		vec3 L            = normalize(lights.lightPositions[i] - inWorldPos);    // L = radiance
		vec3 H            = normalize(viewDir + L);
		float distance    = length(lights.lightPositions[i] - inWorldPos);
		float attenuation = 1.0 / (distance * distance);
		vec3 radiance     = (lights.lightColors[i].rgb * lights.lightColors[i].w);

		// Cook-Torrance BRDF
		float NDF = DistributionGGX(normal, H, roughness);
		float G   = GeometrySmith(normal, viewDir, L, roughness);
		vec3 F    = fresnelSchlick(max(dot(H, viewDir), 0.0), F0);    // calculate the ratio between specular and diffuse reflection

		vec3 numerator    = NDF * G * F;
		float denominator = 4.0 * max(dot(normal, viewDir), 0.0) * max(dot(normal, L), 0.0) + 0.0001;    // 0.0001 to prevent dividing by zero if any of the dot products ends up as 0
		vec3 specular     = numerator / denominator;

		// calculating the specular fraction that amounts the percentage the incoming light's
		// energy is reflected. This way we know both the amount the
		// incoming light reflects and the amount the incoming light refracts
		vec3 specularFraction = F;                               // reflection fraction (how many rays bounces)
		vec3 diffuseFraction  = vec3(1.0) - specularFraction;    // refraction fraction (how many gets consumed by material)
		diffuseFraction *= 1.0 - metallic;                        // greater metallic = diffuseFraction = more bounces

		// scale light by NdotL
		float NdotL = max(dot(normal, L), 0.0);

		// add to outgoing radiance Lo
		Lo += (diffuseFraction * albedo / PI + specular) * radiance * NdotL;
	}

	vec3 ambient = vec3(0.00) * albedo;

	vec3 color = ambient + Lo;

	// color = color / (color + vec3(1.0));    // HDR note that we're doing hdr in hdr shader so no need to do it twice here
	color = pow(color, vec3(1.0 / 2.2));    // gamma correction back to srgb
	//float shadow = 1.0 - isInShadow(inPosLightSpace);

	outFragColor = vec4(color, 1.0);
	// float brightness = dot(outFragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
	// if (brightness > 2.0)
	// {
	// 	outBrightColor = vec4(color, 1.0);
	// }
	// else
	// {
	// 	outBrightColor = vec4(0.0, 0.0, 0.0, 1.0);
	// }
}