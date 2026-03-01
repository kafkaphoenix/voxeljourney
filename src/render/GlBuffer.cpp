#include "GlBuffer.h"

namespace se::render {

GlBuffer::GlBuffer(GLenum target) : m_Target(target) {
    glCreateBuffers(1, &m_Id);
}

GlBuffer::~GlBuffer() {
    release();
}

GlBuffer::GlBuffer(GlBuffer&& other) noexcept
    : m_Id(other.m_Id), m_Target(other.m_Target) {
    other.m_Id = 0;
}

GlBuffer& GlBuffer::operator=(GlBuffer&& other) noexcept {
    if (this != &other) {
        release();
        m_Id = other.m_Id;
        m_Target = other.m_Target;
        other.m_Id = 0;
    }
    return *this;
}

void GlBuffer::setData(GLsizeiptr size, const void* data, GLenum usage) const {
    glNamedBufferData(m_Id, size, data, usage);
}

void GlBuffer::updateSubData(GLintptr offset, GLsizeiptr size, const void* data) const {
    glNamedBufferSubData(m_Id, offset, size, data);
}

void GlBuffer::release() {
    if (m_Id != 0) {
        glDeleteBuffers(1, &m_Id);
        m_Id = 0;
    }
}

}  // namespace se::render
