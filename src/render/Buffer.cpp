#include "Buffer.h"

#include <format>

namespace se::render {

Buffer::Buffer() {
    glCreateBuffers(1, &m_Id);
    std::string bufLabel = std::format("Buffer [{}]", reinterpret_cast<uintptr_t>(this));
    glObjectLabel(GL_BUFFER, m_Id, static_cast<GLsizei>(bufLabel.size()), bufLabel.c_str());
}

void* Buffer::mapWrite(GLintptr offset, GLsizeiptr size) const {
    // GL_MAP_WRITE_BIT: We only need to write to the buffer, so we can avoid some overhead by not allowing reads.
    // GL_MAP_INVALIDATE_RANGE_BIT: We tell OpenGL that we don't care about the existing contents of the buffer
    // in the specified range, which can allow it to avoid unnecessary synchronization and improve performance.
    return glMapNamedBufferRange(m_Id, offset, size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
}

void Buffer::unmap() const { glUnmapNamedBuffer(m_Id); }

Buffer::~Buffer() { release(); }

Buffer::Buffer(Buffer&& other) noexcept : m_Id(other.m_Id) { other.m_Id = 0; }

Buffer& Buffer::operator=(Buffer&& other) noexcept {
    if (this != &other) {
        release();
        m_Id = other.m_Id;
        other.m_Id = 0;
    }
    return *this;
}

void Buffer::setData(std::span<const std::byte> data, GLenum usage) const {
    glNamedBufferData(m_Id, static_cast<GLsizeiptr>(data.size_bytes()), data.data(), usage);
}

void Buffer::setData(GLsizeiptr size, GLenum usage) const { glNamedBufferData(m_Id, size, nullptr, usage); }

void Buffer::setStorage(std::span<const std::byte> data) const {
    glNamedBufferStorage(m_Id, static_cast<GLsizeiptr>(data.size_bytes()), data.data(), 0);
}

void Buffer::updateSubData(GLintptr offset, std::span<const std::byte> data) const {
    glNamedBufferSubData(m_Id, offset, static_cast<GLsizeiptr>(data.size_bytes()), data.data());
}

void Buffer::release() {
    if (m_Id != 0) {
        glDeleteBuffers(1, &m_Id);
        m_Id = 0;
    }
}

}  // namespace se::render
