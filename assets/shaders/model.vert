#version 460 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;
layout(location = 3) in vec4 a_Tangent;
layout(location = 4) in vec4 a_Color;

layout(location = 5) in mat4 i_Model;
layout(location = 9) in vec3 i_NormalMatrix0;
layout(location = 10) in vec3 i_NormalMatrix1;
layout(location = 11) in vec3 i_NormalMatrix2;

out vec2 v_TexCoord;
out vec3 v_Normal;
out vec3 v_WorldPos;
out vec3 v_Tangent;
out float v_BitangentSign;
out vec4 v_Color;

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
    v_Tangent = normalize(normalMatrix * a_Tangent.xyz);
    v_BitangentSign = a_Tangent.w;
    v_Color = a_Color;
    gl_Position = u_ViewProj * worldPos;
}
