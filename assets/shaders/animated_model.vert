#version 460 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;
layout(location = 3) in vec4 a_Tangent;
layout(location = 4) in vec4 a_Color;
layout(location = 5) in ivec4 a_Joints;
layout(location = 6) in vec4 a_Weights;

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
    mat4 u_View;
    mat4 u_Projection;
    vec4 u_SunDir;        // xyz = direction (pointing away from sun)
    vec4 u_SunColor;      // xyz = color, w = intensity
    vec4 u_Ambient;       // xyz = color, w = intensity
    vec4 u_CameraPos;     // xyz = world position
    ivec4 u_LightCounts;  // x = point light count
    PointLight u_PointLights[4];
};

const int MAX_BONES = 128;  // Must match CPU-side definition.
layout(std140, binding = 1) uniform BoneData {
    // mat4 64 bytes * 128 = 8192 bytes, which is within the typical UBO limit of 16KB.
    mat4 u_BoneMatrices[MAX_BONES];
};

uniform mat4 u_Model;
uniform mat3 u_Normal;

void main() {
    // Compute skinning matrix from up to 4 bone influences
    mat4 skinMatrix = a_Weights.x * u_BoneMatrices[a_Joints.x] + a_Weights.y * u_BoneMatrices[a_Joints.y] +
                      a_Weights.z * u_BoneMatrices[a_Joints.z] + a_Weights.w * u_BoneMatrices[a_Joints.w];

    vec4 skinnedPos = skinMatrix * vec4(a_Position, 1.0);
    vec3 skinnedNormal = mat3(skinMatrix) * a_Normal;
    vec3 skinnedTangent = mat3(skinMatrix) * a_Tangent.xyz;

    vec4 worldPos = u_Model * skinnedPos;
    v_WorldPos = worldPos.xyz;
    v_TexCoord = a_TexCoord;
    v_Normal = normalize(u_Normal * skinnedNormal);
    v_Tangent = normalize(u_Normal * skinnedTangent);
    v_BitangentSign = a_Tangent.w;
    v_Color = a_Color;
    gl_Position = u_Projection * u_View * worldPos;
}
