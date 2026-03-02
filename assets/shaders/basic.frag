#version 460 core

out vec4 FragColor;
in vec2 v_TexCoord;
in vec3 v_Normal;
in vec3 v_WorldPos;

uniform sampler2D u_Texture;
uniform bool u_HasTexture;
uniform vec4 u_BaseColorFactor;
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
    vec4 baseColor = u_HasTexture ? texture(u_Texture, v_TexCoord) : vec4(1.0);
    return baseColor * u_BaseColorFactor;
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
    vec3 ambient = baseColor * u_Ambient.xyz * u_Ambient.w;
    vec3 diffuse = computeSunDiffuse(baseColor, normal);
    vec3 points = computePointLights(baseColor, normal);
    return ambient + diffuse + points;
}

void main() {
    vec4 baseColor = sampleBaseColor();
    applyAlphaCutoff(baseColor.a);

    vec3 normal = normalize(v_Normal);
    vec3 color = computeLighting(baseColor.rgb, normal);

    FragColor = vec4(color, baseColor.a);
}
