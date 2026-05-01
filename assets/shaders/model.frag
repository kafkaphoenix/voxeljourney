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
    vec4 positionRange;
    vec4 colorIntensity;
};

layout(std140, binding = 0) uniform FrameData {
    mat4 u_ViewProj;
    vec4 u_SunDir;
    vec4 u_SunColor;
    vec4 u_Ambient;
    vec4 u_LightCounts;
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

vec3 computeSunDiffuse(vec3 baseColor, vec3 normal) {
    vec3 L = -u_SunDir.xyz;
    float NdotL = max(dot(normal, L), 0.0);
    return baseColor * NdotL * u_SunColor.xyz;
}

vec3 computePointLights(vec3 baseColor, vec3 normal) {
    vec3 pointAccum = vec3(0.0);
    int pointCount = int(u_LightCounts.x);
    for (int i = 0; i < pointCount; ++i) {
        vec3 lightPos = u_PointLights[i].positionRange.xyz;
        float range = u_PointLights[i].positionRange.w;
        vec3 lightColor = u_PointLights[i].colorIntensity.xyz;
        float intensity = u_PointLights[i].colorIntensity.w;

        vec3 toLight = lightPos - v_WorldPos;
        float dist = length(toLight);
        vec3 Lp = normalize(toLight);
        float NdotLp = max(dot(normal, Lp), 0.0);
        float attenuation = clamp(1.0 - dist / range, 0.0, 1.0);
        pointAccum += baseColor * NdotLp * lightColor * intensity * attenuation;
    }
    return pointAccum;
}

vec3 computeLighting(vec3 baseColor, vec3 normal) {
    float occlusion = texture(u_Occlusion, v_TexCoord).r;
    vec3 ambient = baseColor * u_Ambient.xyz * u_Ambient.w * occlusion;
    vec3 diffuse = computeSunDiffuse(baseColor, normal);
    vec3 points = computePointLights(baseColor, normal);
    return ambient + diffuse + points;
}

void main() {
    vec4 baseColor = sampleBaseColor();
    applyAlphaCutoff(baseColor.a);

    vec3 normal = perturbNormal();
    vec3 color = computeLighting(baseColor.rgb, normal);

    // Emissive is additive and unaffected by lighting
    vec3 emissive = texture(u_Emissive, v_TexCoord).rgb * u_EmissiveFactor;
    color += emissive;

    FragColor = vec4(color, baseColor.a);
}
