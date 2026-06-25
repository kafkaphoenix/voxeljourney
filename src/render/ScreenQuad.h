#pragma once

#include "VertexArray.h"

namespace se::render {

// Fullscreen triangle drawn without any vertex buffer.
// Positions and UVs are computed from gl_VertexID in the vertex shader.
// A single oversized triangle avoids the diagonal-seam overdraw of a quad.
// See https://wallisc.github.io/rendering/2021/04/18/Fullscreen-Pass.html
class ScreenQuad {
public:
    ScreenQuad() = default;

    ScreenQuad(const ScreenQuad&) = delete;
    ScreenQuad& operator=(const ScreenQuad&) = delete;
    ScreenQuad(ScreenQuad&&) = default;
    ScreenQuad& operator=(ScreenQuad&&) = default;

    void draw() const;

private:
    VertexArray m_Vao;  // empty VAO (required by core profile)
};

}  // namespace se::render
