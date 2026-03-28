#pragma once
#include <glad/glad.h>

#include <cstddef>
#include <glm/vec3.hpp>
#include <span>

#include "Buffer.h"
#include "BufferLayout.h"
#include "VertexArray.h"

namespace se::render {

struct AABB {
    glm::vec3 min;
    glm::vec3 max;
};

class Mesh {
public:
    Mesh(std::span<const std::byte> vertices, std::span<const unsigned int> indices, const AABB& aabb,
         const BufferLayout& layout, bool instanced = false);

    // Convenience constructor. Allows passing typed vertex data directly, as long as it's laid out in memory
    // according to the layout.
    template <typename T>
    Mesh(const std::vector<T>& vertices, const std::vector<unsigned int>& indices, const AABB& aabb,
         const BufferLayout& layout, bool instanced = false)
        : Mesh(std::as_bytes(std::span(vertices)), std::span(indices), aabb, layout, instanced) {}

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&&) = delete;
    Mesh& operator=(Mesh&&) = delete;

    void draw() const;
    void drawInstanced(size_t count) const;
    void updateInstanceBuffer(std::span<const std::byte> data);

    // Convenience overload to update instance buffer with typed data directly
    template <typename T>
    void updateInstanceBuffer(const std::vector<T>& data) {
        updateInstanceBuffer(std::as_bytes(std::span(data)));
    }

    [[nodiscard]] unsigned int getVAO() const { return m_Vao.id(); }
    [[nodiscard]] size_t getIndexCount() const { return m_IndexCount; }
    [[nodiscard]] bool isInstanced() const { return m_Instanced; }

    // First attrib slot used by instance data
    [[nodiscard]] GLuint getInstanceAttribBase() const { return m_InstanceAttribBase; }

    static void setDefaultInstanceCapacityBytes(size_t bytes);

    [[nodiscard]] const AABB& getAABB() const { return m_AABB; }
    void setAABB(const AABB& aabb) { m_AABB = aabb; }

private:
    // Sets up per-vertex attribs from layout on binding 0.
    // Returns the first free attrib slot after the layout (used as instanceAttribBase).
    GLuint setupVertexAttributes(const BufferLayout& layout);

    // Sets up modelMatrix (4 x vec4) and normalMatrix (3 x vec3) on binding 1
    // starting at baseIndex. Divisor is set to 1.
    void setupInstanceAttributes(GLuint baseIndex);

    VertexArray m_Vao;
    Buffer m_Vbo;
    Buffer m_Ebo;
    AABB m_AABB;

    // Only allocated when m_Instanced = true
    Buffer m_InstanceVbo;
    size_t m_InstanceCapacityBytes = 0;
    GLuint m_InstanceAttribBase = 0;
    bool m_Instanced = false;

    size_t m_IndexCount = 0;
    static size_t s_DefaultInstanceCapacityBytes;
};

}  // namespace se::render