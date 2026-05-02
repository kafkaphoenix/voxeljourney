#version 460 core

// Fullscreen triangle (NDC):

//         (-1, 3)
//            *
//           /|
//          / |
//         /  |
//        /   |
//       /    |
//      /     |
// (-1,-1)---*(3,-1)
// This technique avoids the diagonal artifacts that can occur with a fullscreen quad.
out vec2 v_TexCoord;

void main() {
    // Generate positions: (-1,-1), (-1,3), (3,-1)
    vec2 pos = vec2((gl_VertexID & 1) * 4.0 - 1.0, (gl_VertexID & 2) * 2.0 - 1.0);
    v_TexCoord = pos * 0.5 + 0.5;
    gl_Position = vec4(pos, 0.0, 1.0);
}
