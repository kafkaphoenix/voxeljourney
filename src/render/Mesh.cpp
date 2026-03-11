#include "Mesh.h"

#include <cstddef>
#include <glm/glm.hpp>
#include <stdexcept>

#include "GlUtils.h"
#include "Renderer.h"
#include <cstring>

namespace se::render {

size_t Mesh::s_DefaultInstanceCapacityBytes = 0;

void Mesh::setDefaultInstanceCapacityBytes(size_t bytes) {
    s_DefaultInstanceCapacityBytes = bytes;
}

Mesh::Mesh(float* vertices, size_t vertSize,
           unsigned int* indices, size_t idxCount, const AABB& aabb)
    : indexCount(idxCount), m_AABB(aabb) {
    if (!vertices || !indices || vertSize == 0 || idxCount == 0) {
        throw std::invalid_argument("Invalid mesh data provided!");
    }

    m_Vbo.setData(vertSize, vertices, GL_STATIC_DRAW);

    m_Ebo.setData(
        idxCount * sizeof(unsigned int),
        indices, GL_STATIC_DRAW);

    m_Vao.setVertexBuffer(0, m_Vbo.id(), 0, 8 * sizeof(float));
    m_Vao.setElementBuffer(m_Ebo.id());

    // Position attribute (location = 0)
    m_Vao.enableAttrib(0);
    m_Vao.setAttribFormat(0, 3, GL_FLOAT, GL_FALSE, 0);
    m_Vao.setAttribBinding(0, 0);

    // Normal attribute (location = 1)
    m_Vao.enableAttrib(1);
    m_Vao.setAttribFormat(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float));
    m_Vao.setAttribBinding(1, 0);

    // Texture coordinate attribute (location = 2)
    m_Vao.enableAttrib(2);
    m_Vao.setAttribFormat(2, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float));
    m_Vao.setAttribBinding(2, 0);

    // Setup instance buffer with default capacity if specified
    if (s_DefaultInstanceCapacityBytes > 0) {
        m_InstanceVbo.setData(static_cast<GLsizeiptr>(s_DefaultInstanceCapacityBytes), nullptr, GL_DYNAMIC_DRAW);
        m_InstanceCapacityBytes = s_DefaultInstanceCapacityBytes;
    }

    // Setup instance buffer (binding = 1) We use this buffer for Instanced data like modelMatrix and normalMatrix
    // that's why we set the divisor to 1 below. The actual data will be uploaded later via updateInstanceBuffer()
    // in Renderer::flushBatch() when we know how many instances we need to draw for each batch.
    const GLsizei instanceStride = static_cast<GLsizei>(sizeof(InstanceData));
    m_Vao.setVertexBuffer(1, m_InstanceVbo.id(), 0, instanceStride);

    // Setup instance modelMatrix attributes (locations 3-6)
    for (int i = 0; i < 4; i++) {
        m_Vao.enableAttrib(3 + i);
        m_Vao.setAttribFormat(
            3 + i, 4, GL_FLOAT, GL_FALSE,
            static_cast<GLuint>(offsetof(InstanceData, modelMatrix) + sizeof(glm::vec4) * i));
        m_Vao.setAttribBinding(3 + i, 1);
    }

    // Setup instance normalMatrix attributes (locations 7-9, only 3 vec4s for mat3)
    for (int i = 0; i < 3; i++) {
        m_Vao.enableAttrib(7 + i);
        m_Vao.setAttribFormat(
            7 + i, 3, GL_FLOAT, GL_FALSE,
            static_cast<GLuint>(offsetof(InstanceData, normalMatrix) + sizeof(glm::vec3) * i));
        m_Vao.setAttribBinding(7 + i, 1);
    }
    m_Vao.setBindingDivisor(1, 1);  // Tell OpenGL this is per-instance data

    checkGlError("Mesh::Mesh");
}

void Mesh::drawInstanced(size_t count) const {
    if (count == 0) return;

    m_Vao.bind();

    glDrawElementsInstanced(
        GL_TRIANGLES,
        static_cast<GLsizei>(indexCount),
        GL_UNSIGNED_INT,
        nullptr,
        static_cast<GLsizei>(count));

    checkGlError("Mesh::drawInstanced");

    VertexArray::unbind();
}

void Mesh::updateInstanceBuffer(const void* data, size_t size) const {
    if (size == 0) return;

    if (size > m_InstanceCapacityBytes) {
        size_t newCapacity = m_InstanceCapacityBytes == 0 ? size : m_InstanceCapacityBytes;
        while (newCapacity < size) {
            newCapacity *= 2;
        }
        m_InstanceVbo.setData(static_cast<GLsizeiptr>(newCapacity), nullptr, GL_DYNAMIC_DRAW);
        m_InstanceCapacityBytes = newCapacity;
    }

    void* ptr = m_InstanceVbo.mapWrite(0, static_cast<GLsizeiptr>(size));
    if (ptr) {
        memcpy(ptr, data, size);
        // we signal that we're done writing to the buffer so it can be used for rendering now.
        m_InstanceVbo.unmap();
    }
}

}  // namespace se::render
