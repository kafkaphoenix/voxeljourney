#include "GlBuffer.h"

#include <glad/glad.h>

namespace se::render {

GlBuffer::GlBuffer() {
    glCreateBuffers(1, &m_Id);
}

void* GlBuffer::mapWrite(GLintptr offset, GLsizeiptr size) const {
    // GL_MAP_WRITE_BIT: We only need to write to the buffer, so we can avoid some overhead by not allowing reads.
    // GL_MAP_INVALIDATE_RANGE_BIT: We tell OpenGL that we don't care about the existing contents of the buffer
    // in the specified range, which can allow it to avoid unnecessary synchronization and improve performance.
    // GL_MAP_UNSYNCHRONIZED_BIT: We tell OpenGL that we will handle synchronization ourselves.
    return glMapNamedBufferRange(m_Id, offset, size,
                                 GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
}

void GlBuffer::unmap() const {
    glUnmapNamedBuffer(m_Id);
}

GlBuffer::~GlBuffer() {
    release();
}

GlBuffer::GlBuffer(GlBuffer&& other) noexcept
    : m_Id(other.m_Id) {
    other.m_Id = 0;
}

GlBuffer& GlBuffer::operator=(GlBuffer&& other) noexcept {
    if (this != &other) {
        release();
        m_Id = other.m_Id;
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
