#pragma once
#include <glad/glad.h>

#include <span>

namespace se::render {

class Buffer {
public:
    Buffer();
    ~Buffer();

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(Buffer&& other) noexcept;

    void setData(std::span<const std::byte> data, GLenum usage) const;
    void setData(GLsizeiptr size, GLenum usage) const;       // allocate only, no data
    void setStorage(std::span<const std::byte> data) const;  // immutable storage (cannot reallocate)
    void updateSubData(GLintptr offset, std::span<const std::byte> data) const;
    [[nodiscard]] void* mapWrite(GLintptr offset, GLsizeiptr size) const;
    void unmap() const;
    [[nodiscard]] unsigned int id() const { return m_Id; }

private:
    void release();

    unsigned int m_Id = 0;
};

}  // namespace se::render
