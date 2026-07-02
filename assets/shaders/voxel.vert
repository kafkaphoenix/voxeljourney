#version 460 core

layout(location = 0) in uvec4 a_Position;
layout(location = 1) in vec2 a_Uv;

layout(std140, binding = 2) uniform TerrainFrame {
    mat4 viewProj;
    vec4 sunDir;
    vec4 sunColor;
    vec4 ambient;
};

uniform mat4 u_Model;

void main() { gl_Position = viewProj * u_Model * vec4(vec3(a_Position.xyz), 1.0); }