#include "VertexArray.h"

namespace se::render {

VertexArray::VertexArray() {
    glCreateVertexArrays(1, &m_Id);
}

VertexArray::~VertexArray() {
    release();
}

VertexArray::VertexArray(VertexArray&& other) noexcept : m_Id(other.m_Id) {
    other.m_Id = 0;
}

VertexArray& VertexArray::operator=(VertexArray&& other) noexcept {
    if (this != &other) {
        release();
        m_Id = other.m_Id;
        other.m_Id = 0;
    }
    return *this;
}

void VertexArray::bind() const {
    glBindVertexArray(m_Id);
}

void VertexArray::unbind() {
    glBindVertexArray(0);
}

void VertexArray::enableAttrib(GLuint index) const {
    glEnableVertexArrayAttrib(m_Id, index);
}

void VertexArray::setAttribFormat(GLuint index, GLint size, GLenum type, GLboolean normalized,
                                  GLuint relativeOffset) const {
    glVertexArrayAttribFormat(m_Id, index, size, type, normalized, relativeOffset);
}

void VertexArray::setAttribBinding(GLuint index, GLuint binding) const {
    glVertexArrayAttribBinding(m_Id, index, binding);
}

void VertexArray::setVertexBuffer(GLuint binding, GLuint buffer, GLintptr offset, GLsizei stride) const {
    glVertexArrayVertexBuffer(m_Id, binding, buffer, offset, stride);
}

void VertexArray::setElementBuffer(GLuint buffer) const {
    glVertexArrayElementBuffer(m_Id, buffer);
}

void VertexArray::setBindingDivisor(GLuint binding, GLuint divisor) const {
    glVertexArrayBindingDivisor(m_Id, binding, divisor);
}

void VertexArray::release() {
    if (m_Id != 0) {
        glDeleteVertexArrays(1, &m_Id);
        m_Id = 0;
    }
}

}  // namespace se::render
