#version 430

##import lib/util
##import lib/lighting_util

layout (binding = 0) uniform sampler2D gBuffer0;
layout (binding = 1) uniform sampler2D gBuffer1;
layout (binding = 2) uniform sampler2D depthStencil;

layout (location = 0) in vec2 inTexCoord;
layout (location = 0) out vec4 outFragColor;

const uint maxLights = 16;

uniform int lightCount;
uniform vec3[maxLights] lightPosition;
uniform vec3[maxLights] lightTint;
uniform vec3[maxLights] lightDirection;

uniform mat4 viewMat;
uniform mat4 invViewMat;
uniform mat4 invProjMat;

const float punctualLightSizeSq = 0.01 * 0.01; // 1cm punctual lights

vec3 evaluateBRDF(vec3 diffuseColor, vec3 specularColor, float roughness, vec3 L, vec3 V, vec3 N) {
	vec3 H = normalize(V + L);
	float NdotV = abs(dot(N, V)) + 1e-5; // see [Lagarde/Rousiers 2014]
	float NdotL = saturate(dot(N, L));
	float NdotH = saturate(dot(N, H));
	float VdotH = saturate(dot(V, H));

	// float D = BRDF_D_Blinn(roughness, NdotH);
	// float Vis = 0.25; // implicit case
	// vec3 F = specularColor; // no fresnel

	float D = BRDF_D_GGX(roughness, NdotH);
	float Vis = BRDF_V_Schlick(roughness, NdotV, NdotL);
	vec3 F = BRDF_F_Schlick(specularColor, VdotH);

	vec3 diffuse = BRDF_Diffuse_Lambert(diffuseColor);
	vec3 specular = D * Vis * F;

	return diffuse + specular;
}

void main() {
	float depth = texture(depthStencil, inTexCoord).r;

	vec4 gb0 = texture(gBuffer0, inTexCoord);
	vec3 baseColor = gb0.rgb;
	float roughness = 1.0 - gb0.a;

	vec4 gb1 = texture(gBuffer1, inTexCoord);
	vec3 normal = gb1.rgb;

	// Unproject depth to reconstruct viewPosition in view-space.
	vec3 viewPosition = ScreenPosToViewPos(inTexCoord, depth, invProjMat);
	vec3 worldPosition = (invViewMat * vec4(viewPosition, 1.0)).xyz;

	vec3 directionToView = normalize(-viewPosition);

	vec3 pixelLuminance = vec3(0);

	for (int i = 0; i < lightCount; i++) {
		vec4 currLightPos = viewMat * vec4(lightPosition[i], 1);
		vec3 sampleToLightRay = currLightPos.xyz / currLightPos.w - viewPosition;
		vec3 incidence = normalize(sampleToLightRay);
		vec3 currLightDir = normalize(mat3(viewMat) * lightDirection[i]);
		float falloff = 1;

		float illuminance = 0;
		vec3 currLightColor = lightTint[i];

		if (illuminance == 0) {
			// Determine physically-based distance attenuation.
			float lightDistance = length(abs(lightPosition[i] - worldPosition));
			float lightDistanceSq = lightDistance * lightDistance;
			falloff = 1.0 / (max(lightDistanceSq, punctualLightSizeSq));

			// Calculate illuminance from intensity with E = L * n dot l.
			illuminance = max(dot(normal, incidence), 0) * 3000 * falloff;
		} else {
			// Given value is the orthogonal case, need to project to l.
			illuminance *= max(dot(normal, incidence), 0);
		}

		// Evaluate BRDF and calculate luminance.
		vec3 brdf = evaluateBRDF(baseColor, vec3(0.04), roughness, incidence, directionToView, normal);
		vec3 luminance = brdf * illuminance * currLightColor;

		// Spotlight attenuation.
		float cosSpotAngle = 0.5;
		float spotTerm = dot(incidence, -currLightDir);
		float spotFalloff = smoothstep(cosSpotAngle, 1, spotTerm) * step(-1, cosSpotAngle) + step(cosSpotAngle, -1);

		float occlusion = 1;

		// Sum output.
		pixelLuminance += occlusion * luminance * spotFalloff;
	}

	// Pre-scale luminance by exposure to avoid exceeding MAX_FLOAT16.
	vec3 exposedLuminance = pixelLuminance * 0.1;
	outFragColor = vec4(exposedLuminance, 1);
}
