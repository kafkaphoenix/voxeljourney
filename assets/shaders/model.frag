#version 460 core

out vec4 FragColor;

in vec2 v_TexCoord;
in vec3 v_Normal;
in vec3 v_WorldPos;
in vec3 v_Tangent;
in float v_BitangentSign;
in vec4 v_Color;

layout(binding = 0) uniform sampler2D u_BaseColor;
layout(binding = 1) uniform sampler2D u_NormalMap;
layout(binding = 2) uniform sampler2D u_MetallicRoughness;
layout(binding = 3) uniform sampler2D u_Emissive;
layout(binding = 4) uniform sampler2D u_Occlusion;

// Material parameters
uniform vec4 u_BaseColorFactor;
uniform float u_MetallicFactor;
uniform float u_RoughnessFactor;
uniform vec3 u_EmissiveFactor;
uniform float u_AlphaCutoff;

struct PointLight {
    vec4 positionRange;   // xyz = position, w = range
    vec4 colorIntensity;  // xyz = color, w = intensity
};

layout(std140, binding = 0) uniform FrameData {
    mat4 u_ViewProj;
    vec4 u_SunDir;        // xyz = direction (pointing away from sun)
    vec4 u_SunColor;      // xyz = color, w = intensity
    vec4 u_Ambient;       // xyz = color, w = intensity
    vec4 u_CameraPos;     // xyz = world position
    ivec4 u_LightCounts;  // x = point light count
    PointLight u_PointLights[4];
};

vec4 sampleBaseColor() {
    vec4 texColor = texture(u_BaseColor, v_TexCoord);
    return texColor * u_BaseColorFactor * v_Color;
}

// Perturb the interpolated vertex normal using the tangent-space normal map.
vec3 perturbNormal() {
    vec3 N = normalize(v_Normal);
    vec3 T = v_Tangent - dot(v_Tangent, N) * N;  // re-orthogonalize
    float tLen = length(T);
    // Degenerate tangent (parallel to normal or zero-length): skip perturbation
    // It happens when the model doesn't have tangents and we generate them from UVs,
    // which can produce bad tangents for faces that are very small in UV space.
    // In that case, we just use the interpolated normal without perturbation, which is
    // better than the artifacts we'd get from a bad tangent.
    if (tLen < 1e-6) {
        return N;
    }
    T /= tLen;
    vec3 B = cross(N, T) * v_BitangentSign;
    mat3 TBN = mat3(T, B, N);
    vec3 tangentNormal = texture(u_NormalMap, v_TexCoord).rgb * 2.0 - 1.0;
    return normalize(TBN * tangentNormal);
}

void applyAlphaCutoff(float alpha) {
    if (alpha < u_AlphaCutoff) {
        discard;
    }
}

// Physically based attenuation with smooth range falloff (Frostbite/UE4 style)
float computeAttenuation(float dist, float range) {
    float factor = clamp(1.0 - pow(dist / range, 4.0), 0.0, 1.0);
    float falloff = factor * factor;
    return (falloff / (dist * dist + 1.0));
}

// Blinn-Phong specular with roughness mapped to shininess (not a physically based specular model, but simple and
// efficient)
float computeSpecular(vec3 N, vec3 V, vec3 L, float roughness) {
    vec3 H = normalize(L + V);
    float NdotH = max(dot(N, H), 0.0);
    float shininess = (1.0 - roughness) * (1.0 - roughness) * 256.0;
    return pow(NdotH, max(shininess, 1.0));
}

// -----------------------------------------------------------------------------
// Lighting
// -----------------------------------------------------------------------------

// directional light is not attenuated by distance, but it can be blocked by shadows, so we still want to use
// the albedo color for the diffuse term of the sun contribution to avoid a hard cutoff between lit and shadowed
// areas on metals.
vec3 computeSunContribution(vec3 albedo, vec3 F0, vec3 N, vec3 V, float roughness, float metallic) {
    vec3 L = normalize(-u_SunDir.xyz);
    float NdotL = max(dot(N, L), 0.0);
    vec3 sunCol = u_SunColor.xyz * u_SunColor.w;

    vec3 diffuse = albedo * NdotL * sunCol;
    float spec = computeSpecular(N, V, L, roughness);
    vec3 specular = F0 * spec * NdotL * sunCol;

    return diffuse + specular;
}

vec3 computePointLightContribution(vec3 albedo, vec3 F0, vec3 N, vec3 V, float roughness, float metallic) {
    vec3 accum = vec3(0.0);
    int count = u_LightCounts.x;

    for (int i = 0; i < count; ++i) {
        vec3 lightPos = u_PointLights[i].positionRange.xyz;
        float range = u_PointLights[i].positionRange.w;
        vec3 lightColor = u_PointLights[i].colorIntensity.xyz;
        float intensity = u_PointLights[i].colorIntensity.w;

        vec3 toLight = lightPos - v_WorldPos;
        float dist = length(toLight);
        vec3 L = normalize(toLight);
        float NdotL = max(dot(N, L), 0.0);
        float atten = computeAttenuation(dist, range) * intensity;

        vec3 diffuse = albedo * NdotL * lightColor * atten;
        float spec = computeSpecular(N, V, L, roughness);
        vec3 specular = F0 * spec * NdotL * lightColor * atten;

        accum += diffuse + specular;
    }
    return accum;
}

vec3 computeLighting(vec3 albedo, vec3 N, float metallic, float roughness, float occlusion) {
    vec3 V = normalize(u_CameraPos.xyz - v_WorldPos);

    // F0: dialectric base reflectance 0.04, metals use albedo as specular color
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // Metals have no diffuse component, but we still want to use the albedo color for ambient and point
    // light diffuse contribution to avoid a hard cutoff between metals and non-metals.
    vec3 diffuseAlbedo = albedo * (1.0 - metallic);

    vec3 ambient = mix(diffuseAlbedo * u_Ambient.xyz * u_Ambient.w,
                       F0 * u_Ambient.xyz * u_Ambient.w,  // metals reflect ambient as tinted specular
                       metallic) *
                   occlusion;
    vec3 sun = computeSunContribution(diffuseAlbedo, F0, N, V, roughness, metallic);
    vec3 points = computePointLightContribution(diffuseAlbedo, F0, N, V, roughness, metallic);

    return ambient + sun + points;
}

void main() {
    vec4 baseColor = sampleBaseColor();
    applyAlphaCutoff(baseColor.a);

    // Calculate material properties from textures and factors
    vec3 mr = texture(u_MetallicRoughness, v_TexCoord).rgb;
    float roughness = mr.g * u_RoughnessFactor;
    float metallic = mr.b * u_MetallicFactor;
    float occlusion = texture(u_Occlusion, v_TexCoord).r;

    vec3 N = perturbNormal();

    vec3 color = computeLighting(baseColor.rgb, N, metallic, roughness, occlusion);

    // Emissive is additive and unaffected by lighting
    vec3 emissive = texture(u_Emissive, v_TexCoord).rgb * u_EmissiveFactor;
    color += emissive;

    FragColor = vec4(color, baseColor.a);
}