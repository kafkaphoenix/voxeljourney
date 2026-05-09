#version 460 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_Uv;

layout(std140, binding = 2) uniform TerrainFrame {
    mat4 viewProj;
    vec4 sunDir;
    vec4 sunColor;
    vec4 ambient;
};

void main() {
    gl_Position = viewProj * vec4(a_Position, 1.0);
}