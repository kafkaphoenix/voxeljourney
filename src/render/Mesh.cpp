#include "Mesh.h"

#include <cstddef>
#include <cstring>
#include <format>
#include <glm/glm.hpp>
#include <stdexcept>

#include "RenderQueue.h"

namespace se::render {

size_t Mesh::s_DefaultInstanceCapacityBytes = 0;

static size_t nextPowerOfTwo(size_t v) {
    if (v == 0) {
        return 1;
    }
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    if constexpr (sizeof(size_t) == 8) {
        v |= v >> 32;
    }
    v++;
    return v;
}

void Mesh::setDefaultInstanceCapacityBytes(size_t bytes) { s_DefaultInstanceCapacityBytes = nextPowerOfTwo(bytes); }

GLuint Mesh::setupVertexAttributes(const BufferLayout& layout) {
    GLuint attribIndex = 0;
    for (const auto& element : layout.getElements()) {
        m_Vao.enableAttrib(attribIndex);
        if (element.type == GL_INT || element.type == GL_UNSIGNED_INT) {
            m_Vao.setAttribIFormat(attribIndex, element.count, element.type, element.offset);
        } else {
            m_Vao.setAttribFormat(attribIndex, element.count, element.type, element.normalized, element.offset);
        }
        m_Vao.setAttribBinding(attribIndex, 0);
        ++attribIndex;
    }
    return attribIndex;
}

void Mesh::setupInstanceAttributes(GLuint baseIndex) {
    if (s_DefaultInstanceCapacityBytes > 0) {
        m_InstanceVbo.setData(static_cast<GLsizeiptr>(s_DefaultInstanceCapacityBytes),
                              GL_STREAM_DRAW);  // allocate only
        m_InstanceCapacityBytes = s_DefaultInstanceCapacityBytes;
    }

    const auto instanceStride = static_cast<GLsizei>(sizeof(InstanceData));
    m_Vao.setVertexBuffer(1, m_InstanceVbo.id(), 0, static_cast<GLsizeiptr>(instanceStride));

    for (int i = 0; i < 4; i++) {
        const GLuint slot = baseIndex + static_cast<GLuint>(i);
        m_Vao.enableAttrib(slot);
        m_Vao.setAttribFormat(slot, 4, GL_FLOAT, GL_FALSE,
                              static_cast<GLuint>(offsetof(InstanceData, modelMatrix) + sizeof(glm::vec4) * i));
        m_Vao.setAttribBinding(slot, 1);
    }

    for (int i = 0; i < 3; i++) {
        const GLuint slot = baseIndex + 4 + static_cast<GLuint>(i);
        m_Vao.enableAttrib(slot);
        m_Vao.setAttribFormat(slot, 3, GL_FLOAT, GL_FALSE,
                              static_cast<GLuint>(offsetof(InstanceData, normalMatrix) + sizeof(glm::vec3) * i));
        m_Vao.setAttribBinding(slot, 1);
    }

    m_Vao.setBindingDivisor(1, 1);
}

Mesh::Mesh(std::span<const std::byte> vertices, std::span<const unsigned int> indices, const AABB& aabb,
           const BufferLayout& layout, bool instanced)
    : m_IndexCount(indices.size()), m_AABB(aabb), m_Instanced(instanced) {
    if (vertices.empty() || indices.empty()) {
        throw std::invalid_argument("Invalid mesh data provided!");
    }

    m_Vbo.setStorage(vertices);
    m_Ebo.setStorage(std::as_bytes(indices));
    m_Vao.setVertexBuffer(0, m_Vbo.id(), 0, static_cast<GLsizei>(layout.getStride()));
    m_Vao.setElementBuffer(m_Ebo.id());

    GLuint firstFreeSlot = setupVertexAttributes(layout);

    if (m_Instanced) {
        m_InstanceAttribBase = firstFreeSlot;
        setupInstanceAttributes(m_InstanceAttribBase);
    }
}

void Mesh::draw() const {
    if (m_Instanced) {
        throw std::logic_error("draw called on an instanced Mesh; use drawInstanced instead");
    }

    m_Vao.bind();
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_IndexCount), GL_UNSIGNED_INT, nullptr);
}

void Mesh::drawInstanced(size_t count) const {
    if (!m_Instanced) {
        throw std::logic_error("drawInstanced called on a non-instanced Mesh");
    }
    if (count == 0) {
        return;
    }

    m_Vao.bind();
    glDrawElementsInstanced(GL_TRIANGLES, static_cast<GLsizei>(m_IndexCount), GL_UNSIGNED_INT, nullptr,
                            static_cast<GLsizei>(count));
}

void Mesh::updateInstanceBuffer(std::span<const std::byte> data) {
    if (!m_Instanced) {
        throw std::logic_error("updateInstanceBuffer called on a non-instanced Mesh");
    }

    if (data.empty()) {
        return;
    }

    if (data.size_bytes() > m_InstanceCapacityBytes) {
        size_t newCapacity = nextPowerOfTwo(data.size_bytes());
        if (newCapacity > static_cast<size_t>(std::numeric_limits<GLsizeiptr>::max())) {
            throw std::overflow_error("Instance buffer size exceeds GLsizeiptr maximum value");
        }
        m_InstanceVbo.setData(static_cast<GLsizeiptr>(newCapacity), GL_STREAM_DRAW);  // allocate only
        m_InstanceCapacityBytes = newCapacity;
    }

    if (data.size_bytes() > static_cast<size_t>(std::numeric_limits<GLsizeiptr>::max())) {
        throw std::overflow_error("Instance buffer size exceeds GLsizeiptr maximum value");
    }
    auto dataSize = static_cast<GLsizeiptr>(data.size_bytes());
    void* ptr = m_InstanceVbo.mapWrite(0, dataSize);
    if (ptr) {
        memcpy(ptr, data.data(), data.size_bytes());
        m_InstanceVbo.unmap();
    }
}

}  // namespace se::render