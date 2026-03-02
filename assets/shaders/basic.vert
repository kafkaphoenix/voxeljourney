#version 460 core

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec2 a_TexCoord;

layout (location = 3) in mat4 i_Model;
layout (location = 7) in vec3 i_NormalMatrix0;
layout (location = 8) in vec3 i_NormalMatrix1;
layout (location = 9) in vec3 i_NormalMatrix2;

out vec2 v_TexCoord;
out vec3 v_Normal;
out vec3 v_WorldPos;

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

void main() {
    vec4 worldPos = i_Model * vec4(a_Position, 1.0);
    v_WorldPos = worldPos.xyz;
    v_TexCoord = a_TexCoord;
    mat3 normalMatrix = mat3(i_NormalMatrix0, i_NormalMatrix1, i_NormalMatrix2);
    v_Normal = normalize(normalMatrix * a_Normal);
    gl_Position = u_ViewProj * worldPos;
}
