#version 460 core

out vec4 FragColor;

in vec2 v_TexCoord;

layout(binding = 0) uniform sampler2D u_ScreenTexture;
layout(binding = 1) uniform sampler2D u_OITAccumTexture;
layout(binding = 2) uniform sampler2D u_OITRevealTexture;

uniform float u_Exposure;  // for tone mapping

vec3 toneMapACES(vec3 color) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}

vec3 toneMapACESWithExposure(vec3 color, float exposure) {
    color *= exposure;
    return toneMapACES(color);
}

vec3 gammaCorrect(vec3 color) { return pow(color, vec3(1.0 / 2.2)); }

vec3 compositeTransparency(vec2 uv) {
    vec3 sceneColor = texture(u_ScreenTexture, uv).rgb;
    vec4 accum = texture(u_OITAccumTexture, uv);
    float reveal = texture(u_OITRevealTexture, uv).r;

    vec3 weightedColor = accum.rgb / max(accum.a, 1e-5);
    float transmittance = clamp(reveal, 0.0, 1.0);
    return sceneColor * transmittance + weightedColor * (1.0 - transmittance);
}

void main() {
    vec3 color = compositeTransparency(v_TexCoord);

    color = toneMapACESWithExposure(color, u_Exposure);
    color = gammaCorrect(color);

    FragColor = vec4(color, 1.0);
}