#version 460 core

out vec4 FragColor;
in vec2 v_TexCoord;

layout(binding = 0) uniform sampler2D u_ScreenTexture;

uniform int u_Effect;      // 0=none, 1=tone map, 2=inversion, 3=grayscale, 4=sharpen, 5=blur, 6=edge detect
uniform vec2 u_TexelSize;  // 1.0 / textureSize for kernel offsets

// Reinhard tone mapping: maps HDR colors to [0,1] range
vec3 toneMapReinhard(vec3 color) { return color / (color + vec3(1.0)); }

// sRGB gamma correction
vec3 gammaCorrect(vec3 color) { return pow(color, vec3(1.0 / 2.2)); }

// Applies a 3x3 convolution kernel to the texture
vec3 applyKernel(float kernel[9]) {
    vec2 offsets[9] = vec2[](vec2(-u_TexelSize.x, u_TexelSize.y),   // top-left
                             vec2(0.0, u_TexelSize.y),              // top-center
                             vec2(u_TexelSize.x, u_TexelSize.y),    // top-right
                             vec2(-u_TexelSize.x, 0.0),             // center-left
                             vec2(0.0, 0.0),                        // center
                             vec2(u_TexelSize.x, 0.0),              // center-right
                             vec2(-u_TexelSize.x, -u_TexelSize.y),  // bottom-left
                             vec2(0.0, -u_TexelSize.y),             // bottom-center
                             vec2(u_TexelSize.x, -u_TexelSize.y)    // bottom-right
    );

    vec3 result = vec3(0.0);
    for (int i = 0; i < 9; i++) {
        vec3 texel = texture(u_ScreenTexture, v_TexCoord + offsets[i]).rgb;
        result += texel * kernel[i];
    }
    return result;
}

void main() {
    vec3 color = texture(u_ScreenTexture, v_TexCoord).rgb;

    switch (u_Effect) {
    case 1:  // Tone map only
        color = toneMapReinhard(color);
        color = gammaCorrect(color);
        break;

    case 2:  // Inversion
        color = toneMapReinhard(color);
        color = gammaCorrect(color);
        color = vec3(1.0) - color;
        break;

    case 3: {  // Grayscale (perceptual luminance weights)
        color = toneMapReinhard(color);
        color = gammaCorrect(color);
        float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
        color = vec3(luma);
        break;
    }

    case 4: {  // Sharpen
        color = toneMapReinhard(color);
        color = gammaCorrect(color);
        float kernel[9] = float[](-1, -1, -1, -1, 9, -1, -1, -1, -1);
        color = applyKernel(kernel);
        break;
    }

    case 5: {  // Blur (Gaussian approximation)
        color = toneMapReinhard(color);
        color = gammaCorrect(color);
        float kernel[9] = float[](1.0 / 16.0, 2.0 / 16.0, 1.0 / 16.0, 2.0 / 16.0, 4.0 / 16.0, 2.0 / 16.0, 1.0 / 16.0,
                                  2.0 / 16.0, 1.0 / 16.0);
        color = applyKernel(kernel);
        break;
    }

    case 6: {  // Edge detection
        color = toneMapReinhard(color);
        color = gammaCorrect(color);
        float kernel[9] = float[](1, 1, 1, 1, -8, 1, 1, 1, 1);
        color = applyKernel(kernel);
        break;
    }

    default:  // 0 = raw passthrough
        break;
    }

    FragColor = vec4(color, 1.0);
}
