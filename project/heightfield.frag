#version 420

// required by GLSL spec Sect 4.5.3 (though nvidia does not, amd does)
precision highp float;

///////////////////////////////////////////////////////////////////////////////
// Material
///////////////////////////////////////////////////////////////////////////////
layout (binding = 11) uniform sampler2D material_color1;
uniform float material_reflectivity;
uniform float material_metalness;
uniform float material_fresnel;
uniform float material_shininess;
uniform float material_emission;

uniform int has_emission_texture;
layout(binding = 5) uniform sampler2D emissiveMap;

///////////////////////////////////////////////////////////////////////////////
// Environment
///////////////////////////////////////////////////////////////////////////////
layout(binding = 6) uniform sampler2D environmentMap;
layout(binding = 7) uniform sampler2D irradianceMap;
layout(binding = 8) uniform sampler2D reflectionMap;
uniform float environment_multiplier;

///////////////////////////////////////////////////////////////////////////////
// Light source
///////////////////////////////////////////////////////////////////////////////
uniform vec3 point_light_color = vec3(1.0, 1.0, 1.0);
uniform float point_light_intensity_multiplier = 50.0;
uniform vec3 viewSpaceLightDir;
uniform float spotOuterAngle;
uniform float spotInnerAngle;

///////////////////////////////////////////////////////////////////////////////
// Constants
///////////////////////////////////////////////////////////////////////////////
#define PI 3.14159265359

///////////////////////////////////////////////////////////////////////////////
// Input varyings from vertex shader
///////////////////////////////////////////////////////////////////////////////
in vec2 texCoord;
in vec3 viewSpaceNormal;
in vec3 viewSpacePosition;

///////////////////////////////////////////////////////////////////////////////
// shadowmap variables
///////////////////////////////////////////////////////////////////////////////

in vec4 shadowMapCoord;
layout(binding = 10) uniform sampler2DShadow shadowMapTex;

///////////////////////////////////////////////////////////////////////////////
// Input uniform variables
///////////////////////////////////////////////////////////////////////////////
uniform mat4 viewInverse;
uniform vec3 viewSpaceLightPosition;

///////////////////////////////////////////////////////////////////////////////
// Output color
///////////////////////////////////////////////////////////////////////////////
layout(location = 0) out vec4 fragmentColor;
vec3 material_color = vec3(1.0f);



vec3 calculateDirectIllumiunation(vec3 wo, vec3 n)
{
	material_color = texture2D(material_color1, texCoord).rbg;
	vec3 wi = normalize(viewSpaceLightPosition - viewSpacePosition);
	float d = length(viewSpaceLightPosition - viewSpacePosition);
	vec3 Li = point_light_intensity_multiplier * point_light_color * (1 / pow(d, 2));
	float dNWi = dot(n, wi);
	if(dNWi <= 0.0f)
		return vec3(0);

	vec3 diffuse_term = material_color * (1.0/PI) * dNWi * Li;

	vec3 wh = (normalize(wi + wo));
	float s = material_shininess;
	float dNWh = dot(n, wh);
	float dNWo = dot(n, wo);
	float dWoWh = dot(wo, wh);

	float F = material_fresnel + (1 - material_fresnel) * pow((1 - dNWi), 5);
	float D = (s+2) / 2 * PI * pow(dNWh, s);
	float G = min(1, min(2 * dNWh * dNWo/dWoWh, 2 * dNWh * dNWi / dWoWh));
	float brdf = G * D * F / (length(dNWo) * length(dNWi));

	vec3 dielectric_term = brdf * dNWi * Li + (1-F) * diffuse_term;

	vec3 metal_term = brdf * material_color * dNWi * Li;

	vec3 microfacet_term = material_metalness * metal_term + (1-material_metalness) * dielectric_term;

	return material_reflectivity * microfacet_term + (1 - material_reflectivity) * diffuse_term;
}

vec3 calculateIndirectIllumination(vec3 wo, vec3 n)
{
	vec4 nws = viewInverse * vec4(n,0);
	vec3 fragmentColor;

	float theta = acos(max(-1.0f, min(1.0f, nws.y)));
	float phi = atan(nws.z, nws.x);
	if (phi < 0.0f) phi = phi + 2.0f * PI;

	vec2 lookup = vec2(phi / (2.0 * PI), theta / PI);

	fragmentColor = environment_multiplier * texture(irradianceMap, lookup).xyz;
	vec3 diffuse_term = material_color * 1.0f / PI * fragmentColor;

	vec3 wi = vec3(viewInverse * vec4(reflect(-wo, n),0));
	float roughness = sqrt(sqrt(2 / (material_shininess + 2)));
	float dNWi = dot(n,wi);

	float thetaWi = acos(max(-1.0f, min(1.0f, wi.y)));
	float phiWi = atan(wi.z, wi.x);
	if (phiWi < 0.0f) phiWi = phiWi + 2.0f * PI;

	vec2 lookupWi = vec2(phiWi / (2.0 * PI), thetaWi / PI);

	vec3 Li = environment_multiplier * textureLod(reflectionMap, lookupWi, roughness * 7.0).xyz;

	float F = material_fresnel + (1 - material_fresnel) * pow((1 - dNWi), 5);

	//if material is not metal, the light not reflected should be refracted into material
	vec3 dielectric_term = F * Li + (1-F) * diffuse_term;
	//refracted light takes on the material color for metal
	vec3 metal_term = F * material_color * Li;
	//blend dielectric and metal
	vec3 microfacet_term = material_metalness * metal_term + (1-material_metalness) * dielectric_term;

	return material_reflectivity * microfacet_term + (1 - material_reflectivity) * diffuse_term;
}

void main() 
{
	//float depth= texture( shadowMapTex, shadowMapCoord.xy/shadowMapCoord.w ).r;
	//float visibility= (depth>=(shadowMapCoord.z/shadowMapCoord.w)) ? 1.0 : 0.0;
	float visibility = textureProj( shadowMapTex, shadowMapCoord );
	float attenuation = 1.0;
	
	vec3 posToLight = normalize(viewSpaceLightPosition - viewSpacePosition);
	float cosAngle = dot(posToLight, -viewSpaceLightDir);

	// Spotlight with hard border:
	//float spotAttenuation = (cosAngle > spotOuterAngle) ? 1.0 : 0.0;
	float spotAttenuation = smoothstep(spotOuterAngle, spotInnerAngle, cosAngle);
	visibility *= spotAttenuation;

	vec3 wo = -normalize(viewSpacePosition);
	vec3 n = normalize(viewSpaceNormal);

	// Direct illumination
	vec3 direct_illumination_term = visibility * calculateDirectIllumiunation(wo, n);

	// Indirect illumination
	vec3 indirect_illumination_term = calculateIndirectIllumination(wo, n);

	///////////////////////////////////////////////////////////////////////////
	// Add emissive term. If emissive texture exists, sample this term.
	///////////////////////////////////////////////////////////////////////////
	vec3 emission_term = material_emission * material_color;
	if (has_emission_texture == 1) {
		emission_term = texture(emissiveMap, texCoord).xyz;
	}

	vec3 shading = 
		direct_illumination_term +
		indirect_illumination_term +
		emission_term;

	fragmentColor = vec4(shading, 1.0);
	return;
}