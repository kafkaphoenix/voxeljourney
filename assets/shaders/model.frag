#version 460 core

layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 OITAccum;
layout(location = 2) out float OITReveal;

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
uniform float u_OcclusionStrength;
uniform vec3 u_EmissiveFactor;
uniform float u_AlphaCutoff;
uniform int u_TransparencyPath;  // 0=regular, 1=weighted OIT
uniform int u_DebugView;         // 0=lit, 1=normals, 2=albedo, 3=NdotL, 4=roughness, 5=metallic, 6=occlusion,
                                 // 7=normal map, 8=linear depth, 9=material id, 10=oit revealage,
                                 // 11=emissive, 12=shadow factor placeholder
uniform int u_MaterialId;

struct PointLight {
    vec4 positionRange;   // xyz = position, w = range
    vec4 colorIntensity;  // xyz = color, w = intensity
};

layout(std140, binding = 0) uniform FrameData {
    mat4 u_ViewProj;
    mat4 u_Projection;
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
vec3 perturbNormal(vec3 tangentNormal) {
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

    // Metals have no diffuse; only specular via F0. Albedo is zeroed for the diffuse term.
    vec3 diffuseAlbedo = albedo * (1.0 - metallic);

    vec3 ambientDiffuse =
        diffuseAlbedo * u_Ambient.xyz * u_Ambient.w;  // zero for metals since diffuseAlbedo = albedo * (1-metallic)
    vec3 ambientSpecular = F0 * u_Ambient.xyz * u_Ambient.w;  // always present, tinted by albedo for metals
    vec3 ambient = (ambientDiffuse + ambientSpecular) * occlusion;
    vec3 sun = computeSunContribution(diffuseAlbedo, F0, N, V, roughness, metallic);
    vec3 points = computePointLightContribution(diffuseAlbedo, F0, N, V, roughness, metallic);

    return ambient + sun + points;
}

float computeOITWeight(float alpha) {
    float z = gl_FragCoord.z;
    float depthWeight = 10.0 / (1e-5 + pow(z / 5.0, 2.0) + pow(z / 200.0, 6.0));
    return alpha * clamp(depthWeight, 1e-2, 3e3);
}

float linearDepth01(float depth01) {
    float zNdc = depth01 * 2.0 - 1.0;
    float nearPlane = u_Projection[3][2] / (u_Projection[2][2] - 1.0);
    float farPlane = u_Projection[3][2] / (u_Projection[2][2] + 1.0);
    float linear = (2.0 * nearPlane * farPlane) / (farPlane + nearPlane - zNdc * (farPlane - nearPlane));
    return clamp((linear - nearPlane) / max(farPlane - nearPlane, 1e-5), 0.0, 1.0);
}

vec3 hashColorFromId(int id) {
    uint x = uint(id);
    x ^= 2747636419u;
    x *= 2654435769u;
    x ^= x >> 16u;
    x *= 2654435769u;
    x ^= x >> 16u;
    x *= 2654435769u;

    vec3 rgb = vec3(float((x >> 0u) & 255u), float((x >> 8u) & 255u), float((x >> 16u) & 255u)) / 255.0;
    // Keep colors vivid and avoid near-black IDs.
    return mix(vec3(0.35), rgb, 0.85);
}

void main() {
    vec4 baseColor = sampleBaseColor();
    applyAlphaCutoff(baseColor.a);

    // Calculate material properties from textures and factors
    vec3 mr = texture(u_MetallicRoughness, v_TexCoord).rgb;
    float roughness = mr.g * u_RoughnessFactor;
    float metallic = mr.b * u_MetallicFactor;
    float aoRaw = texture(u_Occlusion, v_TexCoord).r;
    float occlusion = mix(1.0, aoRaw, clamp(u_OcclusionStrength, 0.0, 1.0));
    vec3 tangentNormal = texture(u_NormalMap, v_TexCoord).rgb * 2.0 - 1.0;
    vec3 emissive = texture(u_Emissive, v_TexCoord).rgb * u_EmissiveFactor;

    vec3 N = perturbNormal(tangentNormal);

    if (u_DebugView != 0) {
        vec3 debugColor = vec3(0.0);
        float alpha = clamp(baseColor.a, 0.0, 1.0);
        float revealage = (u_TransparencyPath == 1) ? (1.0 - alpha) : 1.0;

        if (u_DebugView == 1) {
            debugColor = N * 0.5 + 0.5;
        } else if (u_DebugView == 2) {
            debugColor = baseColor.rgb;
        } else if (u_DebugView == 3) {
            float ndotl = max(dot(N, normalize(-u_SunDir.xyz)), 0.0);
            debugColor = vec3(ndotl);
        } else if (u_DebugView == 4) {
            debugColor = vec3(roughness);
        } else if (u_DebugView == 5) {
            debugColor = vec3(metallic);
        } else if (u_DebugView == 6) {
            debugColor = vec3(aoRaw);
        } else if (u_DebugView == 7) {
            debugColor = tangentNormal * 0.5 + 0.5;
        } else if (u_DebugView == 8) {
            debugColor = vec3(linearDepth01(gl_FragCoord.z));
        } else if (u_DebugView == 9) {
            debugColor = hashColorFromId(u_MaterialId);
        } else if (u_DebugView == 10) {
            debugColor = vec3(revealage);
        } else if (u_DebugView == 11) {
            debugColor = emissive;
        } else if (u_DebugView == 12) {
            // Placeholder until shadow maps are implemented.
            debugColor = vec3(1.0, 0.0, 1.0);
        }

        FragColor = vec4(debugColor, 1.0);
        OITAccum = vec4(0.0);
        OITReveal = 1.0;
        return;
    }

    vec3 color = computeLighting(baseColor.rgb, N, metallic, roughness, occlusion);

    // Emissive is additive and unaffected by lighting
    color += emissive;

    if (u_TransparencyPath == 1) {
        float alpha = clamp(baseColor.a, 0.0, 1.0);
        float weight = computeOITWeight(alpha);
        OITAccum = vec4(color * alpha * weight, alpha * weight);
        OITReveal = 1.0 - alpha;
        FragColor = vec4(0.0);
    } else {
        FragColor = vec4(color, baseColor.a);
        OITAccum = vec4(0.0);
        // Non-OIT draws must preserve full transmittance in revealage.
        OITReveal = 1.0;
    }
}