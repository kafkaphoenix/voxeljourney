#pragma once

#include <cstddef>
#include <span>

#include "Buffer.h"
#include "UboDefinitions.h"

namespace se::render {
class UniformBuffer {
public:
    UniformBuffer(GLsizeiptr size, UboBinding binding);
    ~UniformBuffer() = default;

    UniformBuffer(const UniformBuffer&) = delete;
    UniformBuffer& operator=(const UniformBuffer&) = delete;
    UniformBuffer(UniformBuffer&& other) noexcept = default;
    UniformBuffer& operator=(UniformBuffer&& other) noexcept = default;

    [[nodiscard]] unsigned int id() const { return m_Buffer.id(); }
    [[nodiscard]] UboBinding binding() const { return m_Binding; }

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
    UboBinding m_Binding{};
};
}  // namespace se::render