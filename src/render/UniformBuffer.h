#pragma once
#include <cstddef>
#include <span>

#include "Buffer.h"

namespace se::render {
class UniformBuffer {
public:
    UniformBuffer(GLsizeiptr size, GLuint binding);
    ~UniformBuffer() = default;

    UniformBuffer(const UniformBuffer&) = delete;
    UniformBuffer& operator=(const UniformBuffer&) = delete;
    UniformBuffer(UniformBuffer&& other) noexcept = default;
    UniformBuffer& operator=(UniformBuffer&& other) noexcept = default;

    [[nodiscard]] unsigned int id() const { return m_Buffer.id(); }
    [[nodiscard]] unsigned int binding() const { return m_Binding; }

    void update(std::span<const std::byte> data) const;
    void updateSubData(GLintptr offset, std::span<const std::byte> data) const;

    // Convenience overloads for typed data
    template <typename T>
    void update(const T& value) const {
        update(std::as_bytes(std::span(&value, 1)));
    }

    // Convenience overloads for typed data
    template <typename T>
    void updateSubData(GLintptr offset, const T& value) const {
        updateSubData(offset, std::as_bytes(std::span(&value, 1)));
    }

private:
    Buffer m_Buffer;
    GLuint m_Binding = 0;
};
}  // namespace se::render