#pragma once

#include <glad/glad.h>

#include <cstddef>
#include <glm/vec3.hpp>

#include "GlBuffer.h"
#include "VertexArray.h"

namespace se::render {

struct AABB {
    glm::vec3 min;
    glm::vec3 max;
};

class Mesh {
   public:
    Mesh(float* vertices, size_t vertSize,
         unsigned int* indices, size_t idxCount, const AABB& aabb);
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&&) = delete;
    Mesh& operator=(Mesh&&) = delete;

    void drawInstanced(size_t count) const;

    unsigned int getVAO() const { return m_Vao.id(); }
    size_t getIndexCount() const { return indexCount; }
    void updateInstanceBuffer(const void* data, size_t size) const;
    static void setDefaultInstanceCapacityBytes(size_t bytes);
    const AABB& getAABB() const { return m_AABB; }
    void setAABB(const AABB& aabb) { m_AABB = aabb; }

   private:
    VertexArray m_Vao;
    GlBuffer m_Vbo;
    GlBuffer m_Ebo;
    AABB m_AABB;
    GlBuffer m_InstanceVbo;
    mutable size_t m_InstanceCapacityBytes = 0;
    size_t indexCount = 0;
    static size_t s_DefaultInstanceCapacityBytes;
};

}  // namespace se::render
