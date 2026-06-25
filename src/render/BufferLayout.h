#pragma once

#include <glad/glad.h>

#include <string>
#include <vector>

namespace se::render {

struct BufferElement {
    std::string name;
    GLenum type;
    GLuint size;
    GLuint offset;
    GLint count;
    GLboolean normalized;
};

class BufferLayout {
public:
    BufferLayout() = default;
    BufferLayout(const std::vector<BufferElement>& elements);

    [[nodiscard]] const std::vector<BufferElement>& getElements() const { return m_Elements; }
    [[nodiscard]] GLuint getStride() const { return m_Stride; }

    void calculateOffsetsAndStride();

private:
    std::vector<BufferElement> m_Elements;
    GLuint m_Stride = 0;
};

}  // namespace se::render
