#include "UniformBuffer.h"

namespace se::render {

UniformBuffer::UniformBuffer(GLsizeiptr size, GLuint binding)
    : m_Buffer(GL_UNIFORM_BUFFER), m_Binding(binding) {
    m_Buffer.setData(size, nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, m_Binding, m_Buffer.id());
}

void UniformBuffer::update(GLsizeiptr size, const void* data) const {
    m_Buffer.setData(size, data, GL_DYNAMIC_DRAW);
}

void UniformBuffer::updateSubData(GLintptr offset, GLsizeiptr size, const void* data) const {
    m_Buffer.updateSubData(offset, size, data);
}

}  // namespace se::render
