#include "BufferLayout.h"

namespace se::render {

BufferLayout::BufferLayout(const std::vector<BufferElement>& elements) : m_Elements(elements) {
    calculateOffsetsAndStride();
}

void BufferLayout::calculateOffsetsAndStride() {
    GLuint offset = 0;
    m_Stride = 0;
    for (auto& element : m_Elements) {
        element.offset = offset;
        offset += element.size * element.count;
        m_Stride += element.size * element.count;
    }
}

}  // namespace se::render
