#include "UniformBuffer.h"

namespace se::render {

UniformBuffer::UniformBuffer(GLsizeiptr size, GLuint binding) : m_Binding(binding) {
    m_Buffer.setData(size, GL_DYNAMIC_DRAW);  // allocate only
    glBindBufferBase(GL_UNIFORM_BUFFER, m_Binding, m_Buffer.id());
}

void UniformBuffer::update(std::span<const std::byte> data) const { m_Buffer.setData(data, GL_DYNAMIC_DRAW); }

void UniformBuffer::updateSubData(GLintptr offset, std::span<const std::byte> data) const {
    m_Buffer.updateSubData(offset, data);
}

}  // namespace se::render
