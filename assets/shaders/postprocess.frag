#version 460 core

out vec4 FragColor;

in vec2 v_TexCoord;

layout(binding = 0) uniform sampler2D u_ScreenTexture;

uniform int u_Effect;      // 0=none, 1=tone map, 2=inversion, 3=grayscale, 4=sharpen, 5=blur, 6=edge detect
uniform vec2 u_TexelSize;  // 1.0 / textureSize for kernel offsets
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

vec3 gammaCorrect(vec3 color) {
    return pow(color, vec3(1.0 / 2.2));
}

vec3 applyKernel(float kernel[9]) {
    vec2 offsets[9] = vec2[](
        vec2(-u_TexelSize.x,  u_TexelSize.y),   // top-left
        vec2( 0.0,            u_TexelSize.y),   // top-center
        vec2( u_TexelSize.x,  u_TexelSize.y),   // top-right
        vec2(-u_TexelSize.x,  0.0),             // center-left
        vec2( 0.0,            0.0),             // center
        vec2( u_TexelSize.x,  0.0),             // center-right
        vec2(-u_TexelSize.x, -u_TexelSize.y),   // bottom-left
        vec2( 0.0,           -u_TexelSize.y),   // bottom-center
        vec2( u_TexelSize.x, -u_TexelSize.y)    // bottom-right
    );

    vec3 result = vec3(0.0);
    for (int i = 0; i < 9; i++) {
        result += texture(u_ScreenTexture, v_TexCoord + offsets[i]).rgb * kernel[i];
    }
    return result;
}

void main() {
    vec3 color = texture(u_ScreenTexture, v_TexCoord).rgb;

    switch (u_Effect) {
        case 0: {  // No effect
            break;
        }
        case 1: {  // Tone mapping and gamma correction
            color = toneMapACESWithExposure(color, u_Exposure);
            color = gammaCorrect(color);
            break;
        }
        case 2: {  // Inversion
            color = toneMapACESWithExposure(color, u_Exposure);
            color = gammaCorrect(color);
            color = vec3(1.0) - color;
            break;
        }
        case 3: {  // Grayscale (perceptual luminance weights)
            color = toneMapACESWithExposure(color, u_Exposure);
            color = gammaCorrect(color);
            float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
            color = vec3(luma);
            break;
        }
        case 4: {  // Sharpen
            float kernel[9] = float[](-1, -1, -1, -1, 9, -1, -1, -1, -1);
            color = applyKernel(kernel);
            color = toneMapACESWithExposure(color, u_Exposure);
            color = gammaCorrect(color);
            break;
        }
        case 5  : {  // Blur (Gaussian approximation)
            float kernel[9] = float[](
                1.0 / 16.0, 2.0 / 16.0, 1.0 / 16.0,
                2.0 / 16.0, 4.0 / 16.0, 2.0 / 16.0,
                1.0 / 16.0, 2.0 / 16.0, 1.0 / 16.0
            );
            color = applyKernel(kernel);
            color = toneMapACESWithExposure(color, u_Exposure);
            color = gammaCorrect(color);
            break;
        }
        case 6: {  // Edge detection
            float kernel[9] = float[](1, 1, 1, 1, -8, 1, 1, 1, 1);
            color = applyKernel(kernel);
            color = toneMapACESWithExposure(color, u_Exposure);
            color = gammaCorrect(color);
            break;
        }
    }

    FragColor = vec4(color, 1.0);
}